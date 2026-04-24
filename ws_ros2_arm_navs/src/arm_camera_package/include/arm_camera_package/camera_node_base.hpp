#pragma once

#include <memory>
#include <string>
#include <vector>

#include <rclcpp/rclcpp.hpp>

#include "arm_camera_package/camera_stream_base.hpp"
#include "arm_camera_package/config.hpp"
#include "arm_ros_interfaces/msg/image_frame.hpp"

namespace arm_camera_package
{
    using ImageFrameMsg = arm_ros_interfaces::msg::ImageFrame;

    class CameraNodeBase : public rclcpp::Node
    {
    public:
        virtual void on_frame_received(const ImageFrameMsg &frame);

        bool get_device_config(int device_id, CameraConfig &config) const;

        explicit CameraNodeBase(
            const std::string &node_name,
            const rclcpp::NodeOptions &options = rclcpp::NodeOptions());

        CameraNodeBase(
            const std::string &node_name,
            Config config,
            const rclcpp::NodeOptions &options = rclcpp::NodeOptions());

        ~CameraNodeBase() override;

    protected:
        void emit_frame(const FrameMeta &frame);
        void dispose();

        Config config_;
        std::vector<std::shared_ptr<CameraStreamBase>> streams_;
    };

} // namespace arm_camera_package
