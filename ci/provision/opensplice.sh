#!/usr/bin/env bash
# OpenSplice Community Edition 6.9 (OSPL_V6_9_210323OSS_RELEASE), for the
# `opensplice` requirement of ci/run-examples.sh.
#
#   ZERODDS_SRC=<zero-dds checkout> ci/provision/opensplice.sh <prefix>
#
# The recipe is the ZeroDDS tree's tools/opensplice/build-opensplice.sh: its
# header lists each deviation from the release build and why. This script
# clones the tag, checks the commit, gives the recipe what it expects from
# the host, and copies the resulting HDE tree into the prefix. release.com
# there derives OSPL_HOME from its own location, so the tree can be copied
# and cached anywhere.
#
# What the recipe expects from the host:
#
# * JDK 8. OpenSplice's configure reads `javac -version` from stderr, where
#   only JDK 8 and older print it; with a newer javac it stops with "Only
#   Oracle JAVA 1.6 and above is supported". openjdk-8 comes from Ubuntu's
#   universe component. Installing it may move the system `java` and
#   `javac` alternatives; they are put back, so that the steps after this
#   one (the Java examples, fastddsgen) find the runner's Java on PATH.
# * GCC 14. The recipe's flags were worked out against GCC 14. The runner's
#   default GCC 15 has not been tried on this code base, so the build sees
#   gcc-14 under the names OpenSplice's makefiles and configure call (gcc,
#   g++, cc, c++, cpp, gcc-ar). The legs compile their peer with the
#   default g++ against the installed headers and libraries.
# * No stray configuration. configure reads QTDIR, NODEJS_HOME, PYTHON3_HOME,
#   M2_HOME, BOOST_ROOT, PROTOBUF_HOME and MONO_HOME to switch optional
#   components on; they are cleared, so the build does not depend on what the
#   runner image sets.
#
# Exports OSPL_HOME=<prefix>. run.sh sources $OSPL_HOME/release.com itself.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=common.sh
. "$HERE/common.sh"

