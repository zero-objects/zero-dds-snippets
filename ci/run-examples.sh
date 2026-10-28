#!/usr/bin/env bash
# Build AND run every example in this repository, one verdict per example.
#
# The point is the difference between "it compiled" and "it worked". A
# compiled example proves that the API names still exist; it does not prove
# that a sample crossed the wire, that a round-trip is lossless, or that a
# tool accepted its own configuration. This harness runs each example and
# checks what it printed.
#
# A missing toolchain is never a pass and never a silent skip. It is named,
# counted, and reported as a gap in what this run verified — the same
# discipline as `tools/interop-vendors.sh` in the main repository.
#
# Usage:
#   ci/run-examples.sh                     every example
#   ci/run-examples.sh rust-typed …        only the named ones
#   ci/run-examples.sh --list              the manifest, one name per line
#
# Environment:
#   ZERODDS_SRC   the zero-dds checkout the path deps reach into.
#                 Default: the parent of this repository.
#   EX_TIMEOUT    per-example wall clock in seconds (default 600).
#
# Exit status:
#   0  every selected example ran or completed its explicit build-only check.
#   1  at least one example failed or could not be verified.
#   2  the harness itself could not run (bad manifest, unlisted example).
#
# Portability: bash 3.2 and Python 3 (stdlib process-group deadline), on POSIX.
# No associative arrays or `readarray`.

set -uo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EX_CI="$REPO_ROOT/ci"
MANIFEST="$EX_CI/examples.manifest"
ZERODDS_SRC="${ZERODDS_SRC:-$(cd "$REPO_ROOT/.." && pwd)}"
# Per-example wall clock, seconds. Generous: a cold example compiles the DCPS
# runtime before it runs a line. It exists to catch a hang, not slowness.
EX_TIMEOUT="${EX_TIMEOUT:-600}"

export REPO_ROOT EX_CI ZERODDS_SRC
# The directory the C ABI shared library is built into, and the library
# file itself. Two variables because two consumers spell it differently:
# a linker takes the directory (-L), and `@zerodds/node` reads
# ZERODDS_LIB as the path of the file to dlopen.
export ZERODDS_LIBDIR="$ZERODDS_SRC/target/release"
case "$(uname -s)" in
    Darwin) zerodds_dylib=libzerodds.dylib ;;
    *)      zerodds_dylib=libzerodds.so ;;
