#!/usr/bin/env python3
"""Endianness × XCDR-version wire-proof comparator.

Builds the repr × backend × vendor byte table:

  * SELF-CONSISTENCY — for the ZeroDDS reference (Rust) every sample is encoded
    in all four representations (XCDR2-LE/BE, XCDR1-LE/BE); the existing
    `_interop` corpus already shows all 7 ZeroDDS language backends converge on
    the XCDR2-LE bytes, and this proof shows the Rust reference (which the other
    6 conform to) produces the other three reprs through the same codec.
  * VENDOR BYTE-IDENTITY — each ZeroDDS golden is byte-compared against the
    CycloneDDS 0.11 and eProsima FastCDR/FastDDS goldens for the same
    (feature, repr).

Run after `make goldens` (ZeroDDS) and `make vendors` (codepit) have populated
`goldens/`, `vendor-cyclone/`, `vendor-fastcdr/`.
"""
import hashlib
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
FEATS = ["wstr", "mut", "bits", "tree", "arr", "prim", "mapenum"]
REPRS = ["xcdr2-le", "xcdr2-be", "xcdr1-le", "xcdr1-be"]


def rd(p):
    return open(p, "rb").read() if os.path.exists(p) else None


def sha(b):
    return hashlib.sha256(b).hexdigest()[:12] if b is not None else "-"


def cell(z, v):
    if v is None:
        return "n/a"  # vendor cannot emit this (feature/repr unsupported)
    return "MATCH" if z == v else "DIFF"


def main():
    print(f"{'feature':7} {'repr':9} {'zerodds':>16} {'cyclone':>7} {'fastdds':>7}")
    print("-" * 52)
    diffs = []
    for f in FEATS:
        for r in REPRS:
            z = rd(f"{HERE}/goldens/{f}.{r}.rust.bin")
            c = rd(f"{HERE}/vendor-cyclone/{f}.{r}.cyclone.bin")
            v = rd(f"{HERE}/vendor-fastcdr/{f}.{r}.fastdds.bin")
            if z is None:
                print(f"{f:7} {r:9} {'MISSING':>16}")
                continue
            vc, vf = cell(z, c), cell(z, v)
            print(f"{f:7} {r:9} {len(z):>3}/{sha(z)} {vc:>7} {vf:>7}")
            if vc == "DIFF":
                diffs.append((f, r, "cyclone", z, c))
            if vf == "DIFF":
                diffs.append((f, r, "fastdds", z, v))
    if diffs:
        print("\n=== DIFF detail (offset + 16-byte windows) ===")
        for f, r, who, z, v in diffs:
            n = min(len(z), len(v))
            off = next((i for i in range(n) if z[i] != v[i]), n)
            print(f"{f}.{r} vs {who}: len Z={len(z)} {who}={len(v)}  first-diff@{off}")
            print(f"   ZeroDDS [{off}:] = {z[off:off+16].hex(' ')}")
            print(f"   {who:7}[{off}:] = {v[off:off+16].hex(' ')}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
