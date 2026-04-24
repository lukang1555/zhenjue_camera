#pragma once

#include <string>
#include <vector>

namespace arm_common_package
{

    enum class ArmApi
    {
        ArmAutoJobReq,
        ArmAutoJobRsp,
        ArmStatusQuery,
        ArmStatusRsp,
        ArmControlCmd,
        ArmControlCmdRsp,
        MapObstacleQuery,
        MapObstacleRsp,
        ArmSafetyConfig,
        ArmLogQuery,
        ArmLogQueryRsp,
        ArmControlAuthReq,
        ArmControlAuthRsp,
    };

    inline const char *api_to_string(ArmApi api)
    {
        switch (api)
        {
        case ArmApi::ArmAutoJobReq:
            return "ARM_AUTO_JOB_REQ";
        case ArmApi::ArmAutoJobRsp:
            return "ARM_AUTO_JOB_RSP";
        case ArmApi::ArmStatusQuery:
            return "ARM_STATUS_QUERY";
        case ArmApi::ArmStatusRsp:
            return "ARM_STATUS_RSP";
        case ArmApi::ArmControlCmd:
            return "ARM_CONTROL_CMD";
        case ArmApi::ArmControlCmdRsp:
            return "ARM_CONTROL_CMD_RSP";
        case ArmApi::MapObstacleQuery:
            return "MAP_OBSTACLE_QUERY";
        case ArmApi::MapObstacleRsp:
            return "MAP_OBSTACLE_RSP";
        case ArmApi::ArmSafetyConfig:
            return "ARM_SAFETY_CONFIG";
        case ArmApi::ArmLogQuery:
            return "ARM_LOG_QUERY";
        case ArmApi::ArmLogQueryRsp:
            return "ARM_LOG_QUERY_RSP";
        case ArmApi::ArmControlAuthReq:
            return "ARM_CONTROL_AUTH_REQ";
        case ArmApi::ArmControlAuthRsp:
            return "ARM_CONTROL_AUTH_RSP";
        }
        return "";
    }

    inline const std::vector<std::string> &default_supported_api_names()
    {
        static const std::vector<std::string> apiNames = {
            api_to_string(ArmApi::ArmAutoJobReq),
            api_to_string(ArmApi::ArmAutoJobRsp),
            api_to_string(ArmApi::ArmStatusQuery),
            api_to_string(ArmApi::ArmStatusRsp),
            api_to_string(ArmApi::ArmControlCmd),
            api_to_string(ArmApi::ArmControlCmdRsp),
            api_to_string(ArmApi::MapObstacleQuery),
            api_to_string(ArmApi::MapObstacleRsp),
            api_to_string(ArmApi::ArmSafetyConfig),
            api_to_string(ArmApi::ArmLogQuery),
            api_to_string(ArmApi::ArmLogQueryRsp),
            api_to_string(ArmApi::ArmControlAuthReq),
            api_to_string(ArmApi::ArmControlAuthRsp),
        };
        return apiNames;
    }

} // namespace arm_common_package