esac
export ZERODDS_LIB="$ZERODDS_LIBDIR/$zerodds_dylib"
export ZERODDS_C_INC="$ZERODDS_SRC/crates/zerodds-c-api/include"
export ZERODDS_CPP_INC="$ZERODDS_SRC/crates/cpp/include"
# The OMG DDS Java binding jar carries the workspace version in its name,
# so it is resolved rather than spelled out.
ZERODDS_JAR="$(ls "$ZERODDS_SRC"/crates/java-omgdds/java/target/omgdds-*.jar 2>/dev/null | head -n 1)"
export ZERODDS_JAR
# The TypeScript examples have no node_modules of their own: they are checked
# against the real `@zerodds/node` / `@zerodds/ts-wasm` build, and they run
# through `tsx`, which lives in that build's own node_modules. Putting those
# two bin directories on PATH is what "crates/ts-node is built" means for a
# run, so `npm run start` finds the same tsx the binding was compiled with
# rather than whatever a global install happens to hold.
export PATH="$ZERODDS_SRC/crates/ts-node/node_modules/.bin:$ZERODDS_SRC/crates/ts-wasm/node_modules/.bin:$PATH"
# `tsx` resolves `@zerodds/node` through NODE_PATH when the example has no
# node_modules of its own.
export NODE_PATH="$ZERODDS_SRC/crates${NODE_PATH:+:$NODE_PATH}"
# Both spellings: macOS reads DYLD_LIBRARY_PATH, Linux reads LD_LIBRARY_PATH.
export LD_LIBRARY_PATH="$ZERODDS_LIBDIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export DYLD_LIBRARY_PATH="$ZERODDS_LIBDIR${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"

[ -f "$MANIFEST" ] || { echo "no manifest at $MANIFEST" >&2; exit 2; }

manifest_lines() {
    grep -v '^[[:space:]]*#' "$MANIFEST" | grep -v '^[[:space:]]*$'
}

if [ "${1:-}" = "--list" ]; then
    manifest_lines | cut -d'|' -f1
    exit 0
fi

# ---------------------------------------------------------------------------
# Capability probes. Each answers one question with a fact. `probe` prints
# nothing and returns 0 when the need is met; otherwise it prints why not.
# ---------------------------------------------------------------------------
probe() {
    need="$1"
    why=""
    case "$need" in
        -) why="" ;;
        cargo|curl|dotnet|go|java|javac|mvn|node|npm|python3|cmake|make|maturin|tsc)
            command -v "$need" >/dev/null 2>&1 || why="$need is not on PATH" ;;
        cc)  command -v cc  >/dev/null 2>&1 || why="no C compiler (cc) on PATH" ;;
        c++) command -v c++ >/dev/null 2>&1 || why="no C++ compiler (c++) on PATH" ;;
        zerodds-src)
            [ -d "$ZERODDS_SRC/crates" ] || \
                why="no zero-dds checkout at ZERODDS_SRC=$ZERODDS_SRC" ;;
        zerodds-cdylib)
            # Built by this run (`ensure_built`), so what is asked is whether
            # it can be.
            command -v cargo >/dev/null 2>&1 || \
                why="cargo is not on PATH; this run builds libzerodds with it" ;;
        zerodds-jar)
            [ -n "$ZERODDS_JAR" ] && [ -f "$ZERODDS_JAR" ] || \
                why="the OMG DDS Java binding jar is not built (mvn -q install -DskipTests in $ZERODDS_SRC/crates/java-omgdds/java)" ;;
        maturin-installed)
            # `maturin develop` needs a virtualenv, and a host with PEP 668
            # site-packages may refuse a plain install; the wheel is a zip,
            # so unpacking it and pointing PYTHONPATH at it works anywhere.
            python3 -c 'import zerodds' >/dev/null 2>&1 || \
                why="the zerodds Python extension is not importable (maturin develop in $ZERODDS_SRC/crates/py; or maturin build there, unzip target/wheels/zerodds-*.whl somewhere and put that directory on PYTHONPATH)" ;;
        # The entry point the examples import, not just the dist directory:
        # a build that failed part-way leaves the directory behind, and a
        # probe that accepts it turns a missing binding into a type error
        # blamed on the example.
        ts-node-build)
            # Built by this run (`ensure_built`) with the packages' own tsc:
            # @zerodds/node, and the @zerodds/wire it depends on by path
            # and the generated TypeScript imports.
            if [ ! -x "$ZERODDS_SRC/runtime/typescript/node_modules/.bin/tsc" ]; then
                why="runtime/typescript has no node_modules (npm ci there); this run builds @zerodds/wire with them"
            elif [ ! -x "$ZERODDS_SRC/crates/ts-node/node_modules/.bin/tsc" ]; then
                why="crates/ts-node has no node_modules (npm install --install-links=false there); this run builds its dist with them"
            fi ;;
        ts-wasm-build)
            [ -f "$ZERODDS_SRC/crates/ts-wasm/dist/index.d.ts" ] || \
                why="crates/ts-wasm is not built (npm install && npm run build there)" ;;
        pki-files)
            # The six files DDS-Security 1.2 §9.3.1 Table 44 and §9.4.1.2
            # Table 46 name. Without them a participant that asks for
            # security is refused — correctly — so there is nothing to run.
            if [ -z "${ZERODDS_PKI:-}" ]; then
                why="ZERODDS_PKI is not set; it must point at a directory holding the six DDS-Security files (ca-cert.pem, part-cert.pem, part-key.pem, permissions-ca-cert.pem, governance.p7s, permissions.p7s)"
            else
                for f in ca-cert.pem part-cert.pem part-key.pem \
                         permissions-ca-cert.pem governance.p7s permissions.p7s; do
                    [ -f "$ZERODDS_PKI/$f" ] || why="ZERODDS_PKI=$ZERODDS_PKI has no $f"
                done
            fi
            ;;
        rti-connext)
            # Not `peer:rti`: the example's own run.sh generates the RTI peer
            # from the IDL, builds it and starts it, so what has to be on the
            # host is the toolchain, and that is answerable. Both halves are
            # checked because they fail apart — `rtiddsgen` runs without a
            # licence file and the Connext runtime does not, so a host with
            # only the generator produces a peer that exits at startup.
            NDDSHOME="${NDDSHOME:-/opt/rti.com/rti_connext_dds-7.7.0}"
            if [ ! -x "$NDDSHOME/bin/rtiddsgen" ]; then
                why="no rtiddsgen at NDDSHOME=$NDDSHOME (set NDDSHOME to an RTI Connext install)"
            elif [ ! -f "$NDDSHOME/rti_license.dat" ]; then
                why="NDDSHOME=$NDDSHOME has no rti_license.dat; rtiddsgen needs none but the Connext runtime does"
            fi
            ;;
        cyclone-py)
            # The Cyclone DDS Python binding the cyclone legs start their
            # peer with; `import cyclonedds` loads libddsc as well.
            "${PYTHON:-python3}" -c 'import cyclonedds' >/dev/null 2>&1 || \
                why="the Cyclone DDS Python binding is not importable (${PYTHON:-python3} -c 'import cyclonedds'; pip install cyclonedds against a Cyclone DDS install)" ;;
        cyclone-c)
            # The example compiles its peer against the C library with idlc.
            [ -x "${CYCLONE:-/opt/cyclone}/bin/idlc" ] || \
                why="no idlc at CYCLONE=${CYCLONE:-/opt/cyclone} (set CYCLONE to a Cyclone DDS install)" ;;
        fastdds)
            # fastddsgen generates the peer's type support, which is then
            # built against the Fast DDS 3 install.
            if ! command -v "${FASTDDSGEN:-fastddsgen}" >/dev/null 2>&1; then
                why="${FASTDDSGEN:-fastddsgen} is not on PATH (set FASTDDSGEN)"
            elif [ ! -d "${FASTDDS_HOME:-/opt/fastdds}/lib" ]; then
                why="no ${FASTDDS_HOME:-/opt/fastdds}/lib (set FASTDDS_HOME to a Fast DDS install)"
            fi
            ;;
        opendds)
            # A built OpenDDS tree: its setenv.sh puts opendds_idl, ACE/TAO
            # and the libraries on the paths the peer is built with.
            [ -f "${DDS_ROOT:-}/setenv.sh" ] || \
                why="no \$DDS_ROOT/setenv.sh (DDS_ROOT=${DDS_ROOT:-unset}; set it to a built OpenDDS tree)" ;;
        opensplice)
            # OpenSplice Community Edition 6.9: idlpp generates the peer's
            # type support, which is built against the C++ API.
            if [ ! -x "${OSPL_HOME:-/opt/opensplice-6.9}/bin/idlpp" ]; then
                why="no idlpp at OSPL_HOME=${OSPL_HOME:-/opt/opensplice-6.9} (set OSPL_HOME to an OpenSplice install)"
            elif [ ! -f "${OSPL_HOME:-/opt/opensplice-6.9}/release.com" ]; then
                why="no release.com at OSPL_HOME=${OSPL_HOME:-/opt/opensplice-6.9}"
            fi
            ;;
        ros2-rmw-zerodds)
            # A ROS 2 installation and rmw_zerodds_cpp built from this tree
            # (crates/rmw-zerodds-shim/ament); the example sources the one
            # and loads the other.
            if [ ! -f "${ROS_SETUP:-}" ]; then
                why="ROS_SETUP does not name a ROS 2 setup.bash (ROS_SETUP=${ROS_SETUP:-unset})"
            elif ! ls "${RMW_PREFIX:-/nonexistent}"/lib/librmw_zerodds_cpp.* >/dev/null 2>&1; then
                why="no librmw_zerodds_cpp under RMW_PREFIX=${RMW_PREFIX:-unset}/lib (build crates/rmw-zerodds-shim/ament and set RMW_PREFIX to its install prefix)"
            fi
            ;;
        peer:*)
            # There is no probe that can honestly answer "is a Cyclone
            # publisher on this domain". The harness names what is needed
            # and counts the example as unverified rather than guessing.
            why="needs a ${need#peer:} peer on the wire; this harness starts none" ;;
        multicast)
            why="needs multicast over a real interface, not loopback" ;;
        iceoryx-psmx)
            # RouDi and a Cyclone built with the iceoryx PSMX plugin; the
            # example starts both itself.
            command -v iox-roudi >/dev/null 2>&1 || why="iox-roudi is not on PATH"
            [ -z "$why" ] && [ ! -f "${CYCLONE:-/opt/cyclone}/lib/libpsmx_iox.so" ] && \
                why="no Cyclone iceoryx plugin at ${CYCLONE:-/opt/cyclone}/lib/libpsmx_iox.so (set CYCLONE)" ;;
        linux)
            # `sched_setscheduler`, `sched_setaffinity` and the PREEMPT_RT
            # knobs are Linux system calls. On another kernel there is no
            # weaker thing to check: the crate does not compile.
            [ "$(uname -s)" = "Linux" ] || why="needs a Linux kernel; this host runs $(uname -s)" ;;
        *) why="unknown requirement '$need' in the manifest" ;;
    esac
    [ -z "$why" ] && return 0
    printf '%s' "$why"
    return 1
}

