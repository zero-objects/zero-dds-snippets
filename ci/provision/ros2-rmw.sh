#!/usr/bin/env bash
# A ROS 2 binary installation and rmw_zerodds_cpp built from the ZeroDDS
# tree, for the `ros2-rmw-zerodds` requirement of ci/run-examples.sh.
#
#   ZERODDS_SRC=<zero-dds checkout> ci/provision/ros2-rmw.sh <rmw-prefix>
#
# ROS 2 comes from the ROS apt repository as ros-<distro>-ros-base under
# /opt/ros/<distro>. The distribution follows the runner's Ubuntu release,
# the one the ROS project builds binaries of for it: Lyrical on 26.04
# (resolute), Jazzy on 24.04 (noble). ROS_DISTRO overrides the choice. The
# repository is configured by the ros2-apt-source package, pinned by version
# and SHA-256, as the ROS installation guide does it with the latest release.
#
# rmw_zerodds_cpp is built as crates/rmw-zerodds-shim/ament/CMakeLists.txt
# describes: cargo builds librmw_zerodds and libzerodds into
# $ZERODDS_SRC/target/release, where that CMakeLists.txt looks for them; the
# ament package is configured against the ROS installation and installed
# into <rmw-prefix>, the two Rust libraries beside it. That is ZeroDDS code:
# the stamp names the ZeroDDS commit and the ROS rmw package version, and a
# prefix from another commit is rebuilt.
#
# rmw-spatial-bridge/run.sh runs `python3` with rclpy, and rclpy is built for
# the interpreter the ROS packages target, the system python3. Configure and
# the check below use /usr/bin/python3 for the same reason.
#
# Exports ROS_SETUP=/opt/ros/<distro>/setup.bash and RMW_PREFIX=<rmw-prefix>.
# Neither the ROS environment nor the prefix's lib directory is exported;
# run.sh sources and extends them in its own process.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
. "$HERE/common.sh"

