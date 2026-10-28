// The generated codec plus the `DdsType` impl for the shared feature corpus
// `../features.idl` — the same file the other six PSMs generate from.
//
// No `default_extensibility` call: `Pt` and the corpus's other unannotated
// aggregates take ZeroDDS's default, appendable (XTypes 1.3 §7.3.1.2.1.8),
// as `zerodds-idlc` gives them in every other column — the default is part
// of what the goldens pin (CANONICAL.md, Tier 1).

fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .dds_type_impls(true)
        .compile(&["../features.idl"])
}
