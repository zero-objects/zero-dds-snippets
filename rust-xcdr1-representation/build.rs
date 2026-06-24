// Generates the Rust binding for `demo.idl` at build time using the ZeroDDS
// IDL-to-Rust code generator (the same `idlc --rust` path), then the example
// `include!`s the output. The generated `Reading` overrides `encode_xcdr1` /
// `decode_xcdr1` with a *true* XCDR1 (classic CDR) writer/reader — that is what
// makes the two representations actually differ on the wire (a hand-derived
// `#[derive(DdsType)]` type only gets the default, which delegates to XCDR2).

use std::path::PathBuf;

fn main() {
    println!("cargo:rerun-if-changed=demo.idl");
    let idl = std::fs::read_to_string("demo.idl").expect("read demo.idl");
    let ast = zerodds_idl::parse(&idl, &zerodds_idl::config::ParserConfig::default())
        .expect("parse demo.idl");
    // `cdr_only` keeps just the type + its classic CdrEncode/CdrDecode (the
    // wire codec whose BufferWriter max-alignment selects the representation),
    // without the full DdsType impl that would pull in dcps/types/sql_filter.
    let opts = zerodds_idl_rust::RustGenOptions {
        cdr_only: true,
        ..zerodds_idl_rust::RustGenOptions::default()
    };
    let rust = zerodds_idl_rust::generate_rust_module(&ast, &opts).expect("generate rust");
    // Strip the file-level `#![...]` inner attributes: the example `include!`s
    // this inside a `mod`, where inner attributes are not permitted. The lint
    // allows are reapplied as outer attributes on that module in main.rs.
    let rust: String = rust.lines().filter(|l| !l.trim_start().starts_with("#![")).collect::<Vec<_>>().join("\n");
    let out = PathBuf::from(std::env::var("OUT_DIR").unwrap()).join("reading.rs");
    std::fs::write(&out, rust).expect("write generated module");
}