PREFIX="${1:?usage: ZERODDS_SRC=<zero-dds checkout> $0 <install prefix>}"
case "$PREFIX" in /*) ;; *) PREFIX="$(pwd)/$PREFIX" ;; esac
: "${ZERODDS_SRC:?set ZERODDS_SRC to the zero-dds checkout}"
RECIPE="$ZERODDS_SRC/tools/opensplice/build-opensplice.sh"
[ -f "$RECIPE" ] || die "no $RECIPE; ZERODDS_SRC=$ZERODDS_SRC is not a zero-dds checkout"

OSPL_TAG=OSPL_V6_9_210323OSS_RELEASE
OSPL_COMMIT=5ec834ef215e82036bc2f1ac4530ae6001de47cc
JDK8=/usr/lib/jvm/java-8-openjdk-amd64

STAMP="opensplice $OSPL_TAG $OSPL_COMMIT
build-opensplice.sh $(sha256sum "$RECIPE" | cut -d' ' -f1)
compiler gcc-14"

# A restored prefix gets g++ only, which the legs build their peer with; the
# rest below is for a build. check_ldd further down names any shared library
# the installed tree needs and the runner lacks.
log "packages: g++ for the legs"
apt_install g++

built=no
if prefix_complete "$PREFIX" "$STAMP" \
        "$PREFIX/release.com" "$PREFIX/bin/idlpp" "$PREFIX/lib/libdcpssacpp.so" \
        "$PREFIX/lib/libddskernel.so" "$PREFIX/etc/config/ospl_sp_ddsi.xml"; then
    log "$PREFIX already holds OpenSplice $OSPL_TAG; not rebuilding"
else
    log "packages: OpenSplice build tools, GCC 14, JDK 8"
    java_before="$(readlink -f "$(command -v java 2>/dev/null || true)" 2>/dev/null || true)"
    javac_before="$(readlink -f "$(command -v javac 2>/dev/null || true)" 2>/dev/null || true)"
    apt_install git ca-certificates make gcc-14 g++-14 flex bison gawk perl openjdk-8-jdk-headless
    [ -x "$JDK8/bin/javac" ] || die "openjdk-8-jdk-headless installed no $JDK8/bin/javac"
    for tool in java javac; do
        case "$tool" in java) before="$java_before" ;; javac) before="$javac_before" ;; esac
        now="$(readlink -f "$(command -v "$tool" 2>/dev/null || true)" 2>/dev/null || true)"
        if [ -n "$before" ] && [ "$now" != "$before" ]; then
            echo "openjdk-8 moved $tool to $now; putting it back to $before"
            sudo update-alternatives --set "$tool" "$before"
        fi
    done

    log "OpenSplice $OSPL_TAG (tools/opensplice/build-opensplice.sh)"
    built=yes
    make_work opensplice
    git clone --depth 1 --branch "$OSPL_TAG" https://github.com/ADLINK-IST/opensplice.git "$WORK/src"
    verify_commit "$WORK/src" "$OSPL_COMMIT"

    mkdir -p "$WORK/gcc-14"
    for pair in gcc:gcc-14 g++:g++-14 cc:gcc-14 c++:g++-14 cpp:cpp-14 gcc-ar:gcc-ar-14; do
        target="$(command -v "${pair#*:}")" || die "no ${pair#*:} on PATH after installing gcc-14"
        ln -s "$target" "$WORK/gcc-14/${pair%%:*}"
    done
    (
        unset OSPL_HOME OSPL_URI SPLICE_TARGET SPLICE_HOST SPLICE_ORB SPLICE_JDK \
              QTDIR NODEJS_HOME PYTHON3_HOME M2_HOME BOOST_ROOT PROTOBUF_HOME MONO_HOME \
              STRICT_CHECKS
        export PATH="$WORK/gcc-14:$PATH"
        gcc --version | sed -n 1p
        # The recipe replaces the directory it is given; it gets one of its own.
        bash "$RECIPE" "$WORK/src" "$WORK/hde" "$JDK8"
    )

    reset_prefix "$PREFIX"
    mark_prefix "$PREFIX"
    tar -C "$WORK/hde" -cf - . | tar -C "$PREFIX" -xf -
fi

log "checking the install"
(
    # As the legs source it: without -u (release.com reads LD_LIBRARY_PATH
    # unset) and without -e.
    set +eu
    # shellcheck disable=SC1091
    . "$PREFIX/release.com" >/dev/null
    set -eu
    [ "$(readlink -f "$OSPL_HOME")" = "$(readlink -f "$PREFIX")" ] ||
        die "release.com sets OSPL_HOME=$OSPL_HOME, the tree is at $PREFIX"
    check_ldd "$PREFIX/bin/idlpp" "$PREFIX/lib/libdcpssacpp.so" "$PREFIX/lib/libddskernel.so"
    for d in include/sys include/dcps/C++/SACPP; do
        [ -d "$PREFIX/$d" ] || die "no $PREFIX/$d; the legs compile against it"
    done
    # idlpp on a keylist type in the form the legs use: a struct at global
    # scope, in a file whose name is not the type's (idlpp takes the file
    # name as an identifier and refuses a type spelled the same).
    probe_dir="$(mktemp -d "${RUNNER_TEMP:-${TMPDIR:-/tmp}}/provision-ospl.XXXXXX")"
    trap 'rm -rf "$probe_dir"' EXIT
    cd "$probe_dir"
    printf 'struct P { long id; long v; };\n#pragma keylist P id\n' > Probe.idl
    idlpp -S -l cpp Probe.idl
    for f in Probe.cpp ProbeDcps.cpp ProbeDcps_impl.cpp ProbeSplDcps.cpp; do
        [ -f "$f" ] || die "idlpp wrote no $f"
    done
)
if [ "$built" = yes ]; then
    seal_prefix "$PREFIX" "$STAMP"
fi

log "environment"
export_env OSPL_HOME "$PREFIX"
