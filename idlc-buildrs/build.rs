// Verbatim from documentation/04-idl/idlc-handbook.md ("6 build.rs
// integration (Rust)"). `zerodds-build` compiles every `.idl` under
// `idl/` into `$OUT_DIR`, running the same composition pipeline the
// `zerodds-idlc` CLI runs — preprocessing, `#include` resolution and its
// rebuild tracking, vendor key pragmas, default extensibility, and the
// TypeObject lowering.
//
// It is a library call, not a `Command::new("zerodds-idlc")`: a build
// script that shells out needs the CLI on `PATH` at build time, which a
// consumer of the crate has no reason to have installed. Both paths
// generate data types and codecs. This configuration also requests the
// `DdsType` bridge through the typed build-helper API.

fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .include_dir("idl")
        .dds_type_impls(true)
        .compile(&["idl/Robot.idl"])
}
