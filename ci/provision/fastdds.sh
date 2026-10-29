#!/usr/bin/env bash
# Fast DDS 3.6.1 with Fast CDR, foonathan memory and Fast-DDS-Gen, for the
# `fastdds` requirement of ci/run-examples.sh.
#
#   ZERODDS_SRC=<zero-dds checkout> ci/provision/fastdds.sh <prefix>
#
# The recipe is the ZeroDDS tree's tests/interop/ci/install_fastdds.sh: it
# holds the pins (tag and commit of each repository, checked after every
# clone) and is what the ZeroDDS interop workflow runs on the same runner
# image. This script gives it a prefix and a work directory, decides whether
# a restored prefix is complete, and exports what the probe and the run.sh
# files read. The recipe's own hash is the stamp: a change to a pin there
# rebuilds the prefix.
#
# install_fastdds.sh also builds tests/interop/ci/fastdds/fastdds_robot in
# the ZeroDDS checkout. Nothing here uses it; it is the price of not keeping
# a second copy of the recipe.
#
# Exports FASTDDS_HOME=<prefix> and FASTDDSGEN=<the launcher>, and puts the
# launcher's own directory on PATH. The launcher finds its jar from
# `dirname "$0"` and does not follow a symbolic link, so it is never linked
# anywhere else.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
. "$HERE/common.sh"

PREFIX="${1:?usage: ZERODDS_SRC=<zero-dds checkout> $0 <install prefix>}"
case "$PREFIX" in /*) ;; *) PREFIX="$(pwd)/$PREFIX" ;; esac
: "${ZERODDS_SRC:?set ZERODDS_SRC to the zero-dds checkout}"
RECIPE="$ZERODDS_SRC/tests/interop/ci/install_fastdds.sh"
[ -f "$RECIPE" ] || die "no $RECIPE; ZERODDS_SRC=$ZERODDS_SRC is not a zero-dds checkout"

GEN_DIR="$PREFIX/share/fastddsgen/scripts"
GEN="$GEN_DIR/fastddsgen"
STAMP="install_fastdds.sh $(sha256sum "$RECIPE" | cut -d' ' -f1)"

# What a restored prefix needs on a fresh runner: the shared libraries
# libfastdds was linked against (tinyxml2, OpenSSL) and what the legs build
# their peer with. The recipe installs these too, plus a JDK; Fast-DDS-Gen
# runs on the runner's own Java.
log "packages: what the installed libraries and the peer builds need"
apt_install cmake g++ libssl-dev libasio-dev libtinyxml2-dev

built=no
if prefix_complete "$PREFIX" "$STAMP" \
        "$PREFIX/include/fastdds" "$PREFIX/lib/libfastdds.so" "$PREFIX/lib/libfastcdr.so" \
        "$GEN" "$PREFIX/share/fastddsgen/share/fastddsgen/java/fastddsgen.jar"; then
    log "$PREFIX already holds the stack install_fastdds.sh pins; not rebuilding"
else
    log "Fast DDS stack into $PREFIX (tests/interop/ci/install_fastdds.sh)"
    built=yes
    make_work fastdds
    reset_prefix "$PREFIX"
    mark_prefix "$PREFIX"
    # The recipe takes every pin from the environment with the canonical
    # value as default; whatever the runner environment holds under those
    # names must not replace a pin.
    (
        unset FOONATHAN_TAG FOONATHAN_COMMIT FASTCDR_TAG FASTCDR_COMMIT \
              FASTDDS_TAG FASTDDS_COMMIT FASTDDSGEN_TAG FASTDDSGEN_COMMIT
        n_jobs="$(nproc)"
        export FASTDDS_PREFIX="$PREFIX" WORK="$WORK/src" BUILD_JOBS="$n_jobs"
        bash "$RECIPE"
    )
fi

log "checking the install"
[ -x "$GEN" ] || die "no executable $GEN"
LD_LIBRARY_PATH="$PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
    check_ldd "$PREFIX/lib/libfastdds.so" "$PREFIX/lib/libfastcdr.so"
command -v java >/dev/null 2>&1 || die "no java on PATH; fastddsgen is a Java program"
java_version="$(java -version 2>&1)" || die "java -version failed: $java_version"
printf '%s\n' "${java_version%%$'\n'*}"
"$GEN" -version || die "$GEN -version failed"
if [ "$built" = yes ]; then
    seal_prefix "$PREFIX" "$STAMP"
fi

log "environment"
export_env FASTDDS_HOME "$PREFIX"
export_env FASTDDSGEN "$GEN"
export_path "$GEN_DIR"
