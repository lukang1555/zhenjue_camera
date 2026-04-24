#include "arm_control_package/vision_worker.hpp"
#include <iostream>
#include <yaml-cpp/yaml.h>

namespace arm_control_package {

VisionWorker::VisionWorker() : stop_thread_(false) {
    
    arm_camera_package::VisionConfig v_config;

    try {
        std::string yaml_path = "/home/ws_ros2_arm_navs/src/arm_camera_package/config/config.yaml";
        YAML::Node config = YAML::LoadFile(yaml_path);

        YAML::Node vision_node = config["vision_parameters"];
        
        v_config.marker_size_m = vision_node["marker_size_m"].as<double>();
        v_config.camera_matrix = vision_node["camera_matrix"].as<std::vector<double>>();
        v_config.dist_coeffs   = vision_node["dist_coeffs"].as<std::vector<double>>();
        v_config.t_cam2gripper = vision_node["t_cam2gripper"].as<std::vector<double>>();
        
        std::cout << "\033[1;32m[VisionWorker] 成功从 arm_camera_package 读取 YAML 视觉参数！\033[0m" << std::endl;
    }
    catch (const YAML::Exception& e) {
        std::cerr << "\033[1;31m[ERROR] 无法读取视觉 YAML 配置文件，将使用默认硬编码参数！报错原因: " << e.what() << "\033[0m" << std::endl;
        
        v_config.marker_size_m = 0.1; 
        v_config.camera_matrix = {1000.0, 0.0, 640.0, 0.0, 1000.0, 360.0, 0.0, 0.0, 1.0}; 
        v_config.dist_coeffs = {0.0, 0.0, 0.0, 0.0, 0.0}; 
        v_config.t_cam2gripper = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0}; 
    }
    solver_ = std::make_shared<arm_camera_package::VisionTargetSolver>(v_config);

    worker_thread_ = std::thread(&VisionWorker::process_loop, this);
    std::cout << "[VisionWorker] 后台视觉处理独立线程已启动！" << std::endl;
}

VisionWorker::~VisionWorker() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        stop_thread_ = true;
    }
    condition_.notify_all();
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

void VisionWorker::push_image(const cv::Mat& img) {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    
    if (image_queue_.size() > 2) {
        image_queue_.pop(); 
    }
    image_queue_.push(img);
    
    lock.unlock();
    condition_.notify_one(); 
}

void VisionWorker::process_loop() {
    while (true) {
        cv::Mat frame_to_process;
        {
            // 等待队列里有图，或者收到程序退出的信号
            std::unique_lock<std::mutex> lock(queue_mutex_);
            condition_.wait(lock, [this]() { return !image_queue_.empty() || stop_thread_; });

            if (stop_thread_ && image_queue_.empty()) {
                break; // 退出线程
            }

            frame_to_process = image_queue_.front();
            image_queue_.pop();
        }

        std::vector<double> dummy_plc_data; 
        cv::Point3f target_pos;
        
        bool is_detected = solver_->processFrame(frame_to_process, dummy_plc_data, target_pos);

        if (is_detected) {
            std::cout << "[VisionWorker] 🎯 抓取目标锁定！X: " << target_pos.x 
                      << ", Y: " << target_pos.y << ", Z: " << target_pos.z << std::endl;
        }

        // 处理并画好坐标轴后，保存图片
        cv::imwrite("/home/ws_ros2_arm_navs/src/frame_processed.jpg", frame_to_process);
    }
}

} 