# ---------------------------------------------------------------------------
# Tallies. `unverified` is the number that matters: it is how much of the
# repository this run said nothing about. Bash 3.2 has no associative
# arrays, so each bucket is a newline-separated "name<TAB>reason" list.
# ---------------------------------------------------------------------------
PASS_N=0; FAIL_N=0; UNVER_N=0; BLOCK_N=0; BUILT_N=0
FAIL_LIST=""; UNVER_LIST=""

verdict() { # name status detail
    name="$1"; status="$2"; detail="${3:-}"
    case "$status" in
        pass)       PASS_N=$((PASS_N+1));  printf 'PASS        %-38s %s\n' "$name" "$detail" ;;
        fail)       FAIL_N=$((FAIL_N+1));  FAIL_LIST="$FAIL_LIST$name	$detail
";                                         printf 'FAIL        %-38s %s\n' "$name" "$detail" ;;
        unverified) UNVER_N=$((UNVER_N+1)); UNVER_LIST="$UNVER_LIST$name	$detail
";                                         printf 'UNVERIFIED  %-38s %s\n' "$name" "$detail" ;;
        blocked)    BLOCK_N=$((BLOCK_N+1)); printf 'BLOCKED     %-38s %s\n' "$name" "$detail" ;;
        built)      BUILT_N=$((BUILT_N+1)); printf 'BUILT-ONLY  %-38s %s\n' "$name" "$detail" ;;
    esac
}

