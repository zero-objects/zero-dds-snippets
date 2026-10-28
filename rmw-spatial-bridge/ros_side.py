"""The ROS 2 side of the ros2.spatial_bridge example.

    python3 ros_side.py sub SECONDS --config bridge-dds-to-ros.yaml
    python3 ros_side.py pub SECONDS --config bridge-ros-to-dds.yaml

The bridge's writers offer TRANSIENT_LOCAL with KEEP_LAST 10, and both
readers of this example ask for it: a pose the bridge forwarded before its
writer had matched the reader on the other side reaches that reader from
the writer's history.

Run with RMW_IMPLEMENTATION=rmw_zerodds_cpp: the bridge runs inside this
process's RMW, configured by the file after --config (the RMW reads
--config from the process's own arguments, rclpy leaves it alone).
"""
import sys
import time

import rclpy
from geometry_msgs.msg import PoseStamped
from rclpy.qos import DurabilityPolicy, QoSProfile, ReliabilityPolicy

# What the bridge's ROS-side writer offers, asked for on this side: a
# TRANSIENT_LOCAL subscription that matches late still gets the poses the
# bridge forwarded before the match, up to the writer's KEEP_LAST 10.
LATE_JOINER = QoSProfile(
    depth=10,
    reliability=ReliabilityPolicy.RELIABLE,
    durability=DurabilityPolicy.TRANSIENT_LOCAL,
)


def main():
    mode, seconds = sys.argv[1], float(sys.argv[2])
    rclpy.init()
    node = rclpy.create_node("spatial_bridge_example")
    count = 0
    end = time.time() + seconds
    if mode == "sub":
        def on_pose(msg):
            nonlocal count
            count += 1
            p = msg.pose.position
            print(f"ros_side: PoseStamped frame={msg.header.frame_id} "
                  f"position=({p.x}, {p.y}, {p.z})", flush=True)

        node.create_subscription(PoseStamped, "/robot1/pose", on_pose, LATE_JOINER)
        while time.time() < end:
            rclpy.spin_once(node, timeout_sec=0.1)
        print(f"ros_side: received {count} PoseStamped", flush=True)
    else:
        publisher = node.create_publisher(PoseStamped, "/robot1/pose", 10)

        def pose(x):
            msg = PoseStamped()
            msg.header.frame_id = "map"
            msg.pose.position.x, msg.pose.position.y, msg.pose.position.z = x, 2.0, 3.0
            msg.pose.orientation.w = 1.0
            return msg

        end = time.time() + seconds
        while time.time() < end:
            count += 1
            publisher.publish(pose(float(count)))
            rclpy.spin_once(node, timeout_sec=0.1)
        print(f"ros_side: published {count} PoseStamped", flush=True)
        # The bridge forwards from a thread of this process's RMW; the node
        # stays up while it passes the last poses on.
        drain_end = time.time() + 2
        while time.time() < drain_end:
            rclpy.spin_once(node, timeout_sec=0.1)
    node.destroy_node()
    rclpy.shutdown()


main()
