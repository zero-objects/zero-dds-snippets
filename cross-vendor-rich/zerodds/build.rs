// `Rich.idl` carries the same three members under `@final`, `@appendable`
// and `@mutable`. `dds_type_impls(true)` gives each of the three types
// its `DdsType` impl — what `create_topic` takes, and what puts the
// `TypeObject` into the endpoint's TypeLookup server. The peers in this
// matrix ask for it: a type announced by name with no `TypeObject`
// behind it is what a hand-written bridge used to produce (XTypes 1.3
// §7.6.3.2).
//
// `ZERODDS_RICH_DIALECT` builds the codec in one peer's dialect
// (`zerodds_ast_layout::Dialect::parse` spells them: `fastdds`, `opendds`,
// ...). Unset, it is the specification's. The reader does not guess a
// dialect from the bytes: Fast DDS closes a PL_CDR1 parameter list with a
// bare `0x3f02` where the specification's reading sets the must-understand
// bit, and ZeroDDS reports that as a framing error rather than accepting a
// form it was not told to expect. The leg that talks to Fast DDS builds
// with `fastdds`.
fn main() -> Result<(), zerodds_build::Error> {
    println!("cargo:rerun-if-env-changed=ZERODDS_RICH_DIALECT");
    let mut config = zerodds_build::Config::new();
    config.include_dir(".").dds_type_impls(true);
    if let Ok(name) = std::env::var("ZERODDS_RICH_DIALECT") {
        match zerodds_ast_layout::Dialect::parse(&name) {
            Some(dialect) => {
                config.dialect(dialect);
            }
            None => panic!("ZERODDS_RICH_DIALECT={name} names no dialect"),
        }
    }
    config.compile(&["Rich.idl"])
}
