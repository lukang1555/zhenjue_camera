#include "arm_control_package/vision_target_solver.hpp"
#include <iostream>

namespace arm_camera_package {

VisionTargetSolver::VisionTargetSolver(const VisionConfig& config) {
    camera_matrix_ = cv::Mat(3, 3, CV_64F, (void*)config.camera_matrix.data()).clone();
    dist_coeffs_ = cv::Mat(1, 5, CV_64F, (void*)config.dist_coeffs.data()).clone();
    T_cam2gripper_cv_ = cv::Mat(4, 4, CV_64F, (void*)config.t_cam2gripper.data()).clone();
        
    dictionary_ = cv::aruco::getPredefinedDictionary(cv::aruco::DICT_5X5_250);
    detector_params_ = cv::aruco::DetectorParameters();
    
    double half_size = config.marker_size_m / 2.0;
    obj_points_ = {
        cv::Point3f(-half_size,  half_size, 0), 
        cv::Point3f( half_size,  half_size, 0),
        cv::Point3f( half_size, -half_size, 0), 
        cv::Point3f(-half_size, -half_size, 0)
    };
}

cv::Mat VisionTargetSolver::make_orthogonal(const cv::Mat& R) {
    cv::Mat w, u, vt;
    cv::SVD::compute(R, w, u, vt);
    cv::Mat R_ortho = u * vt;
    if (cv::determinant(R_ortho) < 0) {
        u.col(2) *= -1;
        R_ortho = u * vt;
    }
    return R_ortho;
}

bool VisionTargetSolver::processFrame(const cv::Mat& img, const std::vector<double>& current_plc_data, cv::Point3f& out_final_pos) {
    if (img.empty()) return false;

    cv::Mat gray;
    cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);

    std::vector<int> ids;
    std::vector<std::vector<cv::Point2f>> corners;
    cv::Ptr<cv::aruco::Dictionary> dict_ptr = cv::makePtr<cv::aruco::Dictionary>(dictionary_);
    cv::Ptr<cv::aruco::DetectorParameters> params_ptr = cv::makePtr<cv::aruco::DetectorParameters>(detector_params_);

    cv::aruco::detectMarkers(gray, dict_ptr, corners, ids, params_ptr);

    if (ids.empty()) return false; 

    cv::Mat rvec, tvec;
    bool success = cv::solvePnP(obj_points_, corners[0], camera_matrix_, dist_coeffs_, rvec, tvec);
    if (!success) return false;

    cv::Mat R_cam;
    cv::Rodrigues(rvec, R_cam); 


    cv::Mat T_target2cam = cv::Mat::eye(4, 4, CV_64F);
    R_cam.copyTo(T_target2cam(cv::Rect(0, 0, 3, 3)));
    tvec.copyTo(T_target2cam(cv::Rect(3, 0, 1, 3)));

    std::cout << "[Step 1: Camera] 标靶相对于【相机】: " 
              << "X=" << tvec.at<double>(0) << ", Y=" << tvec.at<double>(1) << ", Z=" << tvec.at<double>(2) << std::endl;

    cv::Mat T_target2gripper = T_cam2gripper_cv_ * T_target2cam;

    if (current_plc_data.size() < 12) {
        out_final_pos.x = T_target2gripper.at<double>(0, 3);
        out_final_pos.y = T_target2gripper.at<double>(1, 3);
        out_final_pos.z = T_target2gripper.at<double>(2, 3);
        
        std::cout << "\033[1;33m[Step 2: Gripper] PLC无数据，当前输出为相对于【夹爪】: " 
                  << "X=" << out_final_pos.x << ", Y=" << out_final_pos.y << ", Z=" << out_final_pos.z << "\033[0m" << std::endl;
        return true; 
    }

    cv::Mat mat_plc(3, 4, CV_64F, (void*)current_plc_data.data());
    cv::Mat R_gripper_raw = mat_plc(cv::Rect(0, 0, 3, 3));
    cv::Mat t_gripper = mat_plc(cv::Rect(3, 0, 1, 3));

    cv::Mat R_gripper_clean = make_orthogonal(R_gripper_raw);

    cv::Mat T_gripper2base = cv::Mat::eye(4, 4, CV_64F);
    R_gripper_clean.copyTo(T_gripper2base(cv::Rect(0, 0, 3, 3)));
    t_gripper.copyTo(T_gripper2base(cv::Rect(3, 0, 1, 3)));

    cv::Mat T_target2base = T_gripper2base * T_target2gripper;

    out_final_pos.x = T_target2base.at<double>(0, 3); 
    out_final_pos.y = T_target2base.at<double>(1, 3); 
    out_final_pos.z = T_target2base.at<double>(2, 3); 

    std::cout << "\033[1;32m[Step 3: Base] 标靶相对于【基座】(最终控制坐标): " 
              << "X=" << out_final_pos.x << ", Y=" << out_final_pos.y << ", Z=" << out_final_pos.z << "\033[0m" << std::endl;

    return true;
}
} // namespace arm_camera_package