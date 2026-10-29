#!/usr/bin/env bash
# Cyclone DDS C 11.0.1 with the iceoryx PSMX plugin, for the `cyclone-c` and
# `iceoryx-psmx` requirements of ci/run-examples.sh.
#
#   ci/provision/cyclone-iceoryx.sh <prefix>
#
# What the probes check afterwards:
#   cyclone-c      $CYCLONE/bin/idlc
#   iceoryx-psmx   iox-roudi on PATH and $CYCLONE/lib/libpsmx_iox.so
#
# iceoryx comes from the distribution's packages (iceoryx 2.0.6 on Ubuntu
# 26.04), the release the examples were run against. The packages carry what
# the example uses: iox-roudi in /usr/bin built with RouDi's TOML
# configuration (iceoryx-cyclone-zerocopy/roudi.toml), and the C binding in
# /usr/lib/x86_64-linux-gnu, where crates/iceoryx-cyclone/build.rs looks for
# it. Another upstream version stops the script.
#
# Cyclone is built from the tag 11.0.1, checked against its commit, with the
# CMake flags of the interop image (ci/Dockerfile.interop in the ZeroDDS
# tree) plus ENABLE_ICEORYX=ON: with the default AUTO a missing iceoryx
# builds Cyclone without the plugin, ON stops the configure instead.
#
# iceoryx-cyclone-zerocopy/cyclone_read.xml and cyclone_write.xml name the
# plugin by the absolute path /opt/cyclone/lib/libpsmx_iox.so. A prefix
# elsewhere is therefore linked to /opt/cyclone; an /opt/cyclone that is
# something else stops the script.
#
# Exports CYCLONE=<prefix>. Nothing goes on PATH: the examples call
# $CYCLONE/bin/idlc by path, and iox-roudi is in /usr/bin.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
. "$HERE/common.sh"

PREFIX="${1:?usage: $0 <install prefix>}"
case "$PREFIX" in /*) ;; *) PREFIX="$(pwd)/$PREFIX" ;; esac

CYCLONEDDS_TAG=11.0.1
CYCLONEDDS_COMMIT=e54e991f75a3e67f8e628da3171122e36ea5b872
ICEORYX_UPSTREAM=2.0.6
CMAKE_FLAGS="-DCMAKE_BUILD_TYPE=Release -DBUILD_IDLLIB=ON -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF -DENABLE_SECURITY=OFF -DENABLE_ICEORYX=ON -DENABLE_ICEORYX2=OFF"

log "packages: build tools, iceoryx"
apt_install git cmake g++ make libssl-dev \
    iceoryx libiceoryx-binding-c-dev libiceoryx-posh-dev libiceoryx-hoofs-dev

iox_version="$(dpkg-query -W -f='${Version}' iceoryx)"
case "$iox_version" in
    "$ICEORYX_UPSTREAM"+*|"$ICEORYX_UPSTREAM"-*) ;;
    *) die "the distribution has iceoryx $iox_version; the examples were run against $ICEORYX_UPSTREAM" ;;
esac
command -v iox-roudi >/dev/null 2>&1 || die "the iceoryx package put no iox-roudi on PATH"
echo "iceoryx $iox_version, $(command -v iox-roudi)"

# The plugin is compiled against the iceoryx headers of the installed
# package, so its version is part of what was built.
STAMP="cyclonedds $CYCLONEDDS_TAG $CYCLONEDDS_COMMIT
iceoryx $iox_version
cmake $CMAKE_FLAGS"

built=no
if prefix_complete "$PREFIX" "$STAMP" \
        "$PREFIX/bin/idlc" "$PREFIX/lib/libddsc.so" "$PREFIX/lib/libpsmx_iox.so"; then
    log "$PREFIX already holds Cyclone DDS $CYCLONEDDS_TAG with the iceoryx plugin; not rebuilding"
else
    log "Cyclone DDS $CYCLONEDDS_TAG into $PREFIX"
    built=yes
    make_work cyclone
    reset_prefix "$PREFIX"
    mark_prefix "$PREFIX"
    git clone --depth 1 --branch "$CYCLONEDDS_TAG" \
        https://github.com/eclipse-cyclonedds/cyclonedds.git "$WORK/src"
    verify_commit "$WORK/src" "$CYCLONEDDS_COMMIT"
    # shellcheck disable=SC2086  # CMAKE_FLAGS is a list of words
    cmake -S "$WORK/src" -B "$WORK/build" -DCMAKE_INSTALL_PREFIX="$PREFIX" $CMAKE_FLAGS
    cmake --build "$WORK/build" --parallel "$(nproc)"
    cmake --install "$WORK/build"
    [ -f "$PREFIX/lib/libpsmx_iox.so" ] || die "the build installed no $PREFIX/lib/libpsmx_iox.so"
fi

log "checking the install"
LD_LIBRARY_PATH="$PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
    check_ldd "$PREFIX/bin/idlc" "$PREFIX/lib/libddsc.so" "$PREFIX/lib/libpsmx_iox.so"
# idlc on a one-member type: the call every Cyclone leg makes first.
probe_dir="$(mktemp -d "${RUNNER_TEMP:-${TMPDIR:-/tmp}}/provision-idlc.XXXXXX")"
printf '@final struct P { long x; };\n' > "$probe_dir/p.idl"
( cd "$probe_dir" && LD_LIBRARY_PATH="$PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
    "$PREFIX/bin/idlc" p.idl ) || die "idlc failed on a one-member IDL"
[ -f "$probe_dir/p.c" ] && [ -f "$probe_dir/p.h" ] || die "idlc wrote no p.c/p.h"
rm -rf "$probe_dir"
if [ "$built" = yes ]; then
    seal_prefix "$PREFIX" "$STAMP"
fi

# The two XML configurations of iceoryx-cyclone-zerocopy load the plugin from
# /opt/cyclone/lib.
real_prefix="$(readlink -f "$PREFIX")"
if [ "$(readlink -f /opt/cyclone 2>/dev/null || true)" != "$real_prefix" ]; then
    if [ -L /opt/cyclone ]; then
        sudo rm /opt/cyclone
    elif [ -e /opt/cyclone ]; then
        die "/opt/cyclone exists and is not $PREFIX; iceoryx-cyclone-zerocopy loads the plugin from /opt/cyclone/lib"
    fi
    sudo mkdir -p /opt
    sudo ln -s "$real_prefix" /opt/cyclone
    echo "/opt/cyclone -> $real_prefix"
fi

log "environment"
export_env CYCLONE "$PREFIX"
