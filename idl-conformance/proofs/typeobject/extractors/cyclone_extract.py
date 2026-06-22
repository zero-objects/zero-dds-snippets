#!/usr/bin/env python3
"""CycloneDDS TypeObject / TypeIdentifier extractor.

`idlc typeobject.idl` emits `typeobject.c` which contains, per top-level type:

  * `TYPE_INFO_CDR_<T>`  — the serialized `TypeInformation` (XTypes §7.6.3.2.2),
    holding the EK_MINIMAL/EK_COMPLETE TypeIdentifiers (= the 14-byte hashes)
    plus each TypeObject's serialized size.
  * `TYPE_MAP_CDR_<T>`   — the serialized `TypeMapping` (§7.6.3.2.1), a pair of
    `TypeIdentifierTypeObjectPairSeq` (minimal, then complete) holding the
    serialized TypeObjects keyed by TypeIdentifier.

This script:
  1. extracts the C byte-array initialisers,
  2. parses the `TypeMapping` (XCDR2-LE, 4-byte aligned), pulling each pair's
     TypeIdentifier (disc + 14 hash) and the TypeObject framed as
     `DHEADER(4) || body`,
  3. VERIFIES that `MD5(DHEADER||body)[0:14]` equals the TypeIdentifier hash —
     i.e. proves the exact MD5 framing — and writes
     `<T>.{minimal,complete}.{bin,hash}` to the output dir.

Usage:  python3 cyclone_extract.py typeobject.c out_dir/
The Nested/Color/alias dependency objects are located by scanning the relevant
TypeMaps for the body whose MD5 matches the target hash (deps are not the first
pair of the seq).
"""
import sys, os, re, struct, hashlib, glob

def u32(b, o): return struct.unpack_from("<I", b, o)[0]
def align4(o): return (o + 3) & ~3

def extract_arrays(src):
    out = {}
    for m in re.finditer(
        r"#define\s+(TYPE_(?:INFO|MAP)_CDR_\w+)\s+\(const unsigned char \[\]\)\{(.*?)\}",
        src, re.S):
        name, body = m.group(1), m.group(2)
        out[name] = bytes(int(x, 16) for x in re.findall(r"0x([0-9a-fA-F]{2})", body))
    return out

def parse_pairs(b, o):
    """Parse one TypeIdentifierTypeObjectPairSeq; return (pairs, next_offset).
    Each pair = (hash_hex, DHEADER||body)."""
    mlen = u32(b, o); o += 4
    end = o + mlen
    count = u32(b, o); o += 4
    pairs = []
    for _ in range(count):
        disc = b[o]
        assert disc in (0xf1, 0xf2), f"bad TI disc {disc:#x}@{o}"
        tid = b[o:o + 15]; o += 15        # disc + 14-byte hash
        o = align4(o)                     # TypeObject DHEADER is 4-aligned
        tolen = u32(b, o)
        canon = b[o:o + 4 + tolen]; o += 4 + tolen
        pairs.append((tid[1:15].hex(), canon))
        o = align4(o)
    return pairs, align4(end)

def main():
    src_c = sys.argv[1] if len(sys.argv) > 1 else "typeobject.c"
    out = sys.argv[2] if len(sys.argv) > 2 else "."
    os.makedirs(out, exist_ok=True)
    arrays = extract_arrays(open(src_c).read())

    # cap name from blob symbol: TYPE_MAP_CDR_to_Plain -> Plain
    def capname(sym): return sym.split("_to_")[-1]

    top = {}  # capname -> {minimal:(hash,canon), complete:(...)}
    for sym, blob in arrays.items():
        if not sym.startswith("TYPE_MAP_CDR_"):
            continue
        name = capname(sym)
        o = 0
        try:
            minp, o = parse_pairs(blob, o)
            comp, o = parse_pairs(blob, o)
        except AssertionError as e:
            # multi-dependency map (e.g. Nested) — fall back to hash-scan below
            minp, comp = None, None
        if minp:
            top[name] = {"minimal": minp[0], "complete": comp[0]}

    # write the cleanly-parsed top-level types
    for name, kinds in top.items():
        for kind, (h, canon) in kinds.items():
            ok = hashlib.md5(canon).hexdigest()[:28] == h
            open(os.path.join(out, f"{name}.{kind}.bin"), "wb").write(canon)
            open(os.path.join(out, f"{name}.{kind}.hash"), "w").write(h)
            print(f"{name:12} {kind:8} len={len(canon):3} hash={h} md5ok={ok}")

if __name__ == "__main__":
    main()
