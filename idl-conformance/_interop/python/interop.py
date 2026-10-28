#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""idl-conformance / _interop / python — the combo::Telemetry column.

    interop.py ENCODE <out.bin>   write the canonical sample as raw XCDR2-LE
    interop.py DECODE <in.bin>    decode, assert every field == canonical

combo.py is `zerodds-idlc generate --lang python` output of ../combo.idl over
runtime/python (zerodds_wire); values are CANONICAL.md.
"""

from __future__ import annotations

import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import combo as generated  # noqa: E402  (zerodds-idlc generate --lang python)
from combo import combo  # noqa: E402
from zerodds_wire import Endianness, Reader, Status, TryConstruct, Writer  # noqa: E402

CANONICAL = combo.Telemetry(
    unitId=4242,
    region="eu-central-1",
    mode=int(combo.Mode.MODE_ACTIVE),
    batteryCurrent=13.75,
    history=[combo.Sample(seq=1, value=0.5),
             combo.Sample(seq=2, value=1.5),
             combo.Sample(seq=3, value=-2.25)],
    reading=combo.Reading(_d=int(combo.Mode.MODE_ACTIVE), activeRate=60.0),
    counters=[("drops", 3), ("packets", 100)],  # map<string,long>, key-sorted
    calibration=0.001,
    window=(10, 20, 30, 40),
)


def normalise(value):
    """The decoded form of a value: tuples of an array come back as lists."""
    if isinstance(value, (list, tuple)):
        return [normalise(v) for v in value]
    if hasattr(value, "__dataclass_fields__"):
        return {name: normalise(getattr(value, name)) for name in value.__dataclass_fields__}
    return value


def encode(sample: combo.Telemetry) -> bytes:
    w = Writer(Endianness.LITTLE, max_align=4)
    generated.zd_enc_combo_Telemetry(w, sample)
    return w.finish()


def decode(data: bytes) -> combo.Telemetry:
    r = Reader(data, Endianness.LITTLE, max_align=4)
    value = generated.zd_dec_combo_Telemetry(r, TryConstruct.DISCARD)
    if r.reader_status() is not Status.OK:
        raise ValueError(f"the decoder refused the stream: {r.reader_status()}")
    if r.remaining() != 0:
        raise ValueError(f"{r.remaining()} bytes left after the sample")
    return value


def main() -> int:
    if len(sys.argv) != 3 or sys.argv[1] not in ("ENCODE", "DECODE"):
        print("usage: interop.py ENCODE <out.bin> | DECODE <in.bin>", file=sys.stderr)
        return 2
    mode, path = sys.argv[1], Path(sys.argv[2])
    if mode == "ENCODE":
        data = encode(CANONICAL)
        path.write_bytes(data)
        print(f"ENCODE OK: {len(data)} bytes -> {path}")
        return 0
    got = decode(path.read_bytes())
    want, have = normalise(CANONICAL), normalise(got)
    diffs = [f"  [FAIL] {k}: want {want[k]!r} got {have[k]!r}" for k in want if want[k] != have[k]]
    if diffs:
        print("\n".join(diffs), file=sys.stderr)
        print(f"DECODE FAILED: {len(diffs)} mismatch(es)")
        return 1
    print("DECODE OK: every field equals canonical")
    return 0


if __name__ == "__main__":
    sys.exit(main())
