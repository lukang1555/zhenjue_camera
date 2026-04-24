#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <can_msgs/msg/frame.hpp>
#include <nlohmann/json.hpp>
#include <rclcpp/rclcpp.hpp>

#include "arm_control_package/base/global.hpp"

namespace arm_control_package
{
    class PlcService
    {
    using OnChangeHandler = std::function<void(const ArmMsg)>;

    public:
        struct Options
        {
            std::string can_receive_topic{""};
            std::string can_transmit_topic{""};
            int qos_depth{10};
            uint8_t default_node_id{0};
        };

        PlcService(rclcpp::Node &node, Options options);

        ArmMsg get_status_snapshot() const;

        bool send_control_command(
            const std::string &cmd_type,
            const std::vector<int> &command_params,
            int node_id,
            RosCanFrame *frame_out,
            nlohmann::json *decoded_out,
            std::string *error);

        void register_on_change_handler(OnChangeHandler handler)
        {
            on_change_handler_ = std::move(handler);
        }

    private:
        void on_can_receive(const typename CanRosMsg::SharedPtr msg);
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

        rclcpp::Publisher<CanRosMsg>::SharedPtr transmit_pub_;
        rclcpp::Subscription<CanRosMsg>::SharedPtr receive_sub_;

        // 发送和接收都触发change handler，确保外部状态及时更新
        OnChangeHandler on_change_handler_ = nullptr;
    };

} // namespace arm_control_package
