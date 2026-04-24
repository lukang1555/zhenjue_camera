#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include <ament_index_cpp/get_package_share_directory.hpp>
#include <yaml-cpp/yaml.h>

#include "arm_common_package/config_loader.hpp"

namespace arm_control_package
{

struct ApiServerConfig
{
    std::string bind_address{"0.0.0.0"};
    uint16_t port{9002};
    std::string identity{"ROBOT-CTRL-0001"};
    size_t max_message_bytes{262144};
    int64_t duplicate_ttl_ms{60000};
    size_t high_priority_worker_threads{2};
    size_t normal_priority_worker_threads{4};
};

struct CanServerConfig
{
    std::string receive_topic{"/from_can_bus"};
    std::string transmit_topic{"/to_can_bus"};
    int qos_depth{10};
    uint8_t default_node_id{0};
};

struct ArmConfig
{
    std::string robot_id{"RB001"};
    CanServerConfig can_server;
    ApiServerConfig api_server;
    std::vector<std::string> client_whitelist;
};

inline std::string default_arm_config_path()
{
    return ament_index_cpp::get_package_share_directory("arm_control_package") + "/config/arm_config.yaml";
}

inline ArmConfig load_arm_config(const std::string &file_path = default_arm_config_path())
{
    try
    {
        arm_common_package::YamlConfigLoader<ArmConfig> loader(file_path);
        return loader.get_config();
    }
    catch (const std::exception &ex)
    {
        return ArmConfig{};
    }
}

} // namespace arm_control_package

namespace YAML
{

template <>
struct convert<arm_control_package::ApiServerConfig>
{
    static Node encode(const arm_control_package::ApiServerConfig &cfg)
    {
        Node node;
        node["bind_address"] = cfg.bind_address;
        node["port"] = cfg.port;
        node["identity"] = cfg.identity;
        node["max_message_bytes"] = cfg.max_message_bytes;
        node["duplicate_ttl_ms"] = cfg.duplicate_ttl_ms;
        node["high_priority_worker_threads"] = cfg.high_priority_worker_threads;
        node["normal_priority_worker_threads"] = cfg.normal_priority_worker_threads;
        return node;
    }

    static bool decode(const Node &node, arm_control_package::ApiServerConfig &cfg)
    {
        if (!node || !node.IsMap())
        {
            return false;
        }

        cfg.bind_address = node["bind_address"] ? node["bind_address"].as<std::string>() : "0.0.0.0";

        const int port = node["port"] ? node["port"].as<int>() : 9002;
        cfg.port = static_cast<uint16_t>(std::clamp(port, 1, 65535));

        cfg.identity = node["identity"] ? node["identity"].as<std::string>() : "ROBOT-CTRL-0001";

        cfg.max_message_bytes = node["max_message_bytes"] ? node["max_message_bytes"].as<size_t>() : 262144;
        cfg.duplicate_ttl_ms = node["duplicate_ttl_ms"] ? node["duplicate_ttl_ms"].as<int64_t>() : 60000;

        const int high_threads = node["high_priority_worker_threads"]
                                     ? node["high_priority_worker_threads"].as<int>()
                                     : 2;
        const int normal_threads = node["normal_priority_worker_threads"]
                                       ? node["normal_priority_worker_threads"].as<int>()
                                       : 4;

        cfg.high_priority_worker_threads = static_cast<size_t>(std::max(1, high_threads));
        cfg.normal_priority_worker_threads = static_cast<size_t>(std::max(1, normal_threads));
        return true;
    }
};

template <>
struct convert<arm_control_package::CanServerConfig>
{
    static Node encode(const arm_control_package::CanServerConfig &cfg)
    {
        Node node;
        node["receive_topic"] = cfg.receive_topic;
        node["transmit_topic"] = cfg.transmit_topic;
        node["qos_depth"] = cfg.qos_depth;
        node["default_node_id"] = static_cast<int>(cfg.default_node_id);
        return node;
    }

    static bool decode(const Node &node, arm_control_package::CanServerConfig &cfg)
    {
        if (!node || !node.IsMap())
        {
            return false;
        }

        const YAML::Node receive_topic_node = node["receive_topic"];
        const YAML::Node transmit_topic_node = node["transmit_topic"];
        cfg.receive_topic = receive_topic_node ? receive_topic_node.as<std::string>() : "/ros2can/rx";
        cfg.transmit_topic = transmit_topic_node ? transmit_topic_node.as<std::string>() : "/ros2can/tx";

        const int qos_depth = node["qos_depth"] ? node["qos_depth"].as<int>() : 100;
        cfg.qos_depth = std::max(1, qos_depth);

        const int node_id = node["default_node_id"] ? node["default_node_id"].as<int>() : 1;
        cfg.default_node_id = static_cast<uint8_t>(std::clamp(node_id, 0, 127));
        return true;
    }
};

template <>
struct convert<arm_control_package::ArmConfig>
{
    static Node encode(const arm_control_package::ArmConfig &cfg)
    {
        Node node;
        node["robot_id"] = cfg.robot_id;
        node["can_server"] = cfg.can_server;
        node["api_server"] = cfg.api_server;
        node["client_whitelist"] = cfg.client_whitelist;
        return node;
    }

    static bool decode(const Node &node, arm_control_package::ArmConfig &cfg)
    {
        if (!node || !node.IsMap())
        {
            return false;
        }

        cfg.robot_id = node["robot_id"] ? node["robot_id"].as<std::string>() : "RB001";

        const YAML::Node can_node = node["can_server"] && node["can_server"].IsMap()
                        ? node["can_server"]
                        : YAML::Node();
        if (can_node)
        {
            cfg.can_server = can_node.as<arm_control_package::CanServerConfig>();
        }
        else
        {
            cfg.can_server = arm_control_package::CanServerConfig{};
        }

        const YAML::Node api_node = node["api_server"] && node["api_server"].IsMap()
                        ? node["api_server"]
                        : YAML::Node();
        if (api_node)
        {
            cfg.api_server = api_node.as<arm_control_package::ApiServerConfig>();
        }
        else
        {
            cfg.api_server = arm_control_package::ApiServerConfig{};
        }

        cfg.client_whitelist.clear();
        const YAML::Node whitelist_node = node["client_whitelist"];
        if (whitelist_node && whitelist_node.IsSequence())
        {
            for (const auto &item : whitelist_node)
            {
                if (item.IsScalar())
                {
                    cfg.client_whitelist.push_back(item.as<std::string>());
                }
            }
        }

        return true;
    }
};

} // namespace YAML
