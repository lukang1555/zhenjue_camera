#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include <ament_index_cpp/get_package_share_directory.hpp>
#include <yaml-cpp/yaml.h>

namespace arm_camera_package
{
    struct CameraConfig
    {
        std::string device_id;
        std::vector<std::string> aliases;
        std::string stream_uri;
        std::string http_base_url;
        std::string username;
        std::string password;
        std::string stream_protocol{"rtsp"};
        bool prefer_deepstream_decoder{true};
        int connection_timeout_ms{8000};
    };

    struct config
    {
        std::vector<CameraConfig> devices;
    };

    inline std::string default_config_path()
    {
        return ament_index_cpp::get_package_share_directory("arm_camera_package") + "/config/config.yaml";
    }

    inline bool load_config_from_file(
        const std::string &file_path,
        config *cfg,
        std::string *error)
    {
        if (cfg == nullptr)
        {
            if (error != nullptr)
            {
                *error = "camera config output is null";
            }
            return false;
        }

        try
        {
            const YAML::Node root = YAML::LoadFile(file_path);
            *cfg = root.as<config>();
            return true;
        }
        catch (const std::exception &ex)
        {
            if (error != nullptr)
            {
                *error = ex.what();
            }
            return false;
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
            if (!cfg.aliases.empty())
            {
                if (cfg.aliases.size() == 1)
                {
                    node["alias"] = cfg.aliases.front();
                }
                node["aliases"] = cfg.aliases;
            }
            node["stream_uri"] = cfg.stream_uri;
            node["http_base_url"] = cfg.http_base_url;
            node["username"] = cfg.username;
            node["password"] = cfg.password;
            node["stream_protocol"] = cfg.stream_protocol;
            node["prefer_deepstream_decoder"] = cfg.prefer_deepstream_decoder;
            node["connection_timeout_ms"] = cfg.connection_timeout_ms;
            return node;
        }

        static bool decode(const Node &node, arm_camera_package::CameraConfig &cfg)
        {
            if (!node || !node.IsMap())
            {
                return false;
            }

            cfg.device_id = node["device_id"] ? node["device_id"].as<std::string>() : std::string{};
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

            cfg.aliases.clear();
            if (node["alias"])
            {
                cfg.aliases.push_back(node["alias"].as<std::string>());
            }
            if (node["aliases"] && node["aliases"].IsSequence())
            {
                for (const auto &alias_node : node["aliases"])
                {
                    cfg.aliases.push_back(alias_node.as<std::string>());
                }
            }

            cfg.aliases.erase(
                std::remove_if(
                    cfg.aliases.begin(),
                    cfg.aliases.end(),
                    [](const std::string &alias)
                    { return alias.empty(); }),
                cfg.aliases.end());

            if (cfg.device_id.empty())
            {
                if (!cfg.aliases.empty())
                {
                    cfg.device_id = cfg.aliases.front();
                }
                else
                {
                    return false;
                }
            }

            return true;
        }
    };

    template <>
    struct convert<arm_camera_package::config>
    {
        static Node encode(const arm_camera_package::config &cfg)
        {
            Node node;
            node["devices"] = cfg.devices;
            return node;
        }

        static bool decode(const Node &node, arm_camera_package::config &cfg)
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