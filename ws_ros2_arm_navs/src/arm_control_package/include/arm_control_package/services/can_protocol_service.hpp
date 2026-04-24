#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <can_msgs/msg/frame.hpp>
#include <nlohmann/json.hpp>
#include <rclcpp/rclcpp.hpp>

#include "arm_common_package/arm_msg.hpp"

namespace arm_control_package
{

    using CanRosMsg = can_msgs::msg::Frame;
    using RosCanFrame = arm_common_package::RosCanFrame;
    using ArmMsg = arm_common_package::ArmMsg;

    class CanProtocolService
    {
    public:
        struct Options
        {
            std::string can_receive_topic{"/ros2can/rx"};
            std::string can_transmit_topic{"/ros2can/tx"};
            int qos_depth{100};
            uint8_t default_node_id{1};
        };

        CanProtocolService(rclcpp::Node &node, Options options);

        ArmMsg get_status_snapshot() const;

        bool send_control_command(
            const std::string &cmd_type,
            const std::vector<int> &command_params,
            int node_id,
            RosCanFrame *frame_out,
            nlohmann::json *decoded_out,
            std::string *error);

        std::string backend_name() const;

    private:
        void on_can_rx(const typename CanRosMsg::SharedPtr msg);
        void publish_can_frame(const RosCanFrame &frame);

        static bool decode_cmd_payload(
            const std::string &cmd_type,
            const std::vector<int> &command_params,
            uint16_t *base_id,
            std::array<uint8_t, 8> *data,
            nlohmann::json *decoded,
            std::string *error);

        rclcpp::Node &node_;
        Options options_;

        mutable std::mutex status_mutex_;
        ArmMsg status_;

        rclcpp::Publisher<CanRosMsg>::SharedPtr tx_pub_;
        rclcpp::Subscription<CanRosMsg>::SharedPtr rx_sub_;
    };

} // namespace arm_control_package
