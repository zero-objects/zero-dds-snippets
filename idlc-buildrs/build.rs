// Verbatim from documentation/04-idl/idlc-handbook.md ("6 build.rs
// integration (Rust)"). Shells out to the real `zerodds-idlc` CLI (must be
// on PATH — see README) exactly as the doc shows, generating `idl/Robot.idl`
// into `$OUT_DIR/Robot.rs`.
use std::process::Command;

fn main() {
    let idl_dir = std::path::PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("idl");
    let out_dir = std::env::var("OUT_DIR").unwrap();

    println!("cargo:rerun-if-changed={}", idl_dir.display());
    for entry in std::fs::read_dir(&idl_dir).unwrap() {
        let path = entry.unwrap().path();
        if path.extension().and_then(|e| e.to_str()) != Some("idl") {
            continue;
        }
        println!("cargo:rerun-if-changed={}", path.display());

        let out_file = std::path::Path::new(&out_dir)
            .join(path.file_stem().unwrap())
            .with_extension("rs");

        let status = Command::new("zerodds-idlc")
            .arg("generate")
            .arg(&path)
            .arg("--rust")
            .arg("-o")
            .arg(&out_dir)
            .status()
            .expect("failed to run zerodds-idlc");
        assert!(
            status.success(),
            "zerodds-idlc failed for {}",
            path.display()
        );

        // The generated file carries file-level `#![allow(...)]` inner
        // attributes (it is meant to stand alone as its own module file).
        // `include!`, in lib.rs, splices those tokens into the *including*
        // file rather than a fresh one, and an inner attribute is only
        // legal at the very start of the file it textually ends up in — so
        // strip them here and re-apply the same allows as outer attributes
        // on the `include!` call site instead (see lib.rs).
        let generated = std::fs::read_to_string(&out_file).unwrap();
        let stripped: String = generated
            .lines()
            .filter(|l| !l.trim_start().starts_with("#!["))
            .collect::<Vec<_>>()
            .join("\n");
        std::fs::write(&out_file, stripped).unwrap();
    }
}
