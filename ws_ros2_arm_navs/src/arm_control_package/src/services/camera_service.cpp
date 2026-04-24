#include "arm_control_package/services/camera_service.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>

namespace arm_control_package
{
    namespace
    {
        int clamp_speed(int speed)
        {
            return std::clamp(speed, -100, 100);
        }

        std::string escape_json(const std::string &input)
        {
            std::string out;
            out.reserve(input.size() + 16);
            for (char ch : input)
            {
                if (ch == '\\' || ch == '\"')
                {
                    out.push_back('\\');
                }
                out.push_back(ch);
            }
            return out;
        }
    } // namespace

    CameraService::CameraService(rclcpp::Node &node)
        : node_(node)
    {
        device_id_ = node_.declare_parameter<std::string>("camera.device_id", "1");
        pan_speed_ = clamp_speed(static_cast<int>(node_.declare_parameter("camera.auto_swing.pan_speed", 50)));
        move_ms_ = std::max(100, static_cast<int>(node_.declare_parameter("camera.auto_swing.move_ms", 1800)));
        pause_ms_ = std::max(50, static_cast<int>(node_.declare_parameter("camera.auto_swing.pause_ms", 450)));
        auto_swing_on_start_ = node_.declare_parameter<bool>("camera.auto_swing.enabled", true);
        relative_pan_once_enabled_ = node_.declare_parameter<bool>("camera.relative_pan_once.enabled", true);
        relative_pan_target_delta_ = node_.declare_parameter<double>("camera.relative_pan_once.delta", 15.0);
        status_poll_enabled_ = node_.declare_parameter<bool>("camera.status_poll.enabled", true);
        status_poll_ms_ = std::max(100, static_cast<int>(node_.declare_parameter("camera.status_poll.ms", 500)));
        light_toggle_enabled_ = node_.declare_parameter<bool>("camera.light.toggle_on_swing", true);
        light_method_ = node_.declare_parameter<std::string>("camera.light.method", "PUT");
        light_path_ = node_.declare_parameter<std::string>(
            "camera.light.path",
            "/ISAPI/Image/channels/1/supplementLight");
        light_content_type_ = node_.declare_parameter<std::string>("camera.light.content_type", "application/xml");
        light_on_body_ = node_.declare_parameter<std::string>(
            "camera.light.body_on",
            "<SupplementLight version=\"2.0\" xmlns=\"http://www.hikvision.com/ver20/XMLSchema\">"
            "<supplementLightMode>colorVuWhiteLight</supplementLightMode>"
            "<mixedLightBrightnessRegulatMode>on</mixedLightBrightnessRegulatMode>"
            "<whiteLightBrightness>100</whiteLightBrightness>"
            "</SupplementLight>");
        light_off_body_ = node_.declare_parameter<std::string>(
            "camera.light.body_off",
            "<SupplementLight version=\"2.0\" xmlns=\"http://www.hikvision.com/ver20/XMLSchema\">"
            "<supplementLightMode>close</supplementLightMode>"
            "<mixedLightBrightnessRegulatMode>off</mixedLightBrightnessRegulatMode>"
            "</SupplementLight>");
        light_path_fallback_ = node_.declare_parameter<std::string>(
            "camera.light.path_fallback",
            "/ISAPI/Image/channels/1");
        light_on_body_fallback_ = node_.declare_parameter<std::string>(
            "camera.light.body_on_fallback",
            "<ImageChannel version=\"2.0\" xmlns=\"http://www.hikvision.com/ver20/XMLSchema\">"
            "<SupplementLight>"
            "<supplementLightMode>colorVuWhiteLight</supplementLightMode>"
            "<mixedLightBrightnessRegulatMode>on</mixedLightBrightnessRegulatMode>"
            "<whiteLightBrightness>100</whiteLightBrightness>"
            "</SupplementLight>"
            "</ImageChannel>");
        light_off_body_fallback_ = node_.declare_parameter<std::string>(
            "camera.light.body_off_fallback",
            "<ImageChannel version=\"2.0\" xmlns=\"http://www.hikvision.com/ver20/XMLSchema\">"
            "<SupplementLight>"
            "<supplementLightMode>close</supplementLightMode>"
            "<mixedLightBrightnessRegulatMode>off</mixedLightBrightnessRegulatMode>"
            "</SupplementLight>"
            "</ImageChannel>");

        control_pub_ = node_.create_publisher<std_msgs::msg::String>("camera_http_control", rclcpp::QoS(10));
        status_pub_ = node_.create_publisher<std_msgs::msg::String>("camera_ptz_status", rclcpp::QoS(10));
        result_sub_ = node_.create_subscription<std_msgs::msg::String>(
            "camera_http_control_result",
            rclcpp::QoS(10),
            std::bind(&CameraService::on_control_result, this, std::placeholders::_1));

        if (relative_pan_once_enabled_)
        {
            start_relative_pan_positive(relative_pan_target_delta_);
        }
        else if (auto_swing_on_start_)
        {
            start();
        }

        if (status_poll_enabled_)
        {
            status_timer_ = node_.create_wall_timer(
                std::chrono::milliseconds(status_poll_ms_),
                std::bind(&CameraService::on_status_timer, this));
        }
    }

