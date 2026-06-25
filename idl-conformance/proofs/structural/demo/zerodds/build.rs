use std::path::PathBuf;
fn main() {
    println!("cargo:rerun-if-changed=MapEnum.idl");
    let idl = std::fs::read_to_string("MapEnum.idl").expect("read");
    let ast = zerodds_idl::parse(&idl, &zerodds_idl::config::ParserConfig::default()).expect("parse");
    let rust = zerodds_idl_rust::generate_rust_module(&ast, &zerodds_idl_rust::RustGenOptions::default()).expect("gen");
    let rust: String = rust.lines().filter(|l| !l.trim_start().starts_with("#![")).collect::<Vec<_>>().join("\n");
    let out = PathBuf::from(std::env::var("OUT_DIR").unwrap()).join("mapenum.rs");
    std::fs::write(&out, rust).expect("write");
}
