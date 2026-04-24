#include "arm_control_package/services/control_service.hpp"

namespace arm_control_package
{
    ControlService::ControlService(std::shared_ptr<PlcService> plc_service, 
        std::shared_ptr<ApiService> api_service, 
        std::shared_ptr<ControlContext> control_context)
        : plc_service_(std::move(plc_service)),
          api_service_(std::move(api_service)),
          control_context_(std::move(control_context))
    {
        // 注册ApiService的任务处理回调，例如：
        api_service_->register_task_handler([this](const MessageEnvelope &request)
        {
            // 解析请求，执行任务，更新ControlContext中的状态等
            // 例如：
            auto task_id = request.body.value("task_id", std::string{});
            auto task_type = request.body.value("task_type", std::string{});
            // auto task_params = request.body.value("task_params", nlohmann::json::object());
            
            // 执行任务，例如调用plc_service_发送控制命令
            
            // 更新ControlContext中的任务状态，例如control_context_->set_task_type(...);
            
            // 构造响应消息并返回
            MessageEnvelope rsp;
            rsp.type = "payload";
            rsp.api = arm_common_package::api_to_string(ArmApi::ArmAutoJobRsp);
            rsp.msg_id = MessageCodec::gen_msg_id();
            rsp.identity = api_service_->get_identity();
            rsp.code = arm_common_package::comm_code::Ok;
            rsp.body = nlohmann::json::object(
                    {{"task_id", task_id.c_str()},
                     {"task_type", task_type.c_str()},
                     {"job_status", job_status_to_string(JobStatus::RUNNING)},
                     {"description", "ok"}});
            rsp.ext = nlohmann::json::object();
            rsp.ts = MessageCodec::now_ms();
            return std::vector<MessageEnvelope>{std::move(rsp)};
        });
    }
}