#!/usr/bin/env bash
# OpenDDS 3.34.0 on ACE 6 / TAO 2, built in its own source tree, for the
# `opendds` requirement of ci/run-examples.sh.
#
#   ci/provision/opendds.sh <prefix>
#
# The legs (cross-vendor-*/opendds/run.sh) want a built OpenDDS tree at
# DDS_ROOT: they source its setenv.sh and hand DDS_ROOT to CMake as
# OpenDDS_ROOT. So the tree is cloned into the prefix and built in place;
# nothing is installed anywhere else. setenv.sh and the generated CMake
# configuration hold absolute paths: a cached prefix has to be restored at
# the path it was built at.
#
# Pinned: tag v3.34.0, checked against its commit after the clone. configure
# downloads the ACE+TAO release that the tag's acetao.ini names (ACE 6.5.24 /
# TAO 2.5.24) and checks its MD5. The configure flags are those of the
# interop image (ci/Dockerfile.interop in the ZeroDDS tree) without
# --prefix: the image installs, the legs need the tree.
#
# After the build the object files and the ACE+TAO archive are deleted.
# Nothing reads them, and they are most of the tree's size, which is what a
# cache has to store.
#
# Exports DDS_ROOT=<prefix>. setenv.sh puts ACE_ROOT, TAO_ROOT, PATH and
# LD_LIBRARY_PATH in place for whoever sources it; none of that is exported
# here.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
. "$HERE/common.sh"

PREFIX="${1:?usage: $0 <install prefix>}"
case "$PREFIX" in /*) ;; *) PREFIX="$(pwd)/$PREFIX" ;; esac

OPENDDS_TAG=v3.34.0
OPENDDS_COMMIT=1a9f3ccca65c8933f2698a7cd45a1cef54c64780
CONFIGURE_FLAGS="--no-tests --no-debug --optimize --std=c++17 --compiler=g++"

# perl and make are OpenDDS's build system (configure, MPC, GNU makefiles);
# curl is how configure fetches ACE+TAO; cmake is what the legs build their
# peer with.
log "packages: OpenDDS build tools"
apt_install git make perl curl g++ cmake

STAMP="opendds $OPENDDS_TAG $OPENDDS_COMMIT
path $PREFIX
configure $CONFIGURE_FLAGS"

LIBS="$PREFIX/lib/libOpenDDS_Dcps.so $PREFIX/lib/libOpenDDS_Rtps.so $PREFIX/lib/libOpenDDS_Rtps_Udp.so"

built=no
# shellcheck disable=SC2086  # LIBS is a list of paths without spaces
if prefix_complete "$PREFIX" "$STAMP" \
        "$PREFIX/setenv.sh" "$PREFIX/bin/opendds_idl" "$PREFIX/cmake/config.cmake" $LIBS; then
    log "$PREFIX already holds OpenDDS $OPENDDS_TAG, built in place; not rebuilding"
else
    log "OpenDDS $OPENDDS_TAG, built in $PREFIX"
    built=yes
    reset_prefix "$PREFIX"
    git clone --depth 1 --branch "$OPENDDS_TAG" https://github.com/OpenDDS/OpenDDS.git "$PREFIX"
    mark_prefix "$PREFIX"
    verify_commit "$PREFIX" "$OPENDDS_COMMIT"
    (
        cd "$PREFIX"
        # configure takes ACE, TAO and MPC from these variables when they are
        # set; this build uses the ACE+TAO release the tag pins.
        unset DDS_ROOT ACE_ROOT TAO_ROOT MPC_ROOT
        # shellcheck disable=SC2086  # CONFIGURE_FLAGS is a list of words
        ./configure $CONFIGURE_FLAGS
        make -j"$(nproc)"
    )
    find "$PREFIX" -type d \( -name .obj -o -name .shobj \) -prune -exec rm -rf {} +
    find "$PREFIX" -type f -name '*.o' -delete
    rm -f "$PREFIX"/ACE+TAO-src-*
fi

log "checking the tree"
(
    set +u
    export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}"
    # shellcheck disable=SC1091
    . "$PREFIX/setenv.sh"
    set -u
    [ "$(readlink -f "$DDS_ROOT")" = "$(readlink -f "$PREFIX")" ] ||
        die "setenv.sh sets DDS_ROOT=$DDS_ROOT, the tree is at $PREFIX"
    # shellcheck disable=SC2086
    check_ldd "$PREFIX/bin/opendds_idl" "$ACE_ROOT/bin/tao_idl" $LIBS
    # The two IDL compilers on a small type: what opendds_target_sources
    # runs for every leg.
    probe_dir="$(mktemp -d "${RUNNER_TEMP:-${TMPDIR:-/tmp}}/provision-odds.XXXXXX")"
    trap 'rm -rf "$probe_dir"' EXIT
    cd "$probe_dir"
    printf '@final struct P { long x; double y; };\n' > gen.idl
    "$ACE_ROOT/bin/tao_idl" -I"$DDS_ROOT" -I"$TAO_ROOT" -Sa -St --idl-version 4 gen.idl
    "$DDS_ROOT/bin/opendds_idl" -I"$DDS_ROOT" --idl-version 4 gen.idl
    [ -f genTypeSupportImpl.cpp ] || die "opendds_idl wrote no genTypeSupportImpl.cpp"
)
if [ "$built" = yes ]; then
    seal_prefix "$PREFIX" "$STAMP"
fi

log "environment"
export_env DDS_ROOT "$PREFIX"
