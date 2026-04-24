#include "arm_control_package/services/api_service.hpp"

namespace arm_control_package
{
    void ApiService::register_handlers()
    {
        register_auth_handler();
        register_arm_cmd_handler();
    }

    void ApiService::register_auth_handler()
    {
        register_if_enabled(
            ArmApi::ArmControlAuthReq,
            [this](const MessageEnvelope &request)
            {
                const std::string client_identity = request.identity;
                const std::string reason = request.body.value("reason", std::string{});
                auth_cmd_identity = client_identity;
                std::cout << "Received auth request from " << client_identity << ", reason: " << reason << std::endl;

                MessageEnvelope rsp;
                rsp.type = "none";
                rsp.api = arm_common_package::api_to_string(ArmApi::ArmControlAuthRsp);
                rsp.msg_id = MessageCodec::gen_msg_id();
                rsp.identity = websocket_options_.identity;
                rsp.code = arm_common_package::comm_code::Ok;
                rsp.body = nlohmann::json::object();
                rsp.ext = nlohmann::json::object();
                rsp.ts = MessageCodec::now_ms();
                return std::vector<MessageEnvelope>{std::move(rsp)};
            });
    }

    void ApiService::register_arm_cmd_handler()
    {
        register_if_enabled(
            ArmApi::ArmControlCmd,
            [this](const MessageEnvelope &request)
            {
                // TODO 身份认证、任务重置等处理
                auto cmd_id = request.body.value("cmd_id", std::string{});
                auto cmd_type = request.body.value("cmd_type", std::string{});
                auto command_params = request.body.value("command_params", std::vector<int>{});

                // API 当前仅处理开关量和模拟量
                bool success = false;
                std::string error{};
                int node_id = 0; // TODO node_id暂时固定为0
                ArmCmdType cmd_type_enum = parse_arm_cmd_type(cmd_type);
                switch (cmd_type_enum)
                {
                case ArmCmdType::SWITCH:
                case ArmCmdType::ANALOG:
                    success = plc_service_->send_control_command(cmd_type, command_params, node_id, nullptr, nullptr, &error);
                    break;
                default:
                    error = "unsupported cmd_type: " + cmd_type;
                    break;
                }

                MessageEnvelope rsp;
                rsp.type = "none";
                rsp.api = arm_common_package::api_to_string(ArmApi::ArmControlCmdRsp);
                rsp.msg_id = MessageCodec::gen_msg_id();
                rsp.identity = websocket_options_.identity;
                rsp.code = arm_common_package::comm_code::Ok;
                rsp.body = nlohmann::json::object(
                    {{"cmd_id", cmd_id.c_str()},
                     {"success", success},
                     {"error", error.c_str()}});
                rsp.ext = nlohmann::json::object();
                rsp.ts = MessageCodec::now_ms();
                return std::vector<MessageEnvelope>{std::move(rsp)};
            },
            WebSocketServer::HandlerOptions{WebSocketServer::ApiPriority::High});
    }


    void ApiService::register_task_handler(WebSocketServer::ApiHandler handler)
    {
        register_if_enabled(
            ArmApi::ArmControlCmd,
            std::move(handler),
            WebSocketServer::HandlerOptions{WebSocketServer::ApiPriority::High});
    }
}