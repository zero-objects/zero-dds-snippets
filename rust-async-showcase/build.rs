// SPDX-License-Identifier: Apache-2.0
// `reading.idl` -> `$OUT_DIR/reading.rs`. `dds_type_impls(true)` writes
// the `zerodds_dcps::DdsType` impl beside the codec, from the same
// model: the type name, `HAS_KEY` and the key holder the instance
// lifecycle needs, and the `TypeObject` the endpoint announces.

fn main() -> Result<(), zerodds_build::Error> {
    zerodds_build::Config::new()
        .include_dir(".")
        .dds_type_impls(true)
        .compile(&["reading.idl"])
}
