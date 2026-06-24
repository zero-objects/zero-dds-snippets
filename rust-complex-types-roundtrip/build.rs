// Generates the full `DdsType` binding for `demo.idl` (the `idlc --rust` path),
// so the example can encode/decode the complex `Sensor` message under BOTH
// representations: `encode`/`decode` (XCDR2) and `encode_xcdr1`/`decode_xcdr1`
// (classic CDR). The default options (not `cdr_only`) emit the full DdsType
// impl that carries the real per-representation framing.

use std::path::PathBuf;

fn main() {
    println!("cargo:rerun-if-changed=demo.idl");
    let idl = std::fs::read_to_string("demo.idl").expect("read demo.idl");
    let ast = zerodds_idl::parse(&idl, &zerodds_idl::config::ParserConfig::default())
        .expect("parse demo.idl");
    let rust = zerodds_idl_rust::generate_rust_module(&ast, &zerodds_idl_rust::RustGenOptions::default())
        .expect("generate rust");
    // Strip the file-level `#![...]` inner attributes — the example `include!`s
    // this inside a `mod`, where inner attributes are not permitted.
    let rust: String = rust
        .lines()
        .filter(|l| !l.trim_start().starts_with("#!["))
        .collect::<Vec<_>>()
        .join("\n");
    let out = PathBuf::from(std::env::var("OUT_DIR").unwrap()).join("sensor.rs");
    std::fs::write(&out, rust).expect("write generated module");
}