    void CameraService::start()
    {
        if (running_)
        {
            return;
        }

        running_ = true;
        phase_ = SwingPhase::MoveRight;
        on_timer();
        RCLCPP_INFO(
            node_.get_logger(),
            "Camera auto swing started. device=%s pan_speed=%d move_ms=%d pause_ms=%d",
            device_id_.c_str(),
            pan_speed_,
            move_ms_,
            pause_ms_);
    }

    void CameraService::stop()
    {
        if (!running_ && !relative_pan_active_)
        {
            return;
        }

        running_ = false;
        if (timer_ != nullptr)
        {
            timer_->cancel();
            timer_.reset();
        }
        relative_pan_active_ = false;
        relative_pan_start_azimuth_.reset();
        send_stop();
        RCLCPP_INFO(node_.get_logger(), "Camera auto swing stopped.");
    }

    void CameraService::send_pan(int pan_speed)
    {
        publish_ptz(clamp_speed(pan_speed), 0, 0);
    }

    void CameraService::send_stop()
    {
        publish_ptz(0, 0, 0);
    }

    void CameraService::on_timer()
    {
        if (!running_)
        {
            return;
        }

        switch (phase_)
        {
        case SwingPhase::MoveRight:
            if (light_toggle_enabled_)
            {
                send_light_on();
            }
            send_pan(std::abs(pan_speed_));
            phase_ = SwingPhase::StopAfterRight;
            schedule(std::chrono::milliseconds(move_ms_));
            break;
        case SwingPhase::StopAfterRight:
            send_stop();
            phase_ = SwingPhase::MoveLeft;
            schedule(std::chrono::milliseconds(pause_ms_));
            break;
        case SwingPhase::MoveLeft:
            if (light_toggle_enabled_)
            {
                send_light_off();
            }
            send_pan(-std::abs(pan_speed_));
            phase_ = SwingPhase::StopAfterLeft;
            schedule(std::chrono::milliseconds(move_ms_));
            break;
        case SwingPhase::StopAfterLeft:
            send_stop();
            phase_ = SwingPhase::MoveRight;
            schedule(std::chrono::milliseconds(pause_ms_));
            break;
        }
    }

    void CameraService::on_status_timer()
    {
        request_ptz_status();
    }

    void CameraService::schedule(std::chrono::milliseconds duration)
    {
        if (!running_)
        {
            return;
        }

        timer_ = node_.create_wall_timer(duration, [this]()
                                         { this->on_timer(); });
    }

