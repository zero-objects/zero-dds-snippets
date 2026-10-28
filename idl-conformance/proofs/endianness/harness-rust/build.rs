// The generated codec plus the `DdsType` impls for `../features.idl`.
//
// `dds_type_impls(true)`: the harness drives the four representations
// through `DdsType`'s `encode`/`encode_be`/`encode_xcdr1`/`encode_xcdr1_be`
// and their `decode` counterparts.
//
// No `default_extensibility` call: `feat::Pt` is unannotated and takes
// ZeroDDS's default, appendable (XTypes 1.3 §7.3.1.2.1.8) — the 56-byte `arr`
// the README's table and `_interop`'s goldens pin, and the form
// `_interop`'s big-endian decode checks read these goldens with.

fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .dds_type_impls(true)
        .compile(&["../features.idl"])
}
