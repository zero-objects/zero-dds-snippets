#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""IDL-conformance DCPS roundtrip for the ZeroDDS Python PSM.

Publishes ONE fully-populated sample of the combined conformance type over a
REAL DomainParticipant + publisher/subscriber loopback, then reads it back and
asserts the recovered sample equals what was published.

Topic
-----
`combo::Telemetry` — the unmodified canonical conformance fixture
`20_mixed_combo.idl` (`module combo { ... }`), generated *as-is* by
`zerodds-idlc --python` into `20_mixed_combo.py`.  It exercises, in a SINGLE
keyed topic over the wire:

  enum (member) · nested struct · sequence-of-struct · bounded string (@key) ·
  union-as-struct-member (enum discriminator) · map<string,long> ·
  @optional member (present + absent) · fixed-size array (Array[long,4]) ·
  typedef · @key · @appendable extensibility.

Earlier the module-wrapped fixture had to be hand-decomposed into three
separate topics because of real Python codegen/runtime bugs (module-flatten
scoped-name resolution, @optional drop, fixed-array brand loss,
union-as-struct-member rejected).  Those are now fixed, so the full
module-wrapped Telemetry imports and round-trips as a single topic.

See README.md "Known limitations" for the single genuine remaining gap.
"""

import sys

import zerodds

# Generated, UNMODIFIED, by `zerodds-idlc 20_mixed_combo.idl --python`.
# Names are module-flattened (combo:: -> combo_), and all intra-module
# references (union discriminator, field types, sequence/array elements,
# typedef target) resolve correctly.
from importlib import import_module

_combo = import_module("20_mixed_combo")
Mode = _combo.combo_Mode
Sample = _combo.combo_Sample
Reading = _combo.combo_Reading
Telemetry = _combo.combo_Telemetry


def _section(title):
    print("=" * 64)
    print(title)
    print("=" * 64)


def _make_telemetry(calibration):
    """Build a fully-populated Telemetry; `calibration` is the @optional member
    (a float when present, None when absent)."""
    return Telemetry(
        unitId=42,
        region="eu-central-mission-1",
        mode=Mode.MODE_ACTIVE,
        batteryCurrent=3.14159,                  # typedef CurrentInAmpsType
        history=[Sample(seq=1, value=1.5),       # sequence<struct>
                 Sample(seq=2, value=2.5),
                 Sample(seq=3, value=-9.0)],
        reading=Reading.make(Mode.MODE_ACTIVE, 9.81),   # union member, case 1
        counters={"frames": 1000, "drops": 0, "retries": 7},  # map<string,long>
        calibration=calibration,                 # @optional double
        window=[10, 20, 30, 40],                 # long window[4] (fixed array)
    )


def roundtrip_combo(participant):
    _section("Topic: combo::Telemetry (full module-wrapped conformance combo)")
    topic = zerodds.IdlTopic(participant, "ConformanceTelemetry", Telemetry)
    writer = topic.create_writer(participant.create_publisher())
    reader = topic.create_reader(participant.create_subscriber())

    writer.wait_for_matched_subscription(1, 5.0)
    reader.wait_for_matched_publication(1, 5.0)

    # --- @optional PRESENT case ---------------------------------------------
    sent = _make_telemetry(calibration=0.999)
    print("PUB (optional present):", sent)
    writer.write(sent)

    reader.wait_for_data(3.0)
    got = reader.take()
    assert got, "no Telemetry sample received (present case)"
    recovered = got[0]
    print("SUB (optional present):", recovered)
    assert recovered == sent, (
        f"Telemetry mismatch:\n  sent={sent}\n  got ={recovered}")

    # Spot-check every cross-feature interaction explicitly.
    assert recovered.mode is Mode.MODE_ACTIVE                 # enum member
    assert recovered.history[2].value == -9.0                 # seq<struct>
    assert recovered.counters["retries"] == 7                 # map
    assert recovered.batteryCurrent == 3.14159                # typedef
    # union-as-struct-member (active case = case 1, Float64):
    assert recovered.reading.discriminator is Mode.MODE_ACTIVE
    assert recovered.reading.value == 9.81
    # fixed-size array Array[Int32, 4] (reads back as a 4-element list):
    assert recovered.window == [10, 20, 30, 40]
    assert len(recovered.window) == 4
    # @optional present:
    assert recovered.calibration == 0.999

    # --- @optional ABSENT case ----------------------------------------------
    # Same topic, optional present-flag cleared.  Also flip the union to its
    # DEFAULT case (string<16> faultCode) to exercise the default arm.
    sent2 = _make_telemetry(calibration=None)
    sent2.reading = Reading.make(Mode.MODE_FAULT, "E_OVERHEAT")
    print("PUB (optional absent, union default):", sent2)
    writer.write(sent2)

    reader.wait_for_data(3.0)
    got2 = reader.take()
    assert got2, "no Telemetry sample received (absent case)"
    recovered2 = got2[0]
    print("SUB (optional absent, union default):", recovered2)
    assert recovered2 == sent2, (
        f"Telemetry mismatch:\n  sent={sent2}\n  got ={recovered2}")
    assert recovered2.calibration is None                     # @optional absent
    assert recovered2.reading.value == "E_OVERHEAT"           # union default arm

    print("OK: enum + nested struct + seq<struct> + bounded string + "
          "union-member + map + @optional(present/absent) + fixed array + "
          "typedef + @key all survived the wire in ONE module-wrapped topic.\n")


def main():
    factory = zerodds.DomainParticipantFactory.instance()
    participant = factory.create_participant(0)

    roundtrip_combo(participant)

    _section("ALL ROUNDTRIPS PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
