#!/usr/bin/env bash
# zerodds-webdds-bridged between HTTP and DDS: samples written over REST and
# SOAP arrive at an ordinary ZeroDDS program, and what that program writes
# is read back over REST.
#
#   ./run.sh            (ZERODDS_EXAMPLE_DOMAIN, WEBDDS_PORT to change them)
set -uo pipefail
HERE=$(cd "$(dirname "$0")" && pwd)
export ZERODDS_EXAMPLE_DOMAIN=${ZERODDS_EXAMPLE_DOMAIN:-0}
PORT=${WEBDDS_PORT:-8080}
# The daemon from the tree, the DDS program in its own target directory: it
# is a separate workspace, and one target directory for both mixes their
# builds of the same crates.
(cd "$HERE/../.." && cargo build -q --release -p zerodds-webdds-bridge) || exit 1
(cd "$HERE" && cargo build -q --release --target-dir "$HERE/target") || exit 1
BRIDGED=${CARGO_TARGET_DIR:-$HERE/../../target}/release/zerodds-webdds-bridged
SIDE=$HERE/target/release/dds_side
out=$(mktemp -d)

"$BRIDGED" --listen "127.0.0.1:$PORT" --domain "$ZERODDS_EXAMPLE_DOMAIN" \
  --type "Demo::Sensor=$HERE/idl/Sensor.idl" > "$out/bridged.log" 2>&1 &
bridged=$!
trap 'kill $bridged 2>/dev/null' EXIT
sleep 1

API=http://127.0.0.1:$PORT/dds/rest1
ACCEPT='Accept: application/dds-web+xml'
CTYPE='Content-Type: application/dds-web+xml'
P=$API/applications/app1/domain_participants/p1
rest() { # METHOD URL [BODY] -> status code
  if [ $# -eq 3 ]; then
    curl -s -o "$out/body" -w '%{http_code}' -X "$1" "$2" -H "$ACCEPT" -H "$CTYPE" -d "$3"
  else
    curl -s -o "$out/body" -w '%{http_code}' -X "$1" "$2" -H "$ACCEPT"
  fi
}
expect() { # WANT GOT WHAT
  if [ "$1" != "$2" ]; then
    echo "WEBDDS GATEWAY FAILED: $3 answered $2, not $1: $(cat "$out/body")"
    exit 1
  fi
}

expect 201 "$(rest POST "$API/applications" '<application><name>app1</name></application>')" application
expect 201 "$(rest POST "$API/applications/app1/domain_participants" \
  "<domain_participant><domain_id>$ZERODDS_EXAMPLE_DOMAIN</domain_id></domain_participant>")" participant
expect 201 "$(rest POST "$P/topics" '<topic><name>Sensor</name><type_name>Demo::Sensor</type_name></topic>')" topic
expect 201 "$(rest POST "$P/publishers")" publisher
expect 201 "$(rest POST "$P/publishers/pub1/data_writers" '<data_writer><topic_name>Sensor</topic_name></data_writer>')" writer
expect 201 "$(rest POST "$P/subscribers")" subscriber
expect 201 "$(rest POST "$P/subscribers/sub1/data_readers" '<data_reader><topic_name>Sensor</topic_name></data_reader>')" reader

# HTTP -> DDS: five samples over REST, one over SOAP.
"$SIDE" sub 6 > "$out/sub.log" 2>&1 &
sub=$!
sleep 2
for id in 1 2 3 4 5; do
  expect 200 "$(rest POST "$P/publishers/pub1/data_writers/dw1" \
    "<data><sensor_id>$id</sensor_id><value>$id.5</value></data>")" "write $id"
done
curl -s -o "$out/body" -X POST "http://127.0.0.1:$PORT/soap" \
  -H 'Content-Type: application/soap+xml' \
  -d '<soap:Envelope xmlns:soap="http://www.w3.org/2003/05/soap-envelope"><soap:Body>
        <WriteSample><app>app1</app><participant>p1</participant><publisher>pub1</publisher>
          <writer>dw1</writer><data><sensor_id>6</sensor_id><value>6.5</value></data></WriteSample>
      </soap:Body></soap:Envelope>'
grep -q WriteSampleResponse "$out/body" || { echo "WEBDDS GATEWAY FAILED: SOAP WriteSample: $(cat "$out/body")"; exit 1; }
wait $sub
to_dds=$(grep -c "sensor_id=[1-6] value=[1-6].5" "$out/sub.log")

# DDS -> HTTP: the program writes three samples, REST reads them.
"$SIDE" pub 3 > "$out/pub.log" 2>&1 || { cat "$out/pub.log"; exit 1; }
expect 200 "$(rest GET "$P/subscribers/sub1/data_readers/dr1")" read
to_http=$(grep -o '<sensor_id>10[0-2]</sensor_id>' "$out/body" | wc -l)

grep "sensor_id=6 " "$out/sub.log"; tail -1 "$out/sub.log"; tail -1 "$out/pub.log"
grep -o '<data><sensor_id>100</sensor_id>[^/]*</value></data>' "$out/body"
if [ "$to_dds" -eq 6 ] && [ "$to_http" -eq 3 ]; then
  echo "WEBDDS GATEWAY OK: HTTP->DDS $to_dds of 6, DDS->HTTP $to_http of 3"
  rm -rf "$out"
else
  echo "WEBDDS GATEWAY FAILED: HTTP->DDS $to_dds of 6, DDS->HTTP $to_http of 3 (logs in $out)"
  exit 1
fi
