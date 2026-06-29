//! WIRE-level XCDR2 interop for the ZeroDDS Rust PSM.
//!
//! Reuses the generated `combo::Telemetry` codec (the SAME bodies DDS uses on
//! the wire) to:
//!   * ENCODE         -> write the canonical sample's raw XCDR2-LE bytes to the
//!                       shared golden file.
//!   * DECODE <file>  -> read the bytes back, reconstruct the sample, and assert
//!                       EVERY field == the canonical sample. Exit 0 on match,
//!                       nonzero + a diff otherwise.
//!
//! The bytes ARE the wire: `DdsType::encode` emits exactly the @appendable
//! XCDR2 serialization a ZeroDDS DataWriter places on the network (default
//! data representation, little-endian).

use std::collections::BTreeMap;
use std::process::ExitCode;

use zerodds_dcps::DdsType;

#[path = "../generated/20_mixed_combo.rs"]
mod generated;

use generated::combo::{Mode, Reading, Sample, Telemetry};

const GOLDEN: &str =
    "/path/to/zerodds/zerodds-examples/idl-conformance/_interop/goldens/rust.bin";

/// The canonical sample — IDENTICAL values across every language PSM, so the
/// encoded bytes must match byte-for-byte.
fn canonical() -> Telemetry {
    let mut counters: BTreeMap<String, i32> = BTreeMap::new();
    // BTreeMap iterates key-sorted: "drops" < "packets".
    counters.insert("drops".to_string(), 3);
    counters.insert("packets".to_string(), 100);

    Telemetry {
        unitId: 4242,
        region: "eu-central-1".to_string(),
        mode: Mode::MODE_ACTIVE,
        batteryCurrent: 13.75_f64,
        history: vec![
            Sample { seq: 1, value: 0.5 },
            Sample { seq: 2, value: 1.5 },
            Sample { seq: 3, value: -2.25 },
        ],
        reading: Reading::ActiveRate(60.0_f64),
        counters,
        calibration: Some(0.001_f64),
        window: [10, 20, 30, 40],
    }
}

fn encode_golden() -> ExitCode {
    let sample = canonical();
    let mut bytes = Vec::new();
    sample.encode(&mut bytes).expect("encode canonical sample");
    std::fs::write(GOLDEN, &bytes).expect("write golden");
    println!("ENCODE: wrote {} bytes -> {GOLDEN}", bytes.len());
    print!("hex:");
    for (i, b) in bytes.iter().enumerate() {
        if i % 16 == 0 {
            print!("\n  ");
        }
        print!("{b:02x} ");
    }
    println!();
    ExitCode::SUCCESS
}

/// Field-by-field comparison; collects a human-readable diff.
fn diff(expected: &Telemetry, got: &Telemetry) -> Vec<String> {
    let mut d = Vec::new();
    macro_rules! chk {
        ($field:ident, $label:expr) => {
            if expected.$field != got.$field {
                d.push(format!(
                    "  {} mismatch:\n    expected {:?}\n    got      {:?}",
                    $label, expected.$field, got.$field
                ));
            }
        };
    }
    chk!(unitId, "unitId (@key long)");
    chk!(region, "region (@key string<32>)");
    chk!(mode, "mode (enum Mode)");
    chk!(batteryCurrent, "batteryCurrent (typedef double)");
    chk!(history, "history (sequence<Sample>)");
    chk!(reading, "reading (union switch(Mode))");
    chk!(counters, "counters (map<string,long>)");
    chk!(calibration, "calibration (@optional double)");
    chk!(window, "window (long[4])");
    d
}

fn decode_check(path: &str) -> ExitCode {
    let bytes = std::fs::read(path).unwrap_or_else(|e| panic!("read {path}: {e}"));
    println!("DECODE: read {} bytes <- {path}", bytes.len());

    let got = match Telemetry::decode(&bytes) {
        Ok(t) => t,
        Err(e) => {
            eprintln!("FAIL: decode error: {e:?}");
            return ExitCode::FAILURE;
        }
    };

    let expected = canonical();
    let d = diff(&expected, &got);
    if d.is_empty() {
        println!("OK: decoded sample == canonical; all 9 fields match.");
        ExitCode::SUCCESS
    } else {
        eprintln!("FAIL: {} field(s) differ:", d.len());
        for line in &d {
            eprintln!("{line}");
        }
        ExitCode::FAILURE
    }
}

fn main() -> ExitCode {
    let args: Vec<String> = std::env::args().collect();
    match args.get(1).map(String::as_str) {
        Some("ENCODE") => encode_golden(),
        Some("DECODE") => {
            let path = args.get(2).map(String::as_str).unwrap_or(GOLDEN);
            decode_check(path)
        }
        _ => {
            eprintln!("usage: combo_interop ENCODE | DECODE <file>");
            ExitCode::FAILURE
        }
    }
}