selected="$*"
wanted() { # name
    [ -z "$selected" ] && return 0
    for n in $selected; do [ "$n" = "$1" ] && return 0; done
    return 1
}

# Give each command its own process group. Timing out only its shell leaves
# Cargo, compilers and peers alive and consuming memory after the reported
# failure. Exit 124 follows timeout(1), without requiring it on macOS.
with_deadline() { # seconds dir command
    python3 - "$1" "$2" "$3" <<'PY'
import os
import signal
import subprocess
import sys

seconds = int(sys.argv[1])
if seconds <= 0:
    raise SystemExit("deadline must be positive")
child = subprocess.Popen(["bash", "-c", sys.argv[3]], cwd=sys.argv[2],
                         start_new_session=True)

def stop(code):
    try:
        os.killpg(child.pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    child.wait()
    raise SystemExit(code)

signal.signal(signal.SIGTERM, lambda *_: stop(143))
signal.signal(signal.SIGINT, lambda *_: stop(130))
try:
    result = child.wait(timeout=seconds)
except subprocess.TimeoutExpired:
    stop(124)
raise SystemExit(result if result >= 0 else 128 - result)
PY
}

# ---------------------------------------------------------------------------
# What the examples load is built from ZERODDS_SRC by this run, once, before
# the first example that needs it: the C ABI library ZERODDS_LIB names, and
# the JavaScript of @zerodds/node that `tsx` resolves through NODE_PATH, and
# the @zerodds/wire it and the generated TypeScript import.
#
# The harness used to export the path of whatever target/release held and
# accept any crates/ts-node/dist, and an example then measured a library
# older than the tree without a word: the TypeScript QoS check loaded a
# libzerodds.so built before the runtime change it was run to check.
# ---------------------------------------------------------------------------
BUILT_CDYLIB=""   # "", yes, or the reason it failed
BUILT_TS_WIRE=""
BUILT_TS_NODE=""
build_step() { # description dir command...
    what="$1"; where="$2"; shift 2
    log="$(mktemp)"
    if ( cd "$where" && "$@" ) >"$log" 2>&1; then
        rm -f "$log"; printf 'yes'
    else
        printf '%s did not build (%s, in %s): %s' "$what" "$*" "$where" \
            "$(tail -n 1 "$log")"
        rm -f "$log"
    fi
}
ensure_built() { # needs; sets BUILD_WHY and returns 1 when a build failed
    BUILD_WHY=""
    case ",$1," in
        *,zerodds-cdylib,*|*,ts-node-build,*)
            if [ -z "$BUILT_CDYLIB" ]; then
                BUILT_CDYLIB="$(build_step libzerodds "$ZERODDS_SRC" \
                    env CARGO_TARGET_DIR="$ZERODDS_SRC/target" \
                    cargo build -q --release -p zerodds-c-api)"
                [ "$BUILT_CDYLIB" = yes ] && [ ! -f "$ZERODDS_LIB" ] && \
                    BUILT_CDYLIB="cargo built zerodds-c-api but $ZERODDS_LIB is not there"
            fi
            [ "$BUILT_CDYLIB" = yes ] || { BUILD_WHY="$BUILT_CDYLIB"; return 1; } ;;
    esac
    case ",$1," in
        *,ts-node-build,*)
            if [ -z "$BUILT_TS_WIRE" ]; then
                BUILT_TS_WIRE="$(build_step @zerodds/wire "$ZERODDS_SRC/runtime/typescript" \
                    npm run --silent build)"
            fi
            [ "$BUILT_TS_WIRE" = yes ] || { BUILD_WHY="$BUILT_TS_WIRE"; return 1; }
            if [ -z "$BUILT_TS_NODE" ]; then
                BUILT_TS_NODE="$(build_step @zerodds/node "$ZERODDS_SRC/crates/ts-node" \
                    npm run --silent build)"
            fi
            [ "$BUILT_TS_NODE" = yes ] || { BUILD_WHY="$BUILT_TS_NODE"; return 1; } ;;
    esac
    return 0
}

