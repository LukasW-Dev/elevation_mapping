import os

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, TextSubstitution, PathJoinSubstitution, Command
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare
from launch_ros.descriptions import ComposableNode
from launch_ros.actions import ComposableNodeContainer
from launch_ros.parameter_descriptions import ParameterValue

def generate_launch_description():

    elevation_mapping_demos = FindPackageShare('elevation_mapping_demos')

    robot_desc = ParameterValue(
        Command(
            [
                'xacro ',
                PathJoinSubstitution([elevation_mapping_demos, 'urdf', 'leo_rover_depthcam.urdf.xacro'])
            ]
        )
    )

    robot_state_publisher = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        name="robot_state_publisher",
        output="both",
        parameters=[
            {"robot_description": robot_desc},
        ],
    )

    voxel_grid_node = Node(
        package='pcl_ros',
        executable='filter_voxel_grid_node',
        name="pointcloud_filter",
        remappings=[
            ('/input', '/left_laser/pandar'),
            ('/output', '/oak/points/filtered')
        ],
        parameters=[{
            'use_sim_time': True,
            'filter_limit_max': 20.0,
            'filter_limit_min': -20.0,
            'min_points_per_voxel': 2,
            'filter_field_name': 'x',
            'leaf_size': 0.1
        }]
    )

    elevation_mapping_node = Node(
        package='elevation_mapping',
        executable='elevation_mapping_node',
        name='elevation_mapping',
        parameters=[
            PathJoinSubstitution([elevation_mapping_demos, "config", "robots", "leo.yaml"]),
            PathJoinSubstitution([elevation_mapping_demos, "config", "postprocessing", "postprocessor_pipeline.yaml"]),
            {'use_sim_time': True}
        ]
    )

    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', PathJoinSubstitution([elevation_mapping_demos, 'rviz', 'leo_marsyard.rviz'])]
    )

    return LaunchDescription([
        robot_state_publisher,
        voxel_grid_node,
        elevation_mapping_node,
        rviz_node,
    ])