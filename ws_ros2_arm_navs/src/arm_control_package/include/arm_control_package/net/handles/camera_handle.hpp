#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "arm_common_package/msg_base.hpp"
#include "arm_common_package/arm_msg.hpp"

namespace arm_control_package
{

	class CameraHandle
	{
	using MessageEnvelope = arm_common_package::MessageEnvelope;
	using ArmMsg = arm_common_package::ArmMsg;
	using MessageCodec = arm_common_package::MessageCodec;

	public:
		std::vector<MessageEnvelope> handle_arm_status_query(
			const MessageEnvelope &request,
			const ArmMsg &status_snapshot,
			const std::string &server_identity) const
		{
			ArmMsg status = status_snapshot;
			if (request.body.contains("robot_id") && request.body["robot_id"].is_string())
			{
				status.robot_id = request.body["robot_id"].get<std::string>();
			}

			MessageEnvelope rsp;
			rsp.type = "payload";
			rsp.api = "ARM_STATUS_RSP";
			rsp.msg_id = MessageCodec::gen_msg_id();
			rsp.identity = server_identity;
			rsp.code = arm_common_package::comm_code::Ok;
			rsp.body = arm_common_package::to_json(status);
			rsp.ext = nlohmann::json::object();
			rsp.ts = MessageCodec::now_ms();
			return {std::move(rsp)};
		}

		std::vector<MessageEnvelope> handle_arm_log_query(
			const MessageEnvelope &request,
			const std::string &server_identity) const
		{
			MessageEnvelope rsp;
			rsp.type = "payload";
			rsp.api = "ARM_LOG_QUERY_RSP";
			rsp.msg_id = MessageCodec::gen_msg_id();
			rsp.identity = server_identity;
			rsp.code = arm_common_package::comm_code::Ok;
			rsp.body = nlohmann::json::object(
				{{"log_type", request.body.value("log_type", "SYSTEM")},
				 {"page_no", request.body.value("page_no", 1)},
				 {"page_size", request.body.value("page_size", 20)},
				 {"total_count", 0},
				 {"log_items", nlohmann::json::array()}});
			rsp.ext = nlohmann::json::object();
			rsp.ts = MessageCodec::now_ms();
			return {std::move(rsp)};
		}
	};

} // namespace arm_control_package