    void CameraService::publish_ptz(int pan, int tilt, int zoom)
    {
        publish_http(
            "PUT",
            "/ISAPI/PTZCtrl/channels/1/continuous",
            "application/xml",
            [&]()
            {
                std::ostringstream xml;
                xml << "<PTZData><pan>" << pan << "</pan><tilt>" << tilt << "</tilt><zoom>" << zoom << "</zoom></PTZData>";
                return xml.str();
            }(),
            3000);
    }

    void CameraService::send_light_on()
    {
        publish_http(light_method_, light_path_, light_content_type_, light_on_body_, 3000);
        publish_http("PUT", light_path_fallback_, light_content_type_, light_on_body_fallback_, 3000);
    }

    void CameraService::send_light_off()
    {
        publish_http(light_method_, light_path_, light_content_type_, light_off_body_, 3000);
        publish_http("PUT", light_path_fallback_, light_content_type_, light_off_body_fallback_, 3000);
    }

    void CameraService::request_ptz_status()
    {
        publish_http("GET", "/ISAPI/PTZCtrl/channels/1/status", "application/xml", "", 2000);
    }

    void CameraService::start_relative_pan_positive(double delta_deg_like)
    {
        start_relative_pan(std::abs(delta_deg_like));
    }

    void CameraService::start_relative_pan_negative(double delta_deg_like)
    {
        start_relative_pan(-std::abs(delta_deg_like));
    }

    void CameraService::start_relative_pan(double delta_deg_like)
    {
        relative_pan_target_delta_ = std::max(0.1, std::abs(delta_deg_like));
        relative_pan_direction_ = (delta_deg_like < 0.0) ? -1 : 1;
        relative_pan_active_ = true;
        relative_pan_start_azimuth_.reset();
        send_pan(relative_pan_direction_ > 0 ? std::abs(pan_speed_) : -std::abs(pan_speed_));
        RCLCPP_INFO(
            node_.get_logger(),
            "Start relative pan once: direction=%s delta=%.2f (azimuth units).",
            relative_pan_direction_ > 0 ? "right" : "left",
            relative_pan_target_delta_);
    }

    void CameraService::move_to_zero()
    {
        publish_absolute_xy(0, 0);
        RCLCPP_INFO(node_.get_logger(), "Move-to-zero command sent.");
    }

    void CameraService::publish_absolute_xy(int azimuth_x10, int elevation_x10)
    {
        const int azimuth = std::clamp(azimuth_x10, 0, 3600);
        const int elevation = std::clamp(elevation_x10, 0, 900);

        std::ostringstream body;
        body << "<PTZData><AbsoluteHigh><azimuth>"
             << azimuth
             << "</azimuth><elevation>"
             << elevation
             << "</elevation></AbsoluteHigh></PTZData>";
        publish_http("PUT", "/ISAPI/PTZCtrl/channels/1/absolute", "application/xml", body.str(), 3000);
    }

    void CameraService::publish_http(
        const std::string &method,
        const std::string &path,
        const std::string &content_type,
        const std::string &body,
        long timeout_ms)
    {
        std_msgs::msg::String msg;
        msg.data = build_http_payload(method, path, content_type, body, timeout_ms);
        control_pub_->publish(msg);
    }

    std::string CameraService::build_control_payload(int pan, int tilt, int zoom) const
    {
        std::ostringstream body;
        body << "<PTZData><pan>" << pan << "</pan><tilt>" << tilt << "</tilt><zoom>" << zoom << "</zoom></PTZData>";
        return build_http_payload("PUT", "/ISAPI/PTZCtrl/channels/1/continuous", "application/xml", body.str(), 3000);
    }

    std::string CameraService::build_http_payload(
        const std::string &method,
        const std::string &path,
        const std::string &content_type,
        const std::string &body,
        long timeout_ms) const
    {
        std::ostringstream json;
        json << "{"
             << "\"device_id\":\"" << escape_json(device_id_) << "\","
             << "\"method\":\"" << escape_json(method) << "\","
             << "\"path\":\"" << escape_json(path) << "\","
             << "\"content_type\":\"" << escape_json(content_type) << "\","
             << "\"body\":\"" << escape_json(body) << "\","
             << "\"timeout_ms\":" << timeout_ms
             << "}";
        return json.str();
    }

