#!/usr/bin/env bash
# ros2.spatial_bridge across processes: a ROS 2 node with rmw_zerodds_cpp
# runs the bridge in its RMW; a SpatialDDS program on the wire is the other
# side. Both directions, one configuration each.
#
#   ROS_SETUP=<a ROS 2 setup.bash>  RMW_PREFIX=<rmw_zerodds_cpp install prefix>  ./run.sh
set -uo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
: "${ROS_SETUP:?a ROS 2 setup.bash to source}"
: "${RMW_PREFIX:?the install prefix of rmw_zerodds_cpp}"
set +u
# shellcheck disable=SC1090
source "$ROS_SETUP"
set -u
export AMENT_PREFIX_PATH=$RMW_PREFIX:${AMENT_PREFIX_PATH:-}
export LD_LIBRARY_PATH=$RMW_PREFIX/lib:${LD_LIBRARY_PATH:-}
export RMW_IMPLEMENTATION=rmw_zerodds_cpp
export ROS_DOMAIN_ID=${ROS_DOMAIN_ID:-57}
# The bridge reports dropped fields and refused samples as observability
# events; they land in the logs.
export ZERODDS_OBSERVABILITY=${ZERODDS_OBSERVABILITY:-stderr-json}
(cd "$HERE" && cargo build -q --release) || exit 1
SIDE=${CARGO_TARGET_DIR:-$HERE/target}/release/spatial_side
out=$(mktemp -d)

# SpatialDDS -> ROS 2: the ROS node subscribes, its bridge reads FramedPose.
python3 "$HERE/ros_side.py" sub 12 --config "$HERE/bridge-dds-to-ros.yaml" > "$out/ros.log" 2>&1 &
ros=$!
sleep 3
"$SIDE" pub 6 > "$out/spatial.log" 2>&1
wait $ros
to_ros=$(grep -c "frame=map position=(" "$out/ros.log")

# ROS 2 -> SpatialDDS: the ROS node publishes, its bridge writes FramedPose.
# The SpatialDDS reader is TRANSIENT_LOCAL, so a pose the bridge forwards
# before its writer has matched that reader still reaches it.
"$SIDE" sub 40 > "$out/spatial2.log" 2>&1 &
sp=$!
python3 "$HERE/ros_side.py" pub 6 --config "$HERE/bridge-ros-to-dds.yaml" > "$out/ros2.log" 2>&1
published=$(sed -n 's/^ros_side: published \([0-9]*\) PoseStamped$/\1/p' "$out/ros2.log")
# The SpatialDDS side reads until it has as many as were published, or 5 s.
for _ in $(seq 1 50); do
  [ "$(grep -c "frame=map t=\[" "$out/spatial2.log")" -ge "${published:-1}" ] && break
  sleep 0.1
done
kill $sp 2>/dev/null; wait $sp 2>/dev/null
to_dds=$(grep -c "frame=map t=\[" "$out/spatial2.log")

tail -1 "$out/spatial.log"; tail -1 "$out/ros.log"; grep -m1 "position=" "$out/ros.log"
tail -1 "$out/ros2.log"; grep -m1 "t=\[" "$out/spatial2.log"
# Every pose written has to arrive, in both directions.
wrote=$(sed -n 's/^spatial_side: wrote \([0-9]*\) FramedPose$/\1/p' "$out/spatial.log")
if [ "$to_ros" -gt 0 ] && [ "$to_ros" = "${wrote:-}" ] && [ "$to_dds" -gt 0 ] && [ "$to_dds" = "${published:-}" ]; then
  echo "SPATIAL BRIDGE OK: SpatialDDS->ROS $to_ros of $wrote, ROS->SpatialDDS $to_dds of $published"
else
  echo "SPATIAL BRIDGE FAILED: SpatialDDS->ROS $to_ros of ${wrote:-?}, ROS->SpatialDDS $to_dds of ${published:-?} (logs in $out)"
  exit 1
fi
rm -rf "$out"
