#pragma once

#include <memory>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <rclcpp/rclcpp.hpp>

#include "arm_control_package/net/websocket_server.hpp"
#include "arm_control_package/services/plc_service.hpp"
#include "arm_control_package/base/control_context.hpp"

namespace arm_control_package
{
    using ArmApi = arm_common_package::ArmApi;
    using MessageEnvelope = arm_common_package::MessageEnvelope;
	using ArmMsg = arm_common_package::ArmMsg;
	using MessageCodec = arm_common_package::MessageCodec;

    class ApiService
    {
    public:
        void register_handlers();

        void send_message(arm_common_package::MessageEnvelope message)
		{
			websocket_server_->send_message(std::move(message));
		}

        std::string get_identity() const
        {
            return websocket_options_.identity;
        }

        void register_task_handler(WebSocketServer::ApiHandler handler);

        ApiService(WebSocketServer::Options options, std::shared_ptr<PlcService> plc_service, std::shared_ptr<ControlContext> control_context)
            : websocket_options_(options),
              plc_service_(plc_service),
              control_context_(control_context)
        {
            websocket_server_ = std::make_unique<WebSocketServer>(
                std::move(options),
                [this](const std::string &, const std::string &)
                {
                    // TODO
                });

            if (!websocket_server_->start())
            {
                throw std::runtime_error("failed to start websocket server");
            }
        }

        ~ApiService()
        {
            if (websocket_server_)
            {
                websocket_server_->stop();
            }
        }

    private:
        void register_if_enabled(
            ArmApi api,
            WebSocketServer::ApiHandler handler,
            WebSocketServer::HandlerOptions handler_options = WebSocketServer::HandlerOptions{})
        {
            const std::string api_name = api_to_string(api);
            if (api_name.empty() || !handler)
            {
                return;
            }

            websocket_server_->register_handler(api_name, std::move(handler), handler_options);
        }

        void register_auth_handler();

        void register_arm_cmd_handler();

        std::string auth_cmd_identity;
        WebSocketServer::Options websocket_options_;
        std::unique_ptr<WebSocketServer> websocket_server_;
        std::shared_ptr<PlcService> plc_service_;
        std::shared_ptr<ControlContext> control_context_;
    };
}