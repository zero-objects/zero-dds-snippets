# rmw-spatial-bridge

The `ros2.spatial_bridge` of `rmw_zerodds_cpp` between two processes: a
ROS 2 node whose RMW runs the bridge, and a SpatialDDS program on the wire.
`spatial::core::FramedPose` on `sdds/default/FramedPose` is bridged to and
from `geometry_msgs/msg/PoseStamped` on `/robot1/pose`, one direction per
run, each with its own configuration.

- `src/main.rs` — the SpatialDDS side (`spatial_side pub|sub SECONDS`),
  domain `ROS_DOMAIN_ID`.
- `ros_side.py` — the ROS 2 side (rclpy), started with
  `--config bridge-*.yaml`; the RMW reads `--config` from the process's
  own arguments.
- `bridge-dds-to-ros.yaml`, `bridge-ros-to-dds.yaml` — the two bridge
  configurations.

## Run it

Needs a ROS 2 installation (Jazzy here) and `rmw_zerodds_cpp` built and
installed from this tree (`crates/rmw-zerodds-shim/ament`).

```sh
ROS_SETUP=/path/to/ros2/setup.bash RMW_PREFIX=/path/to/rmw_zerodds_cpp/install ./run.sh
```

Output of a run this README was written from (ROS 2 Jazzy, robostack,
`ROS_DOMAIN_ID=57`; five runs, all passed):

```
spatial_side: wrote 58 FramedPose
ros_side: received 58 PoseStamped
ros_side: PoseStamped frame=map position=(1.0, 2.0, 3.0)
ros_side: published 60 PoseStamped
spatial_side: FramedPose frame=map t=[1.0, 2.0, 3.0]
SPATIAL BRIDGE OK: SpatialDDS->ROS 58 of 58, ROS->SpatialDDS 60 of 60
```

The run passes only when every pose written arrives, in both directions.
The bridge's writers offer TRANSIENT_LOCAL with KEEP_LAST 10, and both
readers of this example ask for TRANSIENT_LOCAL: a pose the bridge forwards
before its writer has matched the reader on the other side reaches that
reader from the writer's history of ten (DDS 1.4 §2.2.3.4). A VOLATILE
reader would not get it. The SpatialDDS writer, which is not the bridge's,
stays VOLATILE and waits for the bridge's reader
(`wait_for_matched_subscription`) before its first write; that wait
shortens its writing window, which is why that direction counts 58 or 60.

## What it needs from the SpatialDDS side

The SpatialDDS codecs have only an XCDR2 form; the writer offers
`data_representation` XCDR2, as the bridge's own participant does. The
ROS side is XCDR1 (`CDR_LE`), what ROS 2 middlewares put ROS messages in;
the bridge writes and reads both.
