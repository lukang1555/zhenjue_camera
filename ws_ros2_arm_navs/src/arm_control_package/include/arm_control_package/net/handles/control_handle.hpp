#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "arm_common_package/msg_base.hpp"
#include "arm_control_package/services/can_protocol_service.hpp"

namespace arm_control_package
{

	class ControlHandle
	{
	public:
		std::vector<arm_common_package::MessageEnvelope> handle_arm_auto_job_req(
			const arm_common_package::MessageEnvelope &request,
			const std::string &server_identity) const
		{
			using arm_common_package::MessageCodec;
			using arm_common_package::MessageEnvelope;

			MessageEnvelope rsp;
			rsp.type = "payload";
			rsp.api = "ARM_AUTO_JOB_RSP";
			rsp.msg_id = MessageCodec::gen_msg_id();
			rsp.identity = server_identity;
			rsp.code = arm_common_package::comm_code::Ok;
			rsp.body = nlohmann::json::object(
				{{"task_id", request.body.value("task_id", "")},
				 {"task_type", request.body.value("task_type", "")},
				 {"job_status", "ACCEPTED"},
				 {"description", "auto job request accepted"}});
			rsp.ext = nlohmann::json::object();
			rsp.ts = MessageCodec::now_ms();
			return {std::move(rsp)};
		}

		std::vector<arm_common_package::MessageEnvelope> handle_arm_control_cmd(
			const arm_common_package::MessageEnvelope &request,
			const std::string &server_identity,
			std::shared_ptr<CanProtocolService> can_service) const
		{
			using arm_common_package::MessageCodec;
			using arm_common_package::MessageEnvelope;
			using arm_common_package::comm_code::BadFormat;
			using arm_common_package::comm_code::Ok;

			MessageEnvelope rsp;
			rsp.type = "payload";
			rsp.api = "ARM_CONTROL_CMD_RSP";
			rsp.msg_id = MessageCodec::gen_msg_id();
			rsp.identity = server_identity;
			rsp.ext = nlohmann::json::object();
			rsp.ts = MessageCodec::now_ms();

			const std::string cmd_id = request.body.value("cmd_id", "");
			const std::string cmd_type = request.body.value("cmd_type", "");
			const int node_id = request.body.value("node_id", 0);

			std::vector<int> command_params;
			if (request.body.contains("cmd_params") && request.body["cmd_params"].is_array())
			{
				for (const auto &item : request.body["cmd_params"])
				{
					command_params.push_back(item.is_number_integer() ? item.get<int>() : 0);
				}
			}

			RosCanFrame frame;
			nlohmann::json decoded;
			std::string error;
			if (!can_service->send_control_command(cmd_type, command_params, node_id, &frame, &decoded, &error))
			{
				rsp.code = BadFormat;
				rsp.body = nlohmann::json::object(
					{{"cmd_id", cmd_id},
					 {"cmd_type", cmd_type},
					 {"error", error}});
				return {std::move(rsp)};
			}

			rsp.code = Ok;
			rsp.body = nlohmann::json::object(
				{{"cmd_id", cmd_id},
				 {"cmd_type", cmd_type},
				 {"node_id", node_id},
				 {"can_backend", can_service->backend_name()},
				 {"can_frame", to_json(frame)},
				 {"decoded", decoded}});
			return {std::move(rsp)};
		}
	};

} // namespace arm_control_package
