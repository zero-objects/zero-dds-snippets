#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""XCDR2 wire-level interop: encode/decode the canonical combo::Telemetry sample.

Usage:
    interop.py ENCODE <out.bin>     # write raw XCDR2-LE bytes of canonical sample
    interop.py DECODE <in.bin>      # read bytes, reconstruct, assert ==canonical
"""
import sys
from importlib import import_module

_combo = import_module("combo")
Mode = _combo.combo_Mode
Sample = _combo.combo_Sample
Reading = _combo.combo_Reading
Telemetry = _combo.combo_Telemetry


def canonical():
    return Telemetry(
        unitId=4242,
        region="eu-central-1",
        mode=Mode.MODE_ACTIVE,
        batteryCurrent=13.75,
        history=[Sample(seq=1, value=0.5),
                 Sample(seq=2, value=1.5),
                 Sample(seq=3, value=-2.25)],
        reading=Reading.make(Mode.MODE_ACTIVE, 60.0),
        counters={"drops": 3, "packets": 100},
        calibration=0.001,
        window=[10, 20, 30, 40],
    )


def diff(sent, got):
    out = []
    for f in ("unitId", "region", "mode", "batteryCurrent", "history",
              "reading", "counters", "calibration", "window"):
        a = getattr(sent, f)
        b = getattr(got, f)
        if f == "reading":
            eq = (a.discriminator == b.discriminator and a.value == b.value)
            a = (a.discriminator, a.value)
            b = (b.discriminator, b.value)
        else:
            eq = (a == b)
        if not eq:
            out.append(f"  {f}: sent={a!r} got={b!r}")
    return out


def assert_equal(sent, got):
    d = diff(sent, got)
    if d:
        print("MISMATCH:")
        print("\n".join(d))
        return False
    return True


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    mode, path = sys.argv[1], sys.argv[2]
    sample = canonical()

    if mode == "ENCODE":
        data = sample.encode()
        with open(path, "wb") as f:
            f.write(data)
        print(f"ENCODE: wrote {len(data)} bytes to {path}")
        return 0

    if mode == "DECODE":
        with open(path, "rb") as f:
            data = f.read()
        got = Telemetry.decode(data)
        if assert_equal(sample, got):
            print(f"DECODE: OK ({len(data)} bytes) — all fields == canonical")
            return 0
        return 1

    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
