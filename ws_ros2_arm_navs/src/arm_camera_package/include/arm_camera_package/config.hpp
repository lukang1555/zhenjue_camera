#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include <ament_index_cpp/get_package_share_directory.hpp>
#include <yaml-cpp/yaml.h>

#include "arm_common_package/config_loader.hpp"

namespace arm_camera_package
{
    struct CameraConfig
    {
        int device_id;
        std::string device_name;
        std::string stream_uri;
        std::string http_base_url;
        std::string username;
        std::string password;
        std::string stream_protocol{"rtsp"};
        bool prefer_deepstream_decoder{true};
        int connection_timeout_ms{8000};
        std::string publish_topic{""};
        std::string control_topic{""};
    };

    struct Config
    {
        std::vector<CameraConfig> devices;
    };

    inline std::string default_config_path()
    {
        return ament_index_cpp::get_package_share_directory("arm_camera_package") + "/config/config.yaml";
    }

    inline Config load_config(const std::string &file_path)
    {
        try
        {
            arm_common_package::YamlConfigLoader<Config> loader(file_path);
            return loader.get_config();
        }
        catch (const std::exception &ex)
        {
            return Config{};
        }
    }
}

namespace YAML
{
    template <>
    struct convert<arm_camera_package::CameraConfig>
    {
        static Node encode(const arm_camera_package::CameraConfig &cfg)
        {
            Node node;
            node["device_id"] = cfg.device_id;
            node["device_name"] = cfg.device_name;
            node["stream_uri"] = cfg.stream_uri;
            node["http_base_url"] = cfg.http_base_url;
            node["username"] = cfg.username;
            node["password"] = cfg.password;
            node["stream_protocol"] = cfg.stream_protocol;
            node["prefer_deepstream_decoder"] = cfg.prefer_deepstream_decoder;
            node["connection_timeout_ms"] = cfg.connection_timeout_ms;
            node["publish_topic"] = cfg.publish_topic;
            node["control_topic"] = cfg.control_topic;
            return node;
        }

        static bool decode(const Node &node, arm_camera_package::CameraConfig &cfg)
        {
            if (!node || !node.IsMap())
            {
                return false;
            }

            cfg.device_id = node["device_id"] ? node["device_id"].as<int>() : 0;
            cfg.device_name = node["device_name"] ? node["device_name"].as<std::string>() : std::string{};
            cfg.stream_uri = node["stream_uri"] ? node["stream_uri"].as<std::string>() : std::string{};
            if (cfg.stream_uri.empty() && node["rtsp_url"])
            {
                cfg.stream_uri = node["rtsp_url"].as<std::string>();
            }
            cfg.http_base_url = node["http_base_url"] ? node["http_base_url"].as<std::string>() : std::string{};
            cfg.username = node["username"] ? node["username"].as<std::string>() : std::string{};
            cfg.password = node["password"] ? node["password"].as<std::string>() : std::string{};
            cfg.stream_protocol = node["stream_protocol"] ? node["stream_protocol"].as<std::string>() : std::string{"rtsp"};
            if (cfg.stream_protocol.empty())
            {
                cfg.stream_protocol = "rtsp";
            }
            cfg.prefer_deepstream_decoder =
                node["prefer_deepstream_decoder"] ? node["prefer_deepstream_decoder"].as<bool>() : true;
            cfg.connection_timeout_ms =
                node["connection_timeout_ms"] ? node["connection_timeout_ms"].as<int>() : 8000;
            if (cfg.connection_timeout_ms < 1000)
            {
                cfg.connection_timeout_ms = 1000;
            }

            cfg.publish_topic = node["publish_topic"] ? node["publish_topic"].as<std::string>() : std::string{};
            cfg.control_topic = node["control_topic"] ? node["control_topic"].as<std::string>() : std::string{};
            return true;
        }
    };

    template <>
    struct convert<arm_camera_package::Config>
    {
        static Node encode(const arm_camera_package::Config &cfg)
        {
            Node node;
            node["devices"] = cfg.devices;
            return node;
        }

        static bool decode(const Node &node, arm_camera_package::Config &cfg)
        {
            cfg.devices.clear();
            if (!node)
            {
                return false;
            }

            if (!(node["devices"] && node["devices"].IsSequence()))
            {
                return false;
            }

            for (const auto &item : node["devices"])
            {
                if (!item.IsMap())
                {
                    continue;
                }
                cfg.devices.push_back(item.as<arm_camera_package::CameraConfig>());
            }

            return true;
        }
    };
} // namespace YAML