run_one() { # name needs verdict cmd expect note
    name="$1"; needs="$2"; want_verdict="$3"; cmd="$4"; expect="$5"; note="$6"
    dir="$REPO_ROOT/$name"

    if [ ! -d "$dir" ]; then
        verdict "$name" fail "the manifest names a directory that is not in the tree"
        return
    fi

    # Every requirement named individually, so the report says what to
    # install rather than "skipped".
    missing=""
    for need in $(printf '%s' "$needs" | tr ',' ' '); do
        [ -z "$need" ] && continue
        if ! why="$(probe "$need")"; then
            [ -n "$missing" ] && missing="$missing; "
            missing="$missing$why"
        fi
    done
    if [ -n "$missing" ]; then
        verdict "$name" unverified "$missing"
        return
    fi
    if ! ensure_built "$needs"; then
        verdict "$name" fail "$BUILD_WHY"
        return
    fi

    if [ "$want_verdict" = "blocked" ]; then
        # A historical label is not a failure signature. Run the command and
        # preserve its real failure instead of certifying any nonzero exit as
        # the recorded defect. A successful command makes the label stale.
        if [ "$cmd" = "-" ]; then
            verdict "$name" fail "$note (no command recorded; blocker cannot be verified)"
            return
        fi
        log="$(mktemp)"
        if with_deadline "$EX_TIMEOUT" "$dir" "$cmd" >"$log" 2>&1; then
            verdict "$name" fail "BLOCKER STALE: the manifest calls this blocked, but the command succeeded. Re-check the defect and move the row to \`run\`. Recorded reason: $note"
            tail -n 5 "$log" | sed 's/^/            | /'
            printf '            | full log: %s\n' "$log"
            return
        else
            rc=$?
        fi
        verdict "$name" fail "exit $rc; historical blocker is not a verified diagnosis: $note"
        tail -n 20 "$log" | sed 's/^/            | /'
        printf '            | full log: %s\n' "$log"
        return
    fi

    log="$(mktemp)"
    if with_deadline "$EX_TIMEOUT" "$dir" "$cmd" >"$log" 2>&1; then
        rc=0
    else
        rc=$?
        if [ "$rc" -eq 124 ]; then
            verdict "$name" fail "still running after ${EX_TIMEOUT}s and was killed (a run that waits for a peer must say so in the manifest, not hang)"
            tail -n 20 "$log" | sed 's/^/            | /'
            printf '            | full log: %s\n' "$log"
            return
        fi
        verdict "$name" fail "exit $rc; last line: $(tail -n 1 "$log")"
        tail -n 20 "$log" | sed 's/^/            | /'
        printf '            | full log: %s\n' "$log"
        return
    fi
    if [ "$expect" != "-" ] && ! grep -qF -- "$expect" "$log"; then
        verdict "$name" fail "ran, but its output has no \"$expect\""
        tail -n 20 "$log" | sed 's/^/            | /'
        printf '            | full log: %s\n' "$log"
        return
    fi
    shown="$(grep -vE '^\s*(Compiling|Finished|Running|Updating|Downloaded|Locking|Blocking|warning)' "$log" | grep -v '^$' | tail -n 1)"
    rm -f "$log"
    if [ "$want_verdict" = "build" ]; then
        # The command still ran and still had to succeed. The verdict is
        # separate because what it checked is weaker than a run: the note
        # on the line says what that is.
        verdict "$name" built "$note"
    else
        verdict "$name" pass "${shown:-ran, no output}"
    fi
}

