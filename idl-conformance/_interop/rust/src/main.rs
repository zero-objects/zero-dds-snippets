//! WIRE-level XCDR2 interop for the ZeroDDS Rust PSM.
//!
//! Reuses the generated `combo::Telemetry` codec (the SAME bodies DDS uses on
//! the wire) to:
//!   * ENCODE <file>  -> write the canonical sample's raw XCDR2-LE bytes.
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

// `zerodds-build` writes the module into OUT_DIR; the lint allows sit on the
// wrapping module because `include!` splices the tokens into this file.
#[allow(non_snake_case, non_camel_case_types, unused_imports, dead_code, clippy::all)]
mod generated {
    include!(concat!(env!("OUT_DIR"), "/combo.rs"));
}

use generated::combo::{Mode, Reading, Sample, Telemetry};

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

fn encode_to(path: &str) -> ExitCode {
    let sample = canonical();
    let mut bytes = Vec::new();
    if let Err(e) = <Telemetry as DdsType>::encode(&sample, &mut bytes) {
        eprintln!("FAIL: encode error: {e:?}");
        return ExitCode::FAILURE;
    }
    if let Err(e) = std::fs::write(path, &bytes) {
        eprintln!("FAIL: write {path}: {e}");
        return ExitCode::FAILURE;
    }
    println!("ENCODE: wrote {} bytes -> {path}", bytes.len());
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
    let bytes = match std::fs::read(path) {
        Ok(b) => b,
        Err(e) => {
            eprintln!("FAIL: read {path}: {e}");
            return ExitCode::FAILURE;
        }
    };
    println!("DECODE: read {} bytes <- {path}", bytes.len());

    let got = match <Telemetry as DdsType>::decode(&bytes) {
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
    match (args.get(1).map(String::as_str), args.get(2)) {
        (Some("ENCODE"), Some(path)) => encode_to(path),
        (Some("DECODE"), Some(path)) => decode_check(path),
        _ => {
            eprintln!("usage: combo_interop ENCODE <file> | DECODE <file>");
            ExitCode::FAILURE
        }
    }
}
