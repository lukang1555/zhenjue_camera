#pragma once

#include <can_msgs/msg/frame.hpp>

#include "arm_common_package/arm_api.hpp"
#include "arm_common_package/arm_msg.hpp"
#include "arm_common_package/msg_base.hpp"

namespace arm_control_package
{
    using ArmApi = arm_common_package::ArmApi;
    using MessageEnvelope = arm_common_package::MessageEnvelope;
	using ArmMsg = arm_common_package::ArmMsg;
	using MessageCodec = arm_common_package::MessageCodec;
    using CanRosMsg = can_msgs::msg::Frame;
    using RosCanFrame = arm_common_package::RosCanFrame;


    enum class JobFlag
    {
        None = 0,
        START = 1,
        STOP = 2,
    };


    enum class JobStatus
    {
        None = 0,
        ACCEPTED = 1,
        RUNNING = 2,
        INTERRUPTED = 3,
        DONE = 4,
        FAILED = 5
    };


    enum class TaskType
    {
        None = 0,
        PICK = 1,
        LAND = 2,
        RESET = 3,
    };


    enum class ArmCmdType
    {
        None = 0,
        SWITCH = 1,
        STATUS = 2,
        ANALOG = 3,
        TELEMETRY_300 = 4,
        TELEMETRY_400 = 5,
    };

    inline std::string job_status_to_string(JobStatus status)
    {
        switch (status)
        {
        case JobStatus::None:
            return "None";
        case JobStatus::ACCEPTED:
            return "ACCEPTED";
        case JobStatus::RUNNING:
            return "RUNNING";
        case JobStatus::INTERRUPTED:
            return "INTERRUPTED";
        case JobStatus::DONE:
            return "DONE";
        case JobStatus::FAILED:
            return "FAILED";
        default:
            return "UNKNOWN";
        }
    }



    inline ArmCmdType parse_arm_cmd_type(const std::string &cmd_type)
    {
        if (cmd_type == "switch") return ArmCmdType::SWITCH;
        if (cmd_type == "status") return ArmCmdType::STATUS;
        if (cmd_type == "analog") return ArmCmdType::ANALOG;
        if (cmd_type == "telemetry_300") return ArmCmdType::TELEMETRY_300;
        if (cmd_type == "telemetry_400") return ArmCmdType::TELEMETRY_400;
        return ArmCmdType::None;
    }


    inline std::string arm_cmd_type_to_string(ArmCmdType cmd_type)
    {
        switch (cmd_type)
        {
        case ArmCmdType::SWITCH:
            return "switch";
        case ArmCmdType::STATUS:
            return "status";
        case ArmCmdType::ANALOG:
            return "analog";
        case ArmCmdType::TELEMETRY_300:
            return "telemetry_300";
        case ArmCmdType::TELEMETRY_400:
            return "telemetry_400";
        default:
            return "none";
        }
    }

    
    inline int get_arm_frame_id(ArmCmdType cmd_type)
    {
        switch (cmd_type)
        {
        case ArmCmdType::SWITCH:
            return arm_common_package::CanBaseAiToPlcSwitch;
        case ArmCmdType::ANALOG:
            return arm_common_package::CanBaseAiToPlcAnalog;
        case ArmCmdType::STATUS:
            return arm_common_package::CanBasePlcToAiStatus;
        case ArmCmdType::TELEMETRY_300:
            return arm_common_package::CanBasePlcToAiTelemetry300;
        case ArmCmdType::TELEMETRY_400:
            return arm_common_package::CanBasePlcToAiTelemetry400;
        default:
            return 0;
        }
    }
}