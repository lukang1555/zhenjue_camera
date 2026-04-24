#include "arm_control_package/services/plc_service.hpp"

#include <algorithm>
#include <utility>

namespace arm_control_package
{
    PlcService::PlcService(rclcpp::Node &node, Options options)
        : node_(node),
          options_(std::move(options))
    {
        const auto qos = rclcpp::QoS(static_cast<size_t>(std::max(1, options_.qos_depth)));
        transmit_pub_ = node_.create_publisher<CanRosMsg>(options_.can_transmit_topic, qos);
        receive_sub_ = node_.create_subscription<CanRosMsg>(
            options_.can_receive_topic,
            qos,
            std::bind(&PlcService::on_can_receive, this, std::placeholders::_1));
    }

    ArmMsg PlcService::get_status_snapshot() const
    {
        std::lock_guard<std::mutex> lock(status_mutex_);
        return status_;
    }

    bool PlcService::send_control_command(
        const std::string &cmd_type,
        const std::vector<int> &command_params,
        int node_id,
        RosCanFrame *frame_out,
        nlohmann::json *decoded_out,
        std::string *error)
    {
        uint16_t base_id = 0;
        std::array<uint8_t, 8> data{{0, 0, 0, 0, 0, 0, 0, 0}};
        nlohmann::json decoded;

        if (node_id < 0)
        {
            node_id = static_cast<int>(options_.default_node_id);
        }

        if (!decode_cmd_payload(cmd_type, command_params, &base_id, &data, &decoded, error))
        {
            return false;
        }

        const uint8_t node_id_u8 = static_cast<uint8_t>(std::clamp(node_id, 0, 127));
        const RosCanFrame frame = arm_common_package::make_can_frame(base_id, node_id_u8, data);

        publish_can_frame(frame);

        {
            std::lock_guard<std::mutex> lock(status_mutex_);
            (void)apply_ai_command_frame(frame, status_);
            (void)apply_plc_feedback_frame(frame, status_);
        }

        if (frame_out != nullptr)
        {
            *frame_out = frame;
        }
        if (decoded_out != nullptr)
        {
            *decoded_out = decoded;
        }

        if (on_change_handler_ != nullptr)
        {
            on_change_handler_(status_);
        }

        return true;
    }

    void PlcService::on_can_receive(const typename CanRosMsg::SharedPtr msg)
    {
        RosCanFrame frame;
        frame.id = msg->id;
        frame.is_extended = msg->is_extended;
        frame.is_rtr = msg->is_rtr;
        frame.is_error = msg->is_error;
        frame.dlc = msg->dlc;
        for (size_t i = 0; i < frame.data.size(); ++i)
        {
            frame.data[i] = msg->data[i];
        }

        std::lock_guard<std::mutex> lock(status_mutex_);
        (void)apply_plc_feedback_frame(frame, status_);

        if (on_change_handler_ != nullptr)
        {
            on_change_handler_(status_);
        }
    }

    void PlcService::publish_can_frame(const RosCanFrame &frame)
    {
        CanRosMsg msg;
        msg.id = frame.id;
        msg.is_extended = frame.is_extended;
        msg.is_rtr = frame.is_rtr;
        msg.is_error = frame.is_error;
        msg.dlc = frame.dlc;
        for (size_t i = 0; i < frame.data.size(); ++i)
        {
            msg.data[i] = frame.data[i];
        }

        transmit_pub_->publish(msg);
    }

    bool PlcService::decode_cmd_payload(
        const std::string &cmd_type,
        const std::vector<int> &command_params,
        uint16_t *base_id,
        std::array<uint8_t, 8> *data,
        nlohmann::json *decoded,
        std::string *error)
    {
        if (base_id == nullptr || data == nullptr || decoded == nullptr)
        {
            if (error != nullptr)
            {
                *error = "internal decode target is null";
            }
            return false;
        }

        *data = arm_common_package::to_u8_array(command_params);

        if (cmd_type == "switch")
        {
            *base_id = arm_common_package::CanBaseAiToPlcSwitch;
            *decoded = arm_common_package::to_json(arm_common_package::decode_ai_to_plc_switch(*data));
            return true;
        }
        if (cmd_type == "analog")
        {
            *base_id = arm_common_package::CanBaseAiToPlcAnalog;
            *decoded = arm_common_package::to_json(arm_common_package::decode_ai_to_plc_analog(*data));
            return true;
        }
        if (cmd_type == "status")
        {
            *base_id = arm_common_package::CanBasePlcToAiStatus;
            *decoded = arm_common_package::to_json(arm_common_package::decode_plc_to_ai_status(*data));
            return true;
        }
        if (cmd_type == "telemetry_300")
        {
            *base_id = arm_common_package::CanBasePlcToAiTelemetry300;
            *decoded = arm_common_package::to_json(arm_common_package::decode_plc_to_ai_telemetry_300(*data));
            return true;
        }
        if (cmd_type == "telemetry_400")
        {
            *base_id = arm_common_package::CanBasePlcToAiTelemetry400;
            *decoded = arm_common_package::to_json(arm_common_package::decode_plc_to_ai_telemetry_400(*data));
            return true;
        }

        if (error != nullptr)
        {
            *error = "cmd_type must be one of switch/status/analog/telemetry_300/telemetry_400";
        }
        return false;
    }

} // namespace arm_control_package
