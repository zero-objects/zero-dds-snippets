"""idl-conformance / _interop / python — per-feature XCDR2-LE golden producer.

    python3 features_interop.py ENCODE   -> ../goldens/<feature>.python.bin
    python3 features_interop.py DECODE   -> decode ../goldens/<feature>.golden.bin,
                                            compare with the canonical sample

features.py is `zerodds-idlc generate --lang python` output over
runtime/python (zerodds_wire); values are CANONICAL.md.
"""

from __future__ import annotations

import os
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import features  # noqa: E402  (zerodds-idlc generate --lang python)
from features import feat  # noqa: E402
from zerodds_wire import Endianness, Reader, Status, TryConstruct, Writer  # noqa: E402

GOLDENS = Path(os.environ.get("GOLDENS", HERE.parent / "goldens"))

# ---- canonical samples (CANONICAL.md) ------------------------------------


def pt(x: int, y: int) -> feat.Pt:
    return feat.Pt(x=x, y=y)


def tree(value: int, *kids: feat.Tree) -> feat.Tree:
    return feat.Tree(value=value, kids=list(kids))


CANONICAL = {
    "wstr": ("WStr", feat.WStr(label="café", text="日本語🎉")),
    "mut": ("Mut", feat.Mut(a=1000000, b=2.5, c="ok")),
    "mutnest": ("MutNest", feat.MutNest(
        tag=9, leaf=feat.MutLeaf(u=100, v=1.25),
        list_=[feat.MutLeaf(u=1, v=0.5), feat.MutLeaf(u=2, v=0.25)])),
    "outerkey": ("OuterKey", feat.OuterKey(k=feat.NestedKey(hi=0x01020304, lo=0x05060708), payload=999)),
    "bits": ("Bits", feat.Bits(perm=int(feat.Perm.READ | feat.Perm.EXEC), flags=feat.Flags(kind=5, prio=20))),
    "tree": ("Tree", tree(1, tree(2, tree(4)), tree(3))),
    "arr": ("Arr", feat.Arr(grid=((10, 11, 12), (13, 14, 15)), shape=(pt(1, 2), pt(3, 4)))),
    "prim": ("Prim", feat.Prim(
        i8=-128, u8=255, i16=-32768, u16=65535, i32=-2147483648, u32=4294967295,
        i64=-9223372036854775808, u64=0xFFFFFFFFFFFFFFFF, f32=3.5, f64=-1234.5,
        b=True, o=0xAB, ch="Z")),
    # @bit_bound enum + map<long,Pt> + sequence<Sel>: h=BLUE; m={3:{11,12}}; sels=[Sel{_d=2,n=9}].
    "mapenum": ("MapEnum", feat.MapEnum(
        h=int(feat.Hue.H_BLUE), m=[(3, pt(11, 12))], sels=[feat.Sel(_d=2, n=9)])),
    # map<long,long> primitive-valued, no collection DHEADER (XCDR2 §7.4.3.5): m={7:42,8:99}.
    "mapprim": ("MapPrim", feat.MapPrim(m=[(7, 42), (8, 99)])),
    # fixed<P,S> as its decimal text: price=123.45 (wire 12 34 5c), qty=1234 (01 23 4c).
    "fixed": ("Money", feat.Money(price="123.45", qty="1234")),
}


def normalise(value):
    """The decoded form of a value: tuples of an array come back as lists."""
    if isinstance(value, (list, tuple)):
        return [normalise(v) for v in value]
    if hasattr(value, "__dataclass_fields__"):
        return {name: normalise(getattr(value, name)) for name in value.__dataclass_fields__}
    return value


def encode(type_name: str, sample) -> bytes:
    w = Writer(Endianness.LITTLE, max_align=4)
    getattr(features, f"zd_enc_feat_{type_name}")(w, sample)
    return w.finish()


def decode(type_name: str, data: bytes):
    r = Reader(data, Endianness.LITTLE, max_align=4)
    value = getattr(features, f"zd_dec_feat_{type_name}")(r, TryConstruct.DISCARD)
    if r.reader_status() is not Status.OK:
        raise ValueError(f"the decoder refused the stream: {r.reader_status()}")
    if r.remaining() != 0:
        raise ValueError(f"{r.remaining()} bytes left after the sample")
    return value


# proofs/endianness's goldens: the Rust column's encodings of the same samples
# in XCDR2 and XCDR1, little- and big-endian. XCDR2 big-endian reads with the
# codec above; XCDR1 with x1/features.py, features.idl generated with
# `--encoding xcdr1`.
REPRESENTATIONS = Path(os.environ.get(
    "REPRESENTATIONS", HERE.parent.parent / "proofs" / "endianness" / "goldens"))
REPRESENTED = ("wstr", "mut", "bits", "tree", "arr", "prim", "mapenum")


def decode_with(module, type_name: str, data: bytes, endianness, max_align: int):
    r = Reader(data, endianness, max_align=max_align)
    value = getattr(module, f"zd_dec_feat_{type_name}")(r, TryConstruct.DISCARD)
    if r.reader_status() is not Status.OK:
        raise ValueError(f"the decoder refused the stream: {r.reader_status()}")
    if r.remaining() != 0:
        raise ValueError(f"{r.remaining()} bytes left after the sample")
    return value


def decode_representations() -> int:
    import importlib.util
    spec = importlib.util.spec_from_file_location("features_xcdr1", HERE / "x1" / "features.py")
    xcdr1 = importlib.util.module_from_spec(spec)
    sys.modules["features_xcdr1"] = xcdr1
    spec.loader.exec_module(xcdr1)
    failures = 0
    for feature in REPRESENTED:
        type_name, sample = CANONICAL[feature]
        for tag, module, endianness, max_align in (
            ("xcdr2-be", features, Endianness.BIG, 4),
            ("xcdr1-le", xcdr1, Endianness.LITTLE, 8),
            ("xcdr1-be", xcdr1, Endianness.BIG, 8),
        ):
            data = (REPRESENTATIONS / f"{feature}.{tag}.rust.bin").read_bytes()
            got = decode_with(module, type_name, data, endianness, max_align)
            if normalise(got) != normalise(sample):
                failures += 1
                print(f"  [FAIL] {feature} {tag}: {got!r}", file=sys.stderr)
    if failures:
        print(f"REPRESENTATIONS FAILED: {failures} mismatch(es)")
        return 1
    print("REPRESENTATIONS OK: xcdr2-be, xcdr1-le and xcdr1-be decode to the canonical samples")
    return 0


def main() -> int:
    mode = sys.argv[1] if len(sys.argv) > 1 else "ENCODE"
    if mode == "REPRESENTATIONS":
        return decode_representations()
    if mode == "ENCODE":
        for feature, (type_name, sample) in CANONICAL.items():
            data = encode(type_name, sample)
            (GOLDENS / f"{feature}.python.bin").write_bytes(data)
            print(f"ENCODE {feature}: {len(data)} bytes")
        return 0
    if mode == "DECODE":
        failures = 0
        for feature, (type_name, sample) in CANONICAL.items():
            got = decode(type_name, (GOLDENS / f"{feature}.golden.bin").read_bytes())
            if normalise(got) != normalise(sample):
                failures += 1
                print(f"  [FAIL] {feature}: {got!r}", file=sys.stderr)
        if failures:
            print(f"DECODE FAILED: {failures} mismatch(es)")
            return 1
        print("DECODE OK: every golden decodes to the canonical sample")
        return 0
    print("usage: ENCODE | DECODE", file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main())