PREFIX="${1:?usage: ZERODDS_SRC=<zero-dds checkout> $0 <rmw install prefix>}"
case "$PREFIX" in /*) ;; *) PREFIX="$(pwd)/$PREFIX" ;; esac
: "${ZERODDS_SRC:?set ZERODDS_SRC to the zero-dds checkout}"
AMENT_PKG="$ZERODDS_SRC/crates/rmw-zerodds-shim/ament"
[ -f "$AMENT_PKG/CMakeLists.txt" ] || die "no $AMENT_PKG; ZERODDS_SRC=$ZERODDS_SRC is not a zero-dds checkout"
command -v cargo >/dev/null 2>&1 || die "cargo is not on PATH; librmw_zerodds and libzerodds are built with it"

ROS_APT_SOURCE_VERSION=1.3.0
# shellcheck disable=SC1091
codename="$(. /etc/os-release && echo "${UBUNTU_CODENAME:-${VERSION_CODENAME:-}}")"
case "$codename" in
    resolute)
        default_distro=lyrical
        ROS_APT_SOURCE_SHA256=e70bc980395f4a3b366b9c5e7f8f9b4d63a4064fb115f6ff3bc52f7faa02ef69 ;;
    noble)
        default_distro=jazzy
        ROS_APT_SOURCE_SHA256=f31d84adf5054c7d60ded0e82c0f776a77ea33af53d08f40b9ad7c94cca55296 ;;
    *) die "no ROS 2 distribution and ros2-apt-source pin for Ubuntu '$codename'" ;;
esac
DISTRO="${ROS_DISTRO:-$default_distro}"
ROS_PREFIX="/opt/ros/$DISTRO"
PY=/usr/bin/python3

make_work rmw

log "ROS apt repository (ros2-apt-source $ROS_APT_SOURCE_VERSION)"
installed="$(dpkg-query -W -f='${Status} ${Version}' ros2-apt-source 2>/dev/null || true)"
case "$installed" in
    "install ok installed $ROS_APT_SOURCE_VERSION"*)
        echo "ros2-apt-source $ROS_APT_SOURCE_VERSION is installed" ;;
    *)
        deb="$WORK/ros2-apt-source.deb"
        curl -fsSL -o "$deb" \
            "https://github.com/ros-infrastructure/ros-apt-source/releases/download/$ROS_APT_SOURCE_VERSION/ros2-apt-source_$ROS_APT_SOURCE_VERSION.${codename}_all.deb"
        got="$(sha256sum "$deb" | cut -d' ' -f1)"
        [ "$got" = "$ROS_APT_SOURCE_SHA256" ] ||
            die "ros2-apt-source deb has SHA-256 $got, expected $ROS_APT_SOURCE_SHA256"
        sudo dpkg -i "$deb" ;;
esac

# rclpy and geometry_msgs are what rmw-spatial-bridge/ros_side.py imports;
# the others are what the ament package's CMakeLists.txt finds by name. Each
# is listed so that a missing one fails here, by name.
log "packages: ROS 2 $DISTRO and the rmw build dependencies"
ros_pkgs="ros-$DISTRO-ros-base ros-$DISTRO-rclpy ros-$DISTRO-geometry-msgs
    ros-$DISTRO-ament-cmake ros-$DISTRO-rcutils ros-$DISTRO-rmw ros-$DISTRO-rmw-dds-common
    ros-$DISTRO-rmw-implementation-cmake ros-$DISTRO-rosidl-runtime-c
    ros-$DISTRO-rosidl-typesupport-introspection-c ros-$DISTRO-rosidl-typesupport-introspection-cpp"
# Not in Humble; the ament package builds without it there.
[ "$DISTRO" = humble ] || ros_pkgs="$ros_pkgs ros-$DISTRO-rosidl-dynamic-typesupport"
# shellcheck disable=SC2086  # ros_pkgs is a list of package names
apt_install cmake g++ make $ros_pkgs
[ -f "$ROS_PREFIX/setup.bash" ] || die "no $ROS_PREFIX/setup.bash after installing ros-$DISTRO-ros-base"

zerodds_commit="$(git -C "$ZERODDS_SRC" rev-parse HEAD 2>/dev/null || true)"
STAMP="ros $DISTRO rmw $(dpkg-query -W -f='${Version}' "ros-$DISTRO-rmw")
zerodds ${zerodds_commit:-unknown}"

built=no
if [ -n "$zerodds_commit" ] && prefix_complete "$PREFIX" "$STAMP" \
        "$PREFIX/lib/librmw_zerodds_cpp.so" "$PREFIX/lib/librmw_zerodds.so" "$PREFIX/lib/libzerodds.so" \
        "$PREFIX/share/ament_index/resource_index/rmw_typesupport/rmw_zerodds_cpp"; then
    log "$PREFIX already holds rmw_zerodds_cpp for ZeroDDS $zerodds_commit on $DISTRO; not rebuilding"
else
    built=yes
    log "librmw_zerodds and libzerodds (cargo, $ZERODDS_SRC/target/release)"
    # The command CMakeLists.txt names. CARGO_TARGET_DIR is fixed because
    # the CMake side reads the libraries from $ZERODDS_SRC/target/release.
    ( cd "$ZERODDS_SRC" &&
      CARGO_TARGET_DIR="$ZERODDS_SRC/target" cargo build --release -p rmw-zerodds-shim -p zerodds-c-api )

    log "rmw_zerodds_cpp into $PREFIX"
    reset_prefix "$PREFIX"
    mark_prefix "$PREFIX"
    (
        # Sourced with -e and -u off, as run.sh and the ZeroDDS tree's
        # run_ament_package_test.sh source it.
        set +eu
        # shellcheck disable=SC1091
        . "$ROS_PREFIX/setup.bash"
        set -eu
        ZERODDS_HOME="$ZERODDS_SRC" cmake -S "$AMENT_PKG" -B "$WORK/build" \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_PREFIX_PATH="$ROS_PREFIX" \
            -DCMAKE_INSTALL_PREFIX="$PREFIX" \
            -DPython3_EXECUTABLE="$PY"
        cmake --build "$WORK/build" --parallel "$(nproc)"
        cmake --install "$WORK/build"
    )
fi

log "checking the install"
(
    set +eu
    # shellcheck disable=SC1091
    . "$ROS_PREFIX/setup.bash"
    set -eu
    export AMENT_PREFIX_PATH="$PREFIX:${AMENT_PREFIX_PATH:-}"
    export LD_LIBRARY_PATH="$PREFIX/lib:${LD_LIBRARY_PATH:-}"
    check_ldd "$PREFIX/lib/librmw_zerodds_cpp.so"
    # rclpy loads this copy of the RMW, selected the way run.sh selects it.
    # Domain 44 is not the default of any run.sh in this repository.
    out="$(RMW_IMPLEMENTATION=rmw_zerodds_cpp ROS_DOMAIN_ID=44 timeout 60 "$PY" -c '
import rclpy
rclpy.init()
node = rclpy.create_node("zerodds_provision_probe")
print("LOADED", [l.split()[-1] for l in open("/proc/self/maps") if "librmw_zerodds_cpp" in l][0])
node.destroy_node()
rclpy.shutdown()
' 2>&1)" || die "rclpy did not start with RMW_IMPLEMENTATION=rmw_zerodds_cpp: $out"
    loaded="$(printf '%s\n' "$out" | sed -n 's/^LOADED //p')"
    [ "$(readlink -f "$loaded")" = "$(readlink -f "$PREFIX/lib/librmw_zerodds_cpp.so")" ] ||
        die "rclpy loaded $loaded, not $PREFIX/lib/librmw_zerodds_cpp.so"
    echo "rclpy loaded $loaded"
)
if [ "$built" = yes ] && [ -n "$zerodds_commit" ]; then
    seal_prefix "$PREFIX" "$STAMP"
fi

log "environment"
export_env ROS_SETUP "$ROS_PREFIX/setup.bash"
export_env RMW_PREFIX "$PREFIX"
