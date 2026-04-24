#pragma once

#include <string>
#include <vector>

#include <rclcpp/rclcpp.hpp>

#include "arm_camera_package/config.hpp"

namespace arm_camera_package
{

    struct FrameMeta
    {
        std::string device_id;
        uint64_t frame_index{0};
        int width{0};
        int height{0};
        int64_t pts_ns{0};
        rclcpp::Time received_stamp{0, 0, RCL_SYSTEM_TIME};
        std::vector<uint8_t> bgr_data;
    };

    class CameraNodeBase : public rclcpp::Node
    {
    public:
        explicit CameraNodeBase(
            const std::string &node_name,
            const rclcpp::NodeOptions &options = rclcpp::NodeOptions());
        ~CameraNodeBase() override = default;

    protected:
        void emit_frame(const FrameMeta &frame);
        virtual void on_frame_received(const FrameMeta &frame);
    };

} // namespace arm_camera_package
