#include "arm_camera_package/camera_node_base.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace arm_camera_package
{
CameraNodeBase::CameraNodeBase(const std::string &node_name, const rclcpp::NodeOptions &options)
    : CameraNodeBase(node_name, load_config(default_config_path()), options)
{
}

CameraNodeBase::CameraNodeBase(const std::string &node_name, Config config, const rclcpp::NodeOptions &options)
    : rclcpp::Node(node_name, options), config_(std::move(config))
{
    for (const auto &device : config_.devices)
    {
        auto stream = std::make_shared<CameraStreamBase>(
            device.device_id,
            device.stream_uri,
            [this](const FrameMeta &frame)
            {
                this->emit_frame(frame);
            },
            device.prefer_deepstream_decoder,
            device.connection_timeout_ms);

        // TODO 重连尝试
        std::string error;
        if (!stream->start(&error))
        {
            RCLCPP_ERROR(
                this->get_logger(), "Failed to start stream for device %d: %s", device.device_id,
                error.c_str());
            continue;
        }

        RCLCPP_INFO(
            this->get_logger(),
            "Registered camera device id='%d' name='%s' stream='%s' http='%s'.",
            device.device_id,
            device.device_name.c_str(),
            device.stream_uri.c_str(),
            device.http_base_url.c_str());

        streams_.push_back(stream);
    }
}

CameraNodeBase::~CameraNodeBase()
{
    dispose();
}

void CameraNodeBase::on_frame_received(const ImageFrameMsg &frame)
{
    (void)frame;
}

bool CameraNodeBase::get_device_config(int device_id, CameraConfig &config) const
{
    for (const auto &device : config_.devices)
    {
        if (device.device_id == device_id)
        {
            config = device;
            return true;
        }
    }
    return false;
}

void CameraNodeBase::dispose()
{
    for (auto &stream : streams_)
    {
        stream->stop();
    }
    streams_.clear();
}

void CameraNodeBase::emit_frame(const FrameMeta &frame)
{
    auto msg = std::make_unique<ImageFrameMsg>();
    msg->device_id = static_cast<uint32_t>(std::max(frame.device_id, 0));
    msg->width = static_cast<uint32_t>(std::max(frame.width, 0));
    msg->height = static_cast<uint32_t>(std::max(frame.height, 0));
    msg->pts_ns = static_cast<uint64_t>(std::max<int64_t>(frame.pts_ns, 0));
    msg->receive_ns = static_cast<uint64_t>(std::max<int64_t>(frame.receive_ns, 0));

    const bool valid_mat = frame.bgr_mat != nullptr && !frame.bgr_mat->empty() && frame.bgr_mat->isContinuous() && frame.bgr_mat->type() == CV_8UC3;
    if (valid_mat)
    {
        const size_t bytes = frame.bgr_mat->total() * frame.bgr_mat->elemSize();
        const size_t max_bytes = msg->data.size();
        const size_t copy_size = std::min(bytes, max_bytes);
        std::copy_n(frame.bgr_mat->ptr<uint8_t>(), copy_size, msg->data.begin());
        msg->data_size = static_cast<uint32_t>(copy_size);
    }
    else
    {
        msg->data_size = 0;
    }

    on_frame_received(*msg);
}
} // namespace arm_camera_package
