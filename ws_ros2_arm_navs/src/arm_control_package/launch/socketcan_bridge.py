from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    receiver_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(PathJoinSubstitution([
            FindPackageShare('ros2_socketcan'),
            'launch',
            'socket_can_receiver.launch.py',
        ])),
        launch_arguments={
            'interface': 'vcan0',
            'enable_can_fd': 'false',
            'interval_sec': '0.1',
            'from_can_bus_topic': '/from_can_bus',
        }.items(),
    )

    sender_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(PathJoinSubstitution([
            FindPackageShare('ros2_socketcan'),
            'launch',
            'socket_can_sender.launch.py',
        ])),
        launch_arguments={
            'interface': 'vcan0',
            'enable_can_fd': 'false',
            'timeout_sec': '0.1',
            'to_can_bus_topic': '/to_can_bus',
        }.items(),
    )

    return LaunchDescription([
        receiver_launch,
        sender_launch,
    ])