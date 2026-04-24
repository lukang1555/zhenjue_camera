from launch import LaunchDescription
from launch.substitutions import PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='arm_control_package',
            executable='arm_control_node',
            output='both',
            # parameters=[PathJoinSubstitution([
            #     FindPackageShare('arm_camera_package')])
            # ],
        )
    ])