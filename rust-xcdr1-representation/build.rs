// Generates the Rust binding for `demo.idl` at build time through
// `zerodds-build`, the `build.rs` helper that runs the same composition
// pipeline the `zerodds-idlc` CLI runs (preprocessing, vendor key
// pragmas, default extensibility, TypeObjects). The example then
// `include!`s the output.
//
// `dds_type_impls(true)` is what emits the second codec pair. Without
// it the generated type carries `encode`/`decode` alone; with it, it
// also carries `encode_xcdr1`/`decode_xcdr1`, and the `DdsType` impl
// that states which alignment cap belongs to which representation. Both
// pairs write the same member sequence — the difference between the
// representations is the `Writer`'s alignment cap, which is what
// `src/main.rs` varies by hand to show it.
//
// `compile()` reports `cargo:rerun-if-changed` for `demo.idl` and for
// every file it transitively includes.

fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .include_dir(".")
        .dds_type_impls(true)
        .compile(&["demo.idl"])
}
