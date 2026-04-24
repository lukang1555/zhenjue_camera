#pragma once

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "arm_common_package/msg_base.hpp"

namespace arm_control_package
{

	class LidarHandle
	{
	public:
		std::vector<arm_common_package::MessageEnvelope> handle_map_obstacle_query(
			const arm_common_package::MessageEnvelope &request,
			const std::string &server_identity) const
		{
			using arm_common_package::MessageCodec;
			using arm_common_package::MessageEnvelope;

			const bool include_point_cloud = request.body.value("include_point_cloud", false);

			MessageEnvelope rsp;
			rsp.type = "payload";
			rsp.api = "MAP_OBSTACLE_RSP";
			rsp.msg_id = MessageCodec::gen_msg_id();
			rsp.identity = server_identity;
			rsp.code = arm_common_package::comm_code::Ok;
			rsp.body = nlohmann::json::object(
				{{"frame_id", "MAP_SIM_001"},
				 {"map_time", "2026-04-12 11:03:02.00"},
				 {"coordinate_system", "LOCAL"},
				 {"targets", nlohmann::json::array()},
				 {"point_cloud_ref", include_point_cloud ? "pcd://session/20260412/frame001.pcd" : ""}});
			rsp.ext = nlohmann::json::object();
			rsp.ts = MessageCodec::now_ms();
			return {std::move(rsp)};
		}
	};

} // namespace arm_control_package
