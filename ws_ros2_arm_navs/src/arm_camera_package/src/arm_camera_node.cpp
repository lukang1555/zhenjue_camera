
#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <nlohmann/json.hpp>
#include <std_msgs/msg/string.hpp>

#include "arm_camera_package/camera_node_base.hpp"
#include "arm_camera_package/camera_stream_base.hpp"
#include "arm_camera_package/http_camera_controller.hpp"

namespace arm_camera_package
{
    namespace
    {
        int parse_device_id(const nlohmann::json &payload)
        {
            if (!payload.contains("device_id"))
            {
                throw std::runtime_error("device_id is required");
            }

            const auto &value = payload.at("device_id");
            if (value.is_number_integer())
            {
                return value.get<int>();
            }
            if (value.is_string())
            {
                return std::stoi(value.get<std::string>());
            }
            throw std::runtime_error("device_id must be an integer or integer string");
        }
    } // namespace

    class ArmCameraNode : public CameraNodeBase
    {
    public:
        explicit ArmCameraNode(const rclcpp::NodeOptions &options)
            : CameraNodeBase("arm_camera_node", options)
        {
            for (const auto &cfg : config_.devices)
            {
                if (cfg.publish_topic.empty())
                {
                    continue;
                }

                auto device_publisher = this->create_publisher<ImageFrameMsg>(cfg.publish_topic, rclcpp::QoS(10));
                device_publishers_[cfg.device_id] = std::move(device_publisher);
            }

            // 字符请求
            control_sub_ = this->create_subscription<std_msgs::msg::String>(
                "camera_http_control", rclcpp::QoS(10),
                [this](const std_msgs::msg::String::SharedPtr msg)
                {
                    this->on_http_control(msg);
                });
            control_result_pub_ = this->create_publisher<std_msgs::msg::String>(
                "camera_http_control_result", rclcpp::QoS(10));
        }

        ~ArmCameraNode() override = default;

    private:
        void on_frame_received(const ImageFrameMsg &frame) override
        {
            std::lock_guard<std::mutex> lock(publish_mutex_);
            {
                auto it = device_publishers_.find(frame.device_id);
                if (it != device_publishers_.end())
                {
                    it->second->publish(frame);
                }
            }
        }

        void on_http_control(const std_msgs::msg::String::SharedPtr msg)
        {
            nlohmann::json result;

            try
            {
                const auto payload = nlohmann::json::parse(msg->data);
                const int device_id = parse_device_id(payload);

                CameraConfig config;
                bool has_config = get_device_config(device_id, config);
                if (!has_config)
                {
                    throw std::runtime_error("Device config not found for device_id=" + std::to_string(device_id));
                }

                HttpRequest request;
                request.method = payload.value("method", "GET");
                request.path = payload.value("path", "/");
                request.query = payload.value("query", "");
                request.body = payload.value("body", "");
                request.content_type = payload.value("content_type", "application/json");
                request.timeout_ms = payload.value("timeout_ms", 3000L);

                if (payload.contains("headers") && payload["headers"].is_array())
                {
                    for (const auto &item : payload["headers"])
                    {
                        if (item.is_string())
                        {
                            request.extra_headers.push_back(item.get<std::string>());
                        }
                    }
                }

                const HttpResponse response = http_controller_.send_command(config, request);

                result["ok"] = response.ok();
                result["device_id"] = device_id;
                result["device_name"] = config.device_name;
                result["status_code"] = response.status_code;
                result["body"] = response.body;
                result["error"] = response.error;
            }
            catch (const std::exception &ex)
            {
                result["ok"] = false;
                result["status_code"] = 0;
                result["error"] = ex.what();
            }

            std_msgs::msg::String out;
            out.data = result.dump();

            control_result_pub_->publish(out);
            std::cout << "HTTP control result: " << out.data << std::endl;
        }

        HttpCameraController http_controller_;

        std::mutex publish_mutex_;
        std::unordered_map<int, rclcpp::Publisher<ImageFrameMsg>::SharedPtr> device_publishers_;

        rclcpp::Subscription<std_msgs::msg::String>::SharedPtr control_sub_;
        rclcpp::Publisher<std_msgs::msg::String>::SharedPtr control_result_pub_;
    };

} // namespace arm_camera_package

int main(int argc, char **argv)
{
    try
    {
        rclcpp::init(argc, argv);

        auto node = std::make_shared<arm_camera_package::ArmCameraNode>(rclcpp::NodeOptions{});
        auto executor = std::make_shared<rclcpp::executors::MultiThreadedExecutor>(rclcpp::ExecutorOptions(), 4);
        executor->add_node(node);
        executor->spin();

        rclcpp::shutdown();
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in main: " << e.what() << std::endl;
        if (rclcpp::ok())
        {
            rclcpp::shutdown();
        }
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unknown exception in main" << std::endl;
        if (rclcpp::ok())
        {
            rclcpp::shutdown();
        }
        return 1;
    }
}
