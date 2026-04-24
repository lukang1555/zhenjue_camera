cmake_minimum_required(VERSION 3.8)
project(arm_control_package)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(OpenCV REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(std_msgs REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(can_msgs REQUIRED)
find_package(arm_common_package REQUIRED)
find_package(arm_ros_interfaces REQUIRED)
find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)

add_library(arm_control_camera_service
  src/services/camera_service.cpp
)

target_include_directories(arm_control_camera_service
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

ament_target_dependencies(arm_control_camera_service
  rclcpp
  std_msgs
)

add_executable(arm_control_node
  src/services/api_service.cpp
  src/services/plc_service.cpp
  src/services/camera_service.cpp
  src/arm_control_node.cpp
  src/vision_target_solver.cpp
  src/vision_worker.cpp
)

add_executable(camera_ptz_test_node
  src/camera_ptz_test_node.cpp
)

target_include_directories(arm_control_node
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_include_directories(camera_ptz_test_node
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(arm_control_node
  arm_control_camera_service
  Boost::system
  yaml-cpp
  ${OpenCV_LIBS}
)

target_link_libraries(camera_ptz_test_node
  arm_control_camera_service
  Boost::system
  yaml-cpp
)

ament_target_dependencies(arm_control_node
  rclcpp
  std_msgs
  ament_index_cpp
  can_msgs
  arm_common_package
  arm_ros_interfaces
  cv_bridge
  OpenCV
  yaml-cpp
)

ament_target_dependencies(camera_ptz_test_node
  rclcpp
  std_msgs
)

install(
  TARGETS arm_control_camera_service arm_control_node camera_ptz_test_node
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(
  DIRECTORY include/
  DESTINATION include
)

install(
  DIRECTORY config
  DESTINATION share/${PROJECT_NAME}
)

install(
  DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

install(
  DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(rclcpp std_msgs ament_index_cpp can_msgs arm_common_package)

ament_export_include_directories(include)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
