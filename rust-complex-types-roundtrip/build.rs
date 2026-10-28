// Generates the full `DdsType` binding for `demo.idl` (the `zerodds-idlc`
// pipeline, as a library call), so the example can encode/decode the complex
// `Sensor` message under BOTH representations: `encode`/`decode` (XCDR2) and
// `encode_xcdr1`/`decode_xcdr1` (classic CDR). `dds_type_impls(true)` is what
// emits that impl beside the codec.

fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .dds_type_impls(true)
        .compile(&["demo.idl"])
}
