#include <chrono>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <optional>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

#include "arm_control_package/services/camera_service.hpp"

namespace arm_control_package
{
namespace
{
std::optional<double> extract_json_number(const std::string &json, const std::string &key)
{
    const std::string token = "\"" + key + "\":";
    const size_t pos = json.find(token);
    if (pos == std::string::npos)
    {
        return std::nullopt;
    }

    const size_t value_start = pos + token.size();
    const char *start_ptr = json.c_str() + value_start;
    char *end_ptr = nullptr;
    const double value = std::strtod(start_ptr, &end_ptr);
    if (end_ptr == start_ptr)
    {
        return std::nullopt;
    }
    return value;
}
} // namespace

class CameraPtzTestNode : public rclcpp::Node
{
public:
    explicit CameraPtzTestNode(const rclcpp::NodeOptions &options)
        : rclcpp::Node("camera_ptz_test_node", options)
    {
        camera_service_ = std::make_shared<CameraService>(*this);

        status_sub_ = this->create_subscription<std_msgs::msg::String>(
            "camera_ptz_status",
            rclcpp::QoS(10),
            std::bind(&CameraPtzTestNode::on_status, this, std::placeholders::_1));

        // 给话题连通留一点时间，再执行测试动作。
        start_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(600),
            std::bind(&CameraPtzTestNode::start_test, this));

        fallback_timer_ = this->create_wall_timer(
            std::chrono::seconds(12),
            std::bind(&CameraPtzTestNode::force_finish, this));

        RCLCPP_INFO(this->get_logger(), "PTZ test node ready. Plan: light on -> pan +30 -> light off.");
    }

private:
    void start_test()
    {
        if (started_)
        {
            return;
        }
        started_ = true;
        if (start_timer_ != nullptr)
        {
            start_timer_->cancel();
        }

        const auto control_subscribers = this->count_subscribers("camera_http_control");
        const auto result_publishers = this->count_publishers("camera_http_control_result");
        if (control_subscribers == 0 || result_publishers == 0)
        {
            RCLCPP_WARN(
                this->get_logger(),
                "PTZ link not ready: camera_http_control subs=%zu, camera_http_control_result pubs=%zu. "
                "Please run arm_camera_node first.",
                control_subscribers,
                result_publishers);
        }

        camera_service_->request_ptz_status();
        camera_service_->send_light_on();
        camera_service_->start_relative_pan_positive(30.0);
        RCLCPP_INFO(this->get_logger(), "PTZ test started.");
    }

    void on_status(const std_msgs::msg::String::SharedPtr msg)
    {
        if (!started_ || finished_ || msg == nullptr)
        {
            return;
        }

        const auto azimuth = extract_json_number(msg->data, "azimuth");
        if (!azimuth.has_value())
        {
            return;
        }

        if (!start_azimuth_.has_value())
        {
            start_azimuth_ = *azimuth;
            return;
        }

        double delta = *azimuth - *start_azimuth_;
        if (delta < 0.0)
        {
            delta += 360.0;
        }
        delta = std::fmod(delta, 360.0);

        if (delta >= 30.0)
        {
            finish_test("target delta reached");
        }
    }

    void force_finish()
    {
        if (finished_)
        {
            return;
        }
        finish_test("fallback timeout");
    }

    void finish_test(const std::string &reason)
    {
        finished_ = true;
        camera_service_->send_light_off();
        camera_service_->send_stop();
        camera_service_->move_to_zero();

        if (fallback_timer_ != nullptr)
        {
            fallback_timer_->cancel();
        }

        RCLCPP_INFO(this->get_logger(), "PTZ test finished: %s", reason.c_str());
    }

    std::shared_ptr<CameraService> camera_service_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr status_sub_;
    rclcpp::TimerBase::SharedPtr start_timer_;
    rclcpp::TimerBase::SharedPtr fallback_timer_;
    bool started_{false};
    bool finished_{false};
    std::optional<double> start_azimuth_;
};
} // namespace arm_control_package

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);

    rclcpp::NodeOptions options;
    options.append_parameter_override("camera.auto_swing.enabled", false);
    options.append_parameter_override("camera.relative_pan_once.enabled", false);
    options.append_parameter_override("camera.status_poll.enabled", true);

    auto node = std::make_shared<arm_control_package::CameraPtzTestNode>(options);
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
