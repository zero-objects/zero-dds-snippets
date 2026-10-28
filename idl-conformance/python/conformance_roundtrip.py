#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""IDL-conformance DCPS roundtrip for the ZeroDDS Python PSM.

Publishes fully-populated samples of `combo::Telemetry` (`combo.idl`) over a
real DomainParticipant with a publisher and a subscriber, reads each back and
asserts the recovered sample equals the one written.

`combo.py` is `zerodds-idlc generate --lang python combo.idl` output — the
dataclasses, their codec over zerodds_wire and ZD_TOPIC_DESCRIPTORS; `run.sh`
generates it on every run. One keyed topic exercises, in each sample:

  enum member · nested struct · sequence of struct · bounded string (@key) ·
  union as a struct member (enum discriminator) · map<string,long> ·
  @optional member (present and absent) · fixed-size array (long[4]) ·
  typedef · @key · @appendable extensibility.
"""

import sys

import zerodds
from combo import ZD_TOPIC_DESCRIPTORS, combo


def section(title):
    print("=" * 64)
    print(title)
    print("=" * 64)


def telemetry(calibration, reading):
    """A fully-populated Telemetry; `calibration` is the @optional member."""
    return combo.Telemetry(
        unitId=42,
        region="eu-central-mission-1",
        mode=combo.Mode.MODE_ACTIVE,
        batteryCurrent=3.14159,                   # typedef CurrentInAmpsType
        history=[combo.Sample(seq=1, value=1.5),  # sequence<struct>
                 combo.Sample(seq=2, value=2.5),
                 combo.Sample(seq=3, value=-9.0)],
        reading=reading,                          # union member
        # map<string,long>, in the ascending key order the wire carries
        counters=[("drops", 0), ("frames", 1000), ("retries", 7)],
        calibration=calibration,                  # @optional double
        window=(10, 20, 30, 40),                  # long window[4]
    )


def roundtrip(writer, reader, label, sent):
    print(f"PUB ({label}):", sent)
    writer.write(sent)
    reader.wait_for_data(3.0)
    got = reader.take()
    assert got, f"no Telemetry sample received ({label})"
    recovered = got[0]
    print(f"SUB ({label}):", recovered)
    assert recovered == sent, f"Telemetry mismatch:\n  sent={sent}\n  got ={recovered}"
    return recovered


def main():
    participant = zerodds.DomainParticipantFactory.instance().create_participant(0)

    section("Topic: combo::Telemetry")
    topic = zerodds.IdlTopic(participant, "ConformanceTelemetry",
                             ZD_TOPIC_DESCRIPTORS["combo::Telemetry"])
    writer = topic.create_writer(participant.create_publisher())
    reader = topic.create_reader(participant.create_subscriber())
    writer.wait_for_matched_subscription(1, 5.0)
    reader.wait_for_matched_publication(1, 5.0)

    # @optional present, union case MODE_ACTIVE (double activeRate).
    first = roundtrip(writer, reader, "optional present, union MODE_ACTIVE",
                      telemetry(0.999, combo.Reading(_d=combo.Mode.MODE_ACTIVE, activeRate=9.81)))
    assert first.reading.activeRate == 9.81 and first.reading.idleTicks is None
    assert first.window == (10, 20, 30, 40)
    assert first.calibration == 0.999

    # @optional absent, union default case (string<16> faultCode).
    second = roundtrip(writer, reader, "optional absent, union default",
                       telemetry(None, combo.Reading(_d=combo.Mode.MODE_FAULT, faultCode="E_OVERHEAT")))
    assert second.calibration is None
    assert second.reading.faultCode == "E_OVERHEAT" and second.reading.activeRate is None

    print("OK: enum + nested struct + seq<struct> + bounded string + "
          "union member + map + @optional (present/absent) + fixed array + "
          "typedef + @key survived the wire in one topic.\n")
    section("ALL ROUNDTRIPS PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
