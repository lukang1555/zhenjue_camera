#include <memory>
#include <vector>

#include <rclcpp/rclcpp.hpp>

#include <opencv2/opencv.hpp>
#include "sensor_msgs/msg/point_cloud2.hpp"

#include "arm_common_package/arm_api.hpp"
#include "arm_control_package/config.hpp"
#include "arm_control_package/services/api_service.hpp"
#include "arm_control_package/services/plc_service.hpp"
#include "arm_control_package/services/camera_service.hpp"
#include "arm_ros_interfaces/msg/image_frame.hpp"
#include "arm_control_package/vision_worker.hpp"

namespace arm_control_package
{

using ImageFrameMsg = arm_ros_interfaces::msg::ImageFrame;

class ArmControlNode : public rclcpp::Node
{
public:
    ArmControlNode()
        : rclcpp::Node("arm_control_node")
    {
        const ArmConfig arm_config = load_arm_config();
        const auto can_server_options = to_can_server_options(arm_config.can_server);
        const auto api_server_options = to_api_server_options(arm_config.api_server);
        // camera_service_ = std::make_shared<CameraService>(*this);

        // 全局控制上下文，保存机械臂状态等信息，供各个服务使用
        control_context_ = std::make_shared<ControlContext>(arm_config.robot_id);

        // plc服务
        plc_service_ = std::make_shared<PlcService>(*this, can_server_options);
        plc_service_->register_on_change_handler([this](const ArmMsg status)
        {
            // 更新控制上下文中的机械臂状态，供后续查询使用
            this->control_context_->set_arm_status(status);                   
        });
        
        api_service_ = std::make_shared<ApiService>(api_server_options, plc_service_, control_context_);
        api_service_->register_handlers();
        vision_worker_ = std::make_shared<VisionWorker>();

        init_device_subscription({1, 2});

        // 定时发送plc消息测试通信链路
        // timer_ = create_wall_timer(std::chrono::milliseconds(1000), [this]() {
        //     auto cmd_type = arm_cmd_type_to_string(ArmCmdType::SWITCH);
        //     auto cmd_params = std::vector<int>{0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88};
        //     plc_service_->send_control_command(cmd_type, cmd_params, -1, nullptr, nullptr, nullptr);
        //     std::cout << "send can messages..." << std::endl;
        // });
    }

    ~ArmControlNode() override
    {
        if (camera_service_ != nullptr)
        {
            camera_service_->stop();
        }
    }

private:
    void init_device_subscription(std::vector<int> device_ids)
    {
        for (int device_id : device_ids)
        {
            std::string camera_topic_name = "/hk/camera_" + std::to_string(device_id);
            auto camera_subscription_ = this->create_subscription<ImageFrameMsg>(
                camera_topic_name, rclcpp::QoS(10),
                [this](const ImageFrameMsg::SharedPtr msg)
                {
                    this->on_camera_callback(std::move(*msg));
                }
            );
            camera_subscriptions_.push_back(camera_subscription_);

            std::string lidar_topic_name = "/hesai/lidar_points_" + std::to_string(device_id);
            auto lidar_subscription_ = this->create_subscription<sensor_msgs::msg::PointCloud2>(
                lidar_topic_name, rclcpp::QoS(10),
                [this, device_id](const sensor_msgs::msg::PointCloud2::SharedPtr msg)
                {
                    this->on_lidar_callback(device_id, std::move(msg));
                }
            );
            lidar_subscriptions_.push_back(lidar_subscription_);
            std::cout << "Subscribed to camera topic: " << camera_topic_name << ", lidar topic: " << lidar_topic_name << std::endl;
        }
    }

    void on_camera_callback(const ImageFrameMsg& frame)
    {
        auto start_0 = frame.pts_ns;
        auto start = frame.receive_ns;
        auto end = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
        auto latency_ms = (end - start) / 1e6;
        std::cout << "on_device_callback, latency: " << latency_ms << " ms, pts_ns: " << start_0 / 1e6 << " ms" << std::endl;

        if (frame.data_size > 0)
        {
            // cv::imwrite("/home/ws_ros2_arm_navs/src/frame.jpg", cv::Mat(frame.height, frame.width, CV_8UC3, const_cast<uint8_t *>(frame.data.data())));
            cv::Mat current_frame(frame.height, frame.width, CV_8UC3, const_cast<uint8_t *>(frame.data.data()));
            // vision_worker_->push_image(current_frame.clone());
            if (vision_worker_ != nullptr) {
                vision_worker_->push_image(current_frame.clone());
            } else {
                std::cout << "[ERROR] 视觉工人(vision_worker_)不存在！请检查构造函数是否实例化！" << std::endl;
            }
        }//改成把图片输出到其他文件中，然后开个线程对图片处理
    }

    void on_lidar_callback(const int device_id, const sensor_msgs::msg::PointCloud2::SharedPtr msg)
    {
        std::cout << "Received PointCloud2 message from device " << device_id << ", height: " << msg->height << ", width: " << msg->width << ", point_step: " << msg->point_step << std::endl;
    }

    static WebSocketServer::Options to_api_server_options(const ApiServerConfig &config)
    {
        WebSocketServer::Options options;
        options.bind_address = config.bind_address;
        options.port = config.port;
        options.identity = config.identity;
        options.max_message_bytes = config.max_message_bytes;
        options.duplicate_ttl_ms = config.duplicate_ttl_ms;
        options.high_priority_worker_threads = config.high_priority_worker_threads;
        options.normal_priority_worker_threads = config.normal_priority_worker_threads;
        return options;
    }

    static PlcService::Options to_can_server_options(const CanServerConfig &config)
    {
        PlcService::Options options;
        options.can_receive_topic = config.receive_topic;
        options.can_transmit_topic = config.transmit_topic;
        options.qos_depth = config.qos_depth;
        options.default_node_id = config.default_node_id;
        return options;
    }

    rclcpp::TimerBase::SharedPtr timer_;

    std::shared_ptr<ControlContext> control_context_;
    std::shared_ptr<PlcService> plc_service_;
    std::shared_ptr<ApiService> api_service_;
    std::shared_ptr<CameraService> camera_service_;
    std::shared_ptr<VisionWorker> vision_worker_;
    std::vector<rclcpp::Subscription<ImageFrameMsg>::SharedPtr> camera_subscriptions_;
    std::vector<rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr> lidar_subscriptions_;

};

} // namespace arm_control_package

int main(int argc, char **argv)
{
    try
    {
        rclcpp::init(argc, argv);
        auto node = std::make_shared<arm_control_package::ArmControlNode>();
        auto executor = std::make_shared<rclcpp::executors::MultiThreadedExecutor>(rclcpp::ExecutorOptions(), 8);
        executor->add_node(node);
        executor->spin();

        rclcpp::shutdown();
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in main: " << e.what() << std::endl;
        if (rclcpp::ok())
        {
            rclcpp::shutdown();
        }
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unknown exception in main" << std::endl;
        if (rclcpp::ok())
        {
            rclcpp::shutdown();
        }
        return 1;
    }
}
