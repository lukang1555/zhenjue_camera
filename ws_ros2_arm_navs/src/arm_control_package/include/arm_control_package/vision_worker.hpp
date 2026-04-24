#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <memory>
#include <opencv2/opencv.hpp>
#include "arm_control_package/vision_target_solver.hpp"

namespace arm_control_package {

class VisionWorker {
public:
    VisionWorker();
    ~VisionWorker();

    void push_image(const cv::Mat& img);

private:
    void process_loop();

    std::shared_ptr<arm_camera_package::VisionTargetSolver> solver_;
    
    std::queue<cv::Mat> image_queue_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    bool stop_thread_;
    std::thread worker_thread_;
};

} // namespace arm_control_package