#include "arm_camera_package/camera_stream_base.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <sstream>

namespace arm_camera_package
{
    namespace
    {
        std::once_flag gstreamer_init_once;

        std::string escape_for_pipeline(const std::string &value)
        {
            std::string escaped;
            escaped.reserve(value.size() + 8);
            for (const char ch : value)
            {
                if (ch == '\\' || ch == '"')
                {
                    escaped.push_back('\\');
                }
                escaped.push_back(ch);
            }
            return escaped;
        }

        uint8_t clamp_to_u8(int value)
        {
            if (value < 0)
            {
                return 0;
            }
            if (value > 255)
            {
                return 255;
            }
            return static_cast<uint8_t>(value);
        }

        void nv12_to_bgr(
            const uint8_t *nv12_data,
            int width,
            int height,
            std::vector<uint8_t> *out_bgr)
        {
            if (nv12_data == nullptr || out_bgr == nullptr || width <= 0 || height <= 0)
            {
                return;
            }

            const size_t y_plane_size = static_cast<size_t>(width) * static_cast<size_t>(height);
            const uint8_t *y_plane = nv12_data;
            const uint8_t *uv_plane = nv12_data + y_plane_size;

            out_bgr->assign(y_plane_size * 3, 0);
            auto &bgr = *out_bgr;

            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    const int y_index = y * width + x;
                    const int uv_index = (y / 2) * width + (x / 2) * 2;

                    const int y_val = static_cast<int>(y_plane[y_index]);
                    const int u_val = static_cast<int>(uv_plane[uv_index]) - 128;
                    const int v_val = static_cast<int>(uv_plane[uv_index + 1]) - 128;

                    const int c = y_val - 16;
                    const int d = u_val;
                    const int e = v_val;

                    const int r = (298 * c + 409 * e + 128) >> 8;
                    const int g = (298 * c - 100 * d - 208 * e + 128) >> 8;
                    const int b = (298 * c + 516 * d + 128) >> 8;

                    const size_t out_idx = static_cast<size_t>(y_index) * 3;
                    bgr[out_idx + 0] = clamp_to_u8(b);
                    bgr[out_idx + 1] = clamp_to_u8(g);
                    bgr[out_idx + 2] = clamp_to_u8(r);
                }
            }
        }
    } // namespace

    CameraStreamBase::CameraStreamBase(
        int device_id,
        std::string stream_uri,
        StreamFrameCallback callback,
        bool prefer_deepstream_decoder,
        int connection_timeout_ms)
        : device_id_(device_id),
          stream_uri_(std::move(stream_uri)),
          callback_(std::move(callback)),
          prefer_deepstream_decoder_(prefer_deepstream_decoder),
          connection_timeout_ms_(std::max(1000, connection_timeout_ms))
    {
    }

    CameraStreamBase::~CameraStreamBase()
    {
        stop();
    }

    bool CameraStreamBase::start(std::string *error_msg)
    {
        if (running_.load())
        {
            return true;
        }

        std::call_once(gstreamer_init_once, []()
                       { gst_init(nullptr, nullptr); });

        std::string local_error;
        if (!try_start_with_pipeline(make_pipeline(), &local_error))
        {
            if (error_msg != nullptr)
            {
                *error_msg = local_error;
            }
            return false;
        }

        stop_bus_.store(false);
        bus_thread_ = std::thread(&CameraStreamBase::monitor_bus, this);
        return true;
    }

    void CameraStreamBase::stop()
    {
        stop_bus_.store(true);
        if (bus_thread_.joinable())
        {
            bus_thread_.join();
        }

        if (pipeline_ != nullptr)
        {
            gst_element_set_state(pipeline_, GST_STATE_NULL);
        }

        if (app_sink_ != nullptr)
        {
            gst_object_unref(app_sink_);
            app_sink_ = nullptr;
        }

        if (pipeline_ != nullptr)
        {
            gst_object_unref(pipeline_);
            pipeline_ = nullptr;
        }

        running_.store(false);
    }

    bool CameraStreamBase::is_running() const
    {
        return running_.load();
    }

    GstFlowReturn CameraStreamBase::on_new_sample(GstAppSink *sink, gpointer user_data)
    {
        auto *self = static_cast<CameraStreamBase *>(user_data);
        if (self == nullptr)
        {
            return GST_FLOW_ERROR;
        }

        GstSample *sample = gst_app_sink_pull_sample(sink);
        if (sample == nullptr)
        {
            return GST_FLOW_ERROR;
        }

        const GstFlowReturn flow = self->handle_sample(sample);
        gst_sample_unref(sample);
        return flow;
    }

    GstFlowReturn CameraStreamBase::handle_sample(GstSample *sample)
    {
        if (sample == nullptr)
        {
            return GST_FLOW_ERROR;
        }

        int width = 0;
        int height = 0;

        const GstCaps *caps = gst_sample_get_caps(sample);
        if (caps != nullptr && gst_caps_get_size(caps) > 0)
        {
            const GstStructure *structure = gst_caps_get_structure(caps, 0);
            gst_structure_get_int(structure, "width", &width);
            gst_structure_get_int(structure, "height", &height);
        }

        GstBuffer *buffer = gst_sample_get_buffer(sample);

        FrameMeta frame;
        frame.device_id = device_id_;
        frame.width = width;
        frame.height = height;
        frame.pts_ns = (buffer != nullptr && GST_BUFFER_PTS_IS_VALID(buffer)) ? static_cast<int64_t>(GST_BUFFER_PTS(buffer)) : 0;
        frame.receive_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                               std::chrono::steady_clock::now().time_since_epoch())
                               .count();

        struct MappedBuffer
        {
            GstBuffer *buf{nullptr};
            GstMapInfo map{};
        };

        auto *mapped = new MappedBuffer();
        mapped->buf = gst_buffer_ref(buffer);
        if (!gst_buffer_map(mapped->buf, &mapped->map, GST_MAP_READ))
        {
            gst_buffer_unref(mapped->buf);
            delete mapped;
            return GST_FLOW_ERROR;
        }

        int encoding_ = CV_8UC3;
        
        // 固定输出 BGR（3通道紧凑）
        cv::Mat header(height, width, encoding_, const_cast<guchar *>(mapped->map.data));

        frame.bgr_mat = std::shared_ptr<cv::Mat>(
            new cv::Mat(header),
            [mapped](cv::Mat *p)
            {
                delete p;
                if (mapped && mapped->buf)
                {
                    gst_buffer_unmap(mapped->buf, &mapped->map);
                    gst_buffer_unref(mapped->buf);
                }
                delete mapped;
            });

        if (callback_)
        {
            try
            {
                callback_(frame);
            }
            catch (const std::exception &ex)
            {
                std::cerr << "[CameraStreamBase] Device " << device_id_ << " callback exception: "
                          << ex.what() << std::endl;
                return GST_FLOW_ERROR;
            }
            catch (...)
            {
                std::cerr << "[CameraStreamBase] Device " << device_id_ << " callback unknown exception"
                          << std::endl;
                return GST_FLOW_ERROR;
            }
        }

        return GST_FLOW_OK;
    }

    bool CameraStreamBase::try_start_with_pipeline(
        const std::string &pipeline_desc,
        std::string *error_msg)
    {
        GError *parse_error = nullptr;
        GstElement *candidate_pipeline = gst_parse_launch(pipeline_desc.c_str(), &parse_error);

        if (candidate_pipeline == nullptr)
        {
            if (error_msg != nullptr)
            {
                *error_msg = (parse_error != nullptr) ? parse_error->message : "gst_parse_launch failed";
            }
            if (parse_error != nullptr)
            {
                g_error_free(parse_error);
            }
            return false;
        }

        GstElement *candidate_sink = gst_bin_get_by_name(GST_BIN(candidate_pipeline), "camera_sink");
        if (candidate_sink == nullptr)
        {
            if (error_msg != nullptr)
            {
                *error_msg = "appsink named 'camera_sink' was not found in pipeline";
            }
            gst_object_unref(candidate_pipeline);
            return false;
        }

        GstAppSink *app_sink = GST_APP_SINK(candidate_sink);
        gst_app_sink_set_emit_signals(app_sink, true);
        gst_app_sink_set_drop(app_sink, true);
        gst_app_sink_set_max_buffers(app_sink, 1);
        g_signal_connect(app_sink, "new-sample", G_CALLBACK(CameraStreamBase::on_new_sample), this);

        const auto state_ret = gst_element_set_state(candidate_pipeline, GST_STATE_PLAYING);
        if (state_ret == GST_STATE_CHANGE_FAILURE)
        {
            if (error_msg != nullptr)
            {
                *error_msg = "pipeline failed to enter PLAYING state";
            }
            gst_object_unref(candidate_sink);
            gst_object_unref(candidate_pipeline);
            return false;
        }

        if (state_ret == GST_STATE_CHANGE_ASYNC)
        {
            GstState current_state = GST_STATE_NULL;
            GstState pending_state = GST_STATE_NULL;
            const auto wait_ret = gst_element_get_state(
                candidate_pipeline,
                &current_state,
                &pending_state,
                static_cast<GstClockTime>(connection_timeout_ms_) * GST_MSECOND);

            if (wait_ret == GST_STATE_CHANGE_FAILURE)
            {
                if (error_msg != nullptr)
                {
                    *error_msg = "pipeline failed while waiting for PLAYING state";
                }
                gst_element_set_state(candidate_pipeline, GST_STATE_NULL);
                gst_object_unref(candidate_sink);
                gst_object_unref(candidate_pipeline);
                return false;
            }

            if (wait_ret == GST_STATE_CHANGE_ASYNC)
            {
                if (error_msg != nullptr)
                {
                    *error_msg = "pipeline startup timeout after " + std::to_string(connection_timeout_ms_) + " ms";
                }
                gst_element_set_state(candidate_pipeline, GST_STATE_NULL);
                gst_object_unref(candidate_sink);
                gst_object_unref(candidate_pipeline);
                return false;
            }
        }

        pipeline_ = candidate_pipeline;
        app_sink_ = candidate_sink;
        running_.store(true);
        return true;
    }

    std::string CameraStreamBase::make_pipeline() const
    {
        const std::string uri = escape_for_pipeline(stream_uri_);
        std::ostringstream ss;
        if (prefer_deepstream_decoder_)
        {
            ss << "rtspsrc location=" << uri
                << " protocols=tcp latency=0 do-rtsp-keep-alive=true retry=10000 ! "
                << "application/x-rtp,media=video,encoding-name=H265 ! "
                << "rtph265depay ! h265parse ! nvv4l2decoder ! "
                << "nvvideoconvert ! video/x-raw,format=BGR ! "
                << "appsink name=camera_sink emit-signals=true sync=false max-buffers=1 drop=true";
        }
        else {
    // 🚀 这是一个针对高分辨率 H.265 优化的通用软件解码管线
           ss.str(""); // 先清空流，防止之前拼接了奇怪的东西
           ss << "rtspsrc location=" << uri << " protocols=tcp latency=500 ! " // 延迟调大到500ms，增加稳定性
              << "rtph265depay ! h265parse ! "
              << "queue max-size-buffers=100 max-size-bytes=104857600 ! " // 🚀 关键：给100MB的缓冲区
              << "avdec_h265 ! "
              << "videoconvert ! "
              << "video/x-raw,format=BGR ! "
              << "appsink name=camera_sink sync=false emit-signals=true max-buffers=2 drop=true";
    
    // 在这下面加一行打印，看看最终拼出来的是什么
              std::cout << "DEBUG PIPE: " << ss.str() << std::endl;
        }   
        // else
        // {
        //     //  软解码通道
        //     ss << "rtspsrc location=" << uri << " protocols=tcp latency=0 ! "
        //        << "rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! video/x-raw,format=BGR ! "
        //        << "appsink name=camera_sink sync=false emit-signals=true max-buffers=1 drop=true";
        // }
        return ss.str();
    }

    void CameraStreamBase::monitor_bus()
    {
        if (pipeline_ == nullptr)
        {
            return;
        }

        GstBus *bus = gst_element_get_bus(pipeline_);
        if (bus == nullptr)
        {
            return;
        }

        while (!stop_bus_.load())
        {
            GstMessage *message = gst_bus_timed_pop_filtered(
                bus,
                100 * GST_MSECOND,
                static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_WARNING | GST_MESSAGE_EOS));

            if (message == nullptr)
            {
                continue;
            }

            switch (GST_MESSAGE_TYPE(message))
            {
            case GST_MESSAGE_ERROR:
            {
                GError *error = nullptr;
                gchar *debug_info = nullptr;
                gst_message_parse_error(message, &error, &debug_info);
                std::cerr << "[CameraStreamBase] Device " << device_id_ << " error: "
                          << (error != nullptr ? error->message : "unknown") << std::endl;
                if (error != nullptr)
                {
                    g_error_free(error);
                }
                if (debug_info != nullptr)
                {
                    g_free(debug_info);
                }
                running_.store(false);
                break;
            }
            case GST_MESSAGE_WARNING:
            {
                GError *warning = nullptr;
                gchar *debug_info = nullptr;
                gst_message_parse_warning(message, &warning, &debug_info);
                std::cerr << "[CameraStreamBase] Device " << device_id_ << " warning: "
                          << (warning != nullptr ? warning->message : "unknown") << std::endl;
                if (warning != nullptr)
                {
                    g_error_free(warning);
                }
                if (debug_info != nullptr)
                {
                    g_free(debug_info);
                }
                break;
            }
            case GST_MESSAGE_EOS:
                std::cerr << "[CameraStreamBase] Device " << device_id_ << " received EOS." << std::endl;
                running_.store(false);
                break;
            default:
                break;
            }

            gst_message_unref(message);
        }

        gst_object_unref(bus);
    }

} // namespace arm_camera_package
