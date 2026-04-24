#pragma once

#include "arm_control_package/services/plc_service.hpp"
#include "arm_control_package/base/control_context.hpp"
#include "arm_control_package/services/api_service.hpp"

namespace arm_control_package
{
    /** 
     * ControlService类负责协调PlcService和ApiService之间的交互，以及维护ControlContext中的状态信息。
     * 它可以处理来自ApiService的任务，并通过PlcService发送给机械臂，同时更新ControlContext中的任务状态以供查询。
     */
    class ControlService
    {
    public:
        ControlService(std::shared_ptr<PlcService> plc_service, 
            std::shared_ptr<ApiService> api_service, 
            std::shared_ptr<ControlContext> control_context);

        ~ControlService() = default;
    
    private:
        std::shared_ptr<PlcService> plc_service_;
        std::shared_ptr<ApiService> api_service_;
        std::shared_ptr<ControlContext> control_context_;
    };
} // namespace arm_control_package
