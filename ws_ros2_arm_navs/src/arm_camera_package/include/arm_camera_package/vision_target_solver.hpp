#pragma once
#include <opencv2/opencv.hpp>
#include <opencv2/aruco.hpp>
#include <vector>

namespace arm_camera_package {

struct VisionConfig {
    double marker_size_m;
    std::vector<double> camera_matrix;
    std::vector<double> dist_coeffs;
    std::vector<double> t_cam2gripper;
};

class VisionTargetSolver {
public:
    explicit VisionTargetSolver(const VisionConfig& config);
    bool processFrame(const cv::Mat& img, const std::vector<double>& current_plc_data, cv::Point3f& out_final_pos);

private:

    cv::Mat camera_matrix_, dist_coeffs_, T_cam2gripper_cv_;
    cv::aruco::Dictionary dictionary_;
    cv::aruco::DetectorParameters detector_params_;
    std::vector<cv::Point3f> obj_points_;

    cv::Mat make_orthogonal(const cv::Mat& R);
};

} // namespace arm_camera_package