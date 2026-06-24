"""XCDR1 DECODE conformance: decode the ZeroDDS-Rust XCDR1 (classic CDR /
PL_CDR1) reference goldens with the Python binding and assert every field
equals the canonical sample. Proves the Python decoder reads the XCDR1 wire
(no DHEADER on @appendable/@final, max-align 8, PL_CDR1 for @mutable) — the
counterpart of be_roundtrip.py for the second wire representation."""
import os, sys
_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", "..", "..", ".."))
sys.path.insert(0, os.path.join(_REPO, "crates", "py", "python"))
sys.path.insert(0, _HERE)
import features_interop as fi

_GOLDENS = os.path.join(_HERE, "..", "..", "proofs", "endianness", "goldens")

# Every feature that has an XCDR1-LE golden (arr/bits/mapenum/mut/prim/tree/wstr).
cases = [
    ("wstr", fi.canonical_wstr()),    # @appendable
    ("mut", fi.canonical_mut()),      # @mutable -> PL_CDR1
    ("mapenum", fi.canonical_mapenum()),
    ("tree", fi.canonical_tree()),
    ("arr", fi.canonical_arr()),
    ("prim", fi.canonical_prim()),    # @appendable + 8-byte members (align 8)
]

fail = 0
for name, sample in cases:
    path = os.path.join(_GOLDENS, f"{name}.xcdr1-le.rust.bin")
    if not os.path.exists(path):
        print(f"  {name} xcdr1: SKIP (no golden)")
        continue
    with open(path, "rb") as f:
        data = f.read()
    cls = type(sample)
    # DECODE: representation=0 selects the XCDR1 decode path.
    back = cls.decode(data, "le", 0)
    ok = fi.equal(back, sample)
    print(f"  {name} xcdr1 decode: {'OK' if ok else 'FAIL'}")
    if not ok:
        fail = 1
        print(f"    want {sample}\n    got  {back}")
    # ENCODE: encode(.., representation=0) must be byte-identical to the golden.
    wire = sample.encode("le", 0)
    enc_ok = wire == data
    print(f"  {name} xcdr1 encode: {'OK' if enc_ok else 'FAIL'}"
          f" ({len(wire)}B vs {len(data)}B golden)")
    if not enc_ok:
        fail = 1

sys.exit(fail)
