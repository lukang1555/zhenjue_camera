#pragma once

#include <chrono>
#include <optional>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>

namespace arm_control_package
{
    class CameraService
    {
    public:
        explicit CameraService(rclcpp::Node &node);

        void start();
        void stop();

        // 预留接口：按速度直接控制水平转动，负数向左、正数向右。
        void send_pan(int pan_speed);
        // 预留接口：停止云台连续运动。
        void send_stop();
        // 预留接口：补光灯开/关（具体ISAPI路径和body可参数化）。
        void send_light_on();
        void send_light_off();
        // 请求一次云台状态（含 azimuth/elevation/absoluteZoom）。
        void request_ptz_status();
        // 一次性执行 P+15（可参数化为任意正向增量）。
        void start_relative_pan_positive(double delta_deg_like);
        // 一次性执行 P-15（可参数化为任意负向增量）。
        void start_relative_pan_negative(double delta_deg_like);
        // 通用相对转动：正值向右，负值向左。
        void start_relative_pan(double delta_deg_like);
        // 执行绝对回零（X=0, Y=0）。
        void move_to_zero();

    private:
        enum class SwingPhase
        {
            MoveRight,
            StopAfterRight,
            MoveLeft,
            StopAfterLeft
        };

        void on_timer();
        void on_status_timer();
        void schedule(std::chrono::milliseconds duration);
        void publish_ptz(int pan, int tilt, int zoom);
        void publish_http(
            const std::string &method,
            const std::string &path,
            const std::string &content_type,
            const std::string &body,
            long timeout_ms = 3000);
        std::string build_control_payload(int pan, int tilt, int zoom) const;
        std::string build_http_payload(
            const std::string &method,
            const std::string &path,
            const std::string &content_type,
            const std::string &body,
            long timeout_ms) const;
        static std::optional<double> extract_xml_numeric_value(const std::string &xml, const std::string &tag);
        void on_control_result(const std_msgs::msg::String::SharedPtr msg);
        void update_relative_pan_once(double azimuth_now);
        void publish_absolute_xy(int azimuth_x10, int elevation_x10);

        rclcpp::Node &node_;
        rclcpp::Publisher<std_msgs::msg::String>::SharedPtr control_pub_;
        rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_pub_;
        rclcpp::Subscription<std_msgs::msg::String>::SharedPtr result_sub_;
        rclcpp::TimerBase::SharedPtr timer_;
        rclcpp::TimerBase::SharedPtr status_timer_;

        std::string device_id_{"cam_01"};
        int pan_speed_{50};
        int move_ms_{1500};
        int pause_ms_{300};
        bool auto_swing_on_start_{true};
        bool status_poll_enabled_{true};
        int status_poll_ms_{500};
        bool light_toggle_enabled_{true};
        std::string light_method_{"PUT"};
        std::string light_path_{"/ISAPI/Image/channels/1/supplementLight"};
        std::string light_content_type_{"application/xml"};
        std::string light_on_body_;
        std::string light_off_body_;
        std::string light_path_fallback_{"/ISAPI/Image/channels/1"};
        std::string light_on_body_fallback_;
        std::string light_off_body_fallback_;

        bool relative_pan_once_enabled_{true};
        double relative_pan_target_delta_{15.0};
        bool relative_pan_active_{false};
        int relative_pan_direction_{1};
        std::optional<double> relative_pan_start_azimuth_;

        SwingPhase phase_{SwingPhase::MoveRight};
        bool running_{false};
    };
} // namespace arm_control_package
