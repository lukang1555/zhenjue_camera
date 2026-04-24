cmake_minimum_required(VERSION 3.8)
project(arm_camera_package)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Enable debug symbols
set(CMAKE_BUILD_TYPE Debug)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(arm_common_package REQUIRED)
find_package(arm_ros_interfaces REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)

pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-app-1.0)

add_library(arm_camera_core
  src/camera_node_base.cpp
  src/camera_stream_base.cpp
  src/http_camera_controller.cpp
)

target_include_directories(arm_camera_core
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${GSTREAMER_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

target_compile_options(arm_camera_core PRIVATE ${GSTREAMER_CFLAGS_OTHER})
target_link_directories(arm_camera_core PRIVATE ${GSTREAMER_LIBRARY_DIRS})

target_link_libraries(arm_camera_core
  ${GSTREAMER_LIBRARIES}
  CURL::libcurl
  yaml-cpp
  ${OpenCV_LIBS}
)

ament_target_dependencies(arm_camera_core
  ament_index_cpp
  rclcpp
  std_msgs
  arm_common_package
  arm_ros_interfaces
)

add_executable(arm_camera_node
  src/arm_camera_node.cpp
)

target_link_libraries(arm_camera_node
  arm_camera_core
)

ament_target_dependencies(arm_camera_node
  ament_index_cpp
  rclcpp
  std_msgs
)

install(
  TARGETS arm_camera_core arm_camera_node
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include
)

install(DIRECTORY config
  DESTINATION share/${PROJECT_NAME}
)

ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(ament_index_cpp rclcpp std_msgs)

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
