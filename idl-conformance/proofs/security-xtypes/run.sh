#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
#
# Run the secure XTypes matrix: {ping vendor x pong vendor} x {XTypes type} x
# {governance profile}. Verdict per cell is PROOF_OK / PROOF_FAIL / SEC_FAIL /
# NO_MATCH, taken from the ping-side stdout (field-level content check).
#
# Requires the per-profile assets from the roundtrip-bench security-matrix
# harness (gen_profile.sh writes <profile>/text + cyclone_<role>.xml). Point
# PROFILES_DIR at that output.
#
#   PROFILES_DIR   roundtrip-bench security-matrix/profiles  (default below)
#   BUILD          ./build
#   DYLIB          libzerodds.so dir
#   SAMPLES        per-cell sample count (default 100)
#   TYPES          space list (default: mutable appendable keyed nested)
#   PROFILES       space list (default: data-enc meta-data-enc all-enc rtps-enc)
#   VENDORS        space list (default: zerodds fastdds cyclone)
set -u
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO="${REPO:-$(cd "$HERE/../../../.." && pwd)}"
BUILD="${BUILD:-$HERE/build}"
DYLIB="${DYLIB:-$REPO/target/release}"
PROFILES_DIR="${PROFILES_DIR:-$REPO/tests/perf/dds-roundtrip-bench/security-matrix/profiles}"
SAMPLES="${SAMPLES:-100}"
DOMAIN="${DOMAIN:-200}"
TYPES=(${TYPES:-mutable appendable keyed nested})
PROFILES=(${PROFILES:-data-enc meta-data-enc all-enc rtps-enc})
VENDORS=(${VENDORS:-zerodds fastdds cyclone})

bin() { echo "$BUILD/${1}-secxt"; }

start() { # vendor role profile type log peer  -> pid
  local v="$1" role="$2" prof="$3" ty="$4" log="$5" peer="$6"; shift 6
  local b; b="$(bin "$v")"; local -a e=(ZERODDS_BENCH_DOMAIN=$DOMAIN); local -a a=("$@")
  local CYCLONE="${CYCLONE:-/opt/cyclone}" FASTDDS="${FASTDDS:-/opt/fastdds}"
  case "$v" in
    cyclone) e+=("CYCLONEDDS_URI=file://$PROFILES_DIR/$prof/cyclone_${role}.xml"
                 "LD_LIBRARY_PATH=$CYCLONE/lib:${LD_LIBRARY_PATH:-}") ;;
    fastdds) e+=(ZERODDS_BENCH_SECURITY=1 ZERODDS_BENCH_SEC_DIR=$PROFILES_DIR/$prof/text ZERODDS_BENCH_SEC_NAME=$role
                 "LD_LIBRARY_PATH=$FASTDDS/lib:${LD_LIBRARY_PATH:-}") ;;
    zerodds) e+=(ZERODDS_BENCH_SECURITY=1 ZERODDS_BENCH_SEC_DIR=$PROFILES_DIR/$prof/text ZERODDS_BENCH_SEC_NAME=$role
                 "LD_LIBRARY_PATH=$DYLIB:${LD_LIBRARY_PATH:-}") ;;
  esac
  # FastDDS gates crypto-token reciprocation on ZeroDDS' reliable secure-SPDP
  # channel (0xff0101); only enable it when ZeroDDS talks to a fastdds peer.
  if [ "$v" = zerodds ] && [ "$peer" = fastdds ]; then e+=(ZERODDS_SECURE_SPDP=1); fi
  env "${e[@]}" "$b" "${a[@]}" > "$log" 2>&1 & echo $!
}

cell() { # ping_v pong_v profile type
  local pv="$1" gv="$2" prof="$3" ty="$4"
  [ -x "$(bin "$pv")" ] && [ -x "$(bin "$gv")" ] || { printf "%-8s->%-8s %-10s %-10s : SKIP(no-bin)\n" "$pv" "$gv" "$prof" "$ty"; return; }
  local gl="/tmp/oracle-sec-xtypes/${prof}_${ty}_${pv}_${gv}_pong.log"
  local pl="/tmp/oracle-sec-xtypes/${prof}_${ty}_${pv}_${gv}_ping.log"
  mkdir -p /tmp/oracle-sec-xtypes
  pkill -9 -f -- "-secxt" 2>/dev/null; sleep 1
  local pp; pp=$(start "$gv" pong "$prof" "$ty" "$gl" "$pv" pong --type "$ty" --secs 25); sleep 2
  local gp; gp=$(start "$pv" ping "$prof" "$ty" "$pl" "$gv" ping --type "$ty" --samples "$SAMPLES")
  for i in $(seq 1 40); do kill -0 "$gp" 2>/dev/null || break; sleep 1; done
  kill "$pp" 2>/dev/null; pkill -9 -f -- "-secxt" 2>/dev/null; sleep 1
  local r
  if grep -q "PROOF_OK" "$pl" 2>/dev/null; then r="$(grep -o 'PROOF_OK.*' "$pl" | head -1)"
  elif grep -q "PROOF_FAIL" "$pl" 2>/dev/null; then r="$(grep -o 'PROOF_FAIL.*' "$pl" | head -1)"
  # Vendor security-error strings only -- do NOT match our own "secure echo" banner.
  elif grep -qiE "PKCS7|check_create_participant denied|No governance exists|permissions grant|access rule|Permissions\.cpp|security setter|security_config_create failed" "$pl" "$gl" 2>/dev/null; then r=SEC_FAIL
  # Peer-side decode/deser rejection of the (decrypted) XTypes body -- the
  # "crypto correct but XTypes framing differs" signal.
  elif grep -qiE "deserialization .* failed|decode_fail=[1-9]" "$gl" "$pl" 2>/dev/null; then r=XTYPES_DECODE_FAIL
  elif grep -qiE "match timeout|wait_for_peers timeout" "$pl" "$gl" 2>/dev/null; then r=NO_MATCH
  else r=FAIL; fi
  printf "%-8s->%-8s %-10s %-10s : %s\n" "$pv" "$gv" "$prof" "$ty" "$r"
}

for prof in "${PROFILES[@]}"; do
  echo "######## PROFILE: $prof ########"
  for ty in "${TYPES[@]}"; do
    for pv in "${VENDORS[@]}"; do for gv in "${VENDORS[@]}"; do cell "$pv" "$gv" "$prof" "$ty"; done; done
  done
done
echo "=== secure XTypes matrix done ==="
