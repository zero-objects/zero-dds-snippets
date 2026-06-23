import os, sys
_HERE = os.path.dirname(os.path.abspath(__file__))
_REPO = os.path.abspath(os.path.join(_HERE, "..", "..", "..", ".."))
sys.path.insert(0, os.path.join(_REPO, "crates", "py", "python"))
sys.path.insert(0, _HERE)
import features_interop as fi

cases = [
    ("wstr", fi.canonical_wstr()), ("mut", fi.canonical_mut()),
    ("mapenum", fi.canonical_mapenum()), ("mutnest", fi.canonical_mutnest()),
    ("tree", fi.canonical_tree()), ("arr", fi.canonical_arr()),
    ("prim", fi.canonical_prim()), ("outerkey", fi.canonical_outerkey()),
]
fail = 0
for name, sample in cases:
    cls = type(sample)
    for endian in ("le", "be"):
        data = sample.encode(endian)
        back = cls.decode(data, endian)
        ok = fi.equal(back, sample)
        print(f"  {name} {endian}: {'OK' if ok else 'FAIL'}")
        if not ok:
            fail = 1
            print(f"    want {sample}\n    got  {back}")
sys.exit(fail)
