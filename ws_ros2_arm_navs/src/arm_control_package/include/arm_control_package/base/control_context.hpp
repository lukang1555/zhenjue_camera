#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "arm_control_package/base/global.hpp"

namespace arm_control_package
{
    class ControlContext
    {
    public:
        void set_arm_status(const ArmMsg &status)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            arm_status_ = status;
        }

        ArmMsg get_arm_status() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return arm_status_;
        }

        void set_task_type(TaskType task_type)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            task_type_ = task_type;
        }

        TaskType get_task_type() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return task_type_;
        }

        void set_job_status(JobStatus job_status)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            job_status_ = job_status;
        }

        JobStatus get_job_status() const
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return job_status_;
        }

        ControlContext(std::string robot_id)
            : robot_id_(std::move(robot_id))
        {
        };

        ~ControlContext() = default;
    private:
        std::string robot_id_;

        mutable std::mutex mutex_;
        ArmMsg arm_status_;
        TaskType task_type_{TaskType::None};
        JobStatus job_status_{JobStatus::None};
    };
}