# IDL Conformance — Real-DCPS Roundtrip Examples

One standalone, runnable example per ZeroDDS language binding (PSM). Each publishes a fully-populated sample of combined IDL features over a **real DCPS participant + pub/sub loopback** and asserts the sample survives the wire. Generated from the shared conformance fixtures in `tools/idlc/tests/conformance/fixtures/` via the pre-built `zerodds-idlc`.

These were authored to exercise the codegen + runtime end-to-end; the gaps each one documents are real bugs catalogued in `internal/idl-codegen/real-dcps-findings.md`. (The other `zerodds-examples/*` dirs are website snippet references and are unrelated.)

| Example | Roundtrip | Features verified | How to run |
|---|---|---|---|
| [rust](rust/) | ✅ | 10 | `cd rust && cargo run` |
| [cpp](cpp/) | ✅ | 10 | `cd cpp && ./build.sh && ./combo_roundtrip` |
| [python](python/) | ✅ | 11 | `cd python && PYTHONPATH=../../../crates/py/python python3 conformance_roundtrip.py` |
| [java](java/) | ✅ | 9 | `cd java && ./build-and-run.sh` |
| [typescript](typescript/) | ✅ | 9 | `cd typescript && npx tsx ...` (see its README) |
| [csharp](csharp/) | ✅ | 10 | `cd csharp && dotnet run` |
| [c](c/) | ✅ | 8 | `cd c && ./build.sh` (see its README) |

Each sub-dir has its own README with the exact build/run commands and a **Known limitations** section listing the features it could not round-trip (and why). For the full cross-backend bug catalogue see `internal/idl-codegen/real-dcps-findings.md` in the main repo.