echo "ZeroDDS examples — build and run"
command -v python3 >/dev/null 2>&1 || { echo "Python 3 is required for process cleanup" >&2; exit 2; }
echo "  repository   $REPO_ROOT"
echo "  zero-dds src $ZERODDS_SRC"
echo

# An example in the tree but not in the manifest would be verified by nobody
# and reported by nobody — the exact shape of a silent gap. Refuse to start
# rather than print a total that leaves it out.
unlisted=""
untracked=""
for d in "$REPO_ROOT"/*/; do
    n="$(basename "$d")"
    case "$n" in ci|target|node_modules|.*) continue ;; esac
    grep -q "^$n|" "$MANIFEST" && continue
    # A directory git does not track is not part of the repository the
    # manifest describes — work in progress in somebody's checkout. It is
    # named, so it cannot pass unnoticed, but it does not stop the run.
    if git -C "$REPO_ROOT" ls-files --error-unmatch "$n" >/dev/null 2>&1; then
        unlisted="$unlisted $n"
    else
        untracked="$untracked $n"
    fi
done
if [ -n "$untracked" ]; then
    echo "not tracked by git and not in the manifest, so this run says nothing about:$untracked"
    echo
fi
if [ -n "$unlisted" ]; then
    echo "tracked but not in $MANIFEST:$unlisted" >&2
    echo "add each with a verdict, and a reason where the verdict is not 'run'." >&2
    exit 2
fi

TOTAL=0
while IFS='|' read -r name needs want_verdict cmd expect note; do
    case "$name" in ''|\#*) continue ;; esac
    wanted "$name" || continue
    TOTAL=$((TOTAL + 1))
    run_one "$name" "$needs" "$want_verdict" "$cmd" "$expect" "$note"
done < <(manifest_lines)

if [ "$TOTAL" -eq 0 ]; then
    echo "no examples selected; check the requested names" >&2
    exit 2
fi

echo
echo "----------------------------------------------------------------------"
printf 'ran and checked   %3d\n' "$PASS_N"
printf 'failed            %3d\n' "$FAIL_N"
printf 'built only        %3d   (no program to run; each line carries the reason)\n' "$BUILT_N"
printf 'blocked           %3d   (a named defect; each line carries it)\n' "$BLOCK_N"
printf 'not verified      %3d   (a requirement was missing on this host)\n' "$UNVER_N"
printf 'total in this run %3d\n' "$TOTAL"

if [ "$UNVER_N" -gt 0 ]; then
    echo
    echo "This run verified nothing about these $UNVER_N example(s):"
    printf '%s' "$UNVER_LIST" | while IFS='	' read -r n r; do
        [ -n "$n" ] && printf '  %-38s %s\n' "$n" "$r"
    done
fi
if [ "$FAIL_N" -gt 0 ]; then
    echo
    echo "Failed:"
    printf '%s' "$FAIL_LIST" | while IFS='	' read -r n r; do
        [ -n "$n" ] && printf '  %-38s %s\n' "$n" "$r"
    done
    exit 1
fi
[ "$UNVER_N" -eq 0 ] || exit 1
exit 0