    std::optional<double> CameraService::extract_xml_numeric_value(const std::string &xml, const std::string &tag)
    {
        const std::string open = "<" + tag + ">";
        const std::string close = "</" + tag + ">";

        const size_t start = xml.find(open);
        if (start == std::string::npos)
        {
            return std::nullopt;
        }
        const size_t value_start = start + open.size();
        const size_t end = xml.find(close, value_start);
        if (end == std::string::npos || end <= value_start)
        {
            return std::nullopt;
        }

        const std::string value = xml.substr(value_start, end - value_start);
        char *parse_end = nullptr;
        const double number = std::strtod(value.c_str(), &parse_end);
        if (parse_end == value.c_str())
        {
            return std::nullopt;
        }
        return number;
    }

    void CameraService::on_control_result(const std_msgs::msg::String::SharedPtr msg)
    {
        if (msg == nullptr)
        {
            return;
        }

        // 透传并抽取 PTZStatus，给上层做“转到角度后停止”闭环。
        if (msg->data.find("<PTZStatus") != std::string::npos)
        {
            const auto azimuth = extract_xml_numeric_value(msg->data, "azimuth");
            const auto elevation = extract_xml_numeric_value(msg->data, "elevation");
            const auto absolute_zoom = extract_xml_numeric_value(msg->data, "absoluteZoom");

            if (azimuth.has_value() || elevation.has_value() || absolute_zoom.has_value())
            {
                std::ostringstream payload;
                payload << "{"
                        << "\"device_id\":\"" << escape_json(device_id_) << "\"";
                if (azimuth.has_value())
                {
                    payload << ",\"azimuth\":" << *azimuth;
                }
                if (elevation.has_value())
                {
                    payload << ",\"elevation\":" << *elevation;
                }
                if (absolute_zoom.has_value())
                {
                    payload << ",\"absolute_zoom\":" << *absolute_zoom;
                }
                payload << ",\"stamp_ns\":" << node_.get_clock()->now().nanoseconds()
                        << "}";

                std_msgs::msg::String status_msg;
                status_msg.data = payload.str();
                status_pub_->publish(status_msg);

                if (azimuth.has_value())
                {
                    update_relative_pan_once(*azimuth);
                }
            }
        }

        RCLCPP_INFO_THROTTLE(
            node_.get_logger(),
            *node_.get_clock(),
            2000,
            "camera_http_control_result: %s",
            msg->data.c_str());
    }

    void CameraService::update_relative_pan_once(double azimuth_now)
    {
        if (!relative_pan_active_)
        {
            return;
        }

        if (!relative_pan_start_azimuth_.has_value())
        {
            relative_pan_start_azimuth_ = azimuth_now;
            return;
        }

        const double start = *relative_pan_start_azimuth_;
        double delta = 0.0;
        if (relative_pan_direction_ > 0)
        {
            delta = azimuth_now - start;
            if (delta < 0.0)
            {
                delta += 360.0;
            }
        }
        else
        {
            delta = start - azimuth_now;
            if (delta < 0.0)
            {
                delta += 360.0;
            }
        }
        // 防止跨越多圈时丢失边界，约束在 [0, 360) 便于单圈比较。
        delta = std::fmod(delta, 360.0);

        if (delta >= relative_pan_target_delta_)
        {
            send_stop();
            relative_pan_active_ = false;
            RCLCPP_INFO(
                node_.get_logger(),
                "Relative pan complete: start=%.2f current=%.2f delta=%.2f target=%.2f",
                start,
                azimuth_now,
                delta,
                relative_pan_target_delta_);
        }
    }
} // namespace arm_control_package
