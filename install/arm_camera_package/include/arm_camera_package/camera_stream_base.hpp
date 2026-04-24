#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <thread>

#include <gst/app/gstappsink.h>
#include <gst/gst.h>

#include "arm_camera_package/camera_node_base.hpp"

namespace arm_camera_package
{

    class CameraStreamBase
    {
    public:
        using StreamFrameCallback = std::function<void(const FrameMeta &)>;

        CameraStreamBase(
            std::string device_id,
            std::string stream_uri,
            StreamFrameCallback callback,
            bool prefer_deepstream_decoder = true,
            int connection_timeout_ms = 8000);
        ~CameraStreamBase();

        CameraStreamBase(const CameraStreamBase &) = delete;
        CameraStreamBase &operator=(const CameraStreamBase &) = delete;

        bool start(std::string *error_msg = nullptr);
        void stop();
        bool is_running() const;

    private:
        static GstFlowReturn on_new_sample(GstAppSink *sink, gpointer user_data);
        GstFlowReturn handle_sample(GstSample *sample);

        bool try_start_with_pipeline(const std::string &pipeline_desc, std::string *error_msg);
        std::string make_pipeline() const;
        void monitor_bus();

        std::string device_id_;
        std::string stream_uri_;
        StreamFrameCallback callback_;
        bool prefer_deepstream_decoder_{true};
        int connection_timeout_ms_{8000};

        std::atomic<bool> running_{false};
        std::atomic<bool> stop_bus_{false};
        uint64_t frame_index_{0};

        GstElement *pipeline_{nullptr};
        GstElement *app_sink_{nullptr};
        std::thread bus_thread_;
    };

} // namespace arm_camera_package
