// SPDX-License-Identifier: Apache-2.0
//! Shared keyed topic type for the async showcase. A `@key` field is what makes
//! `register_instance` / `dispose` / `unregister_instance` meaningful.
//!
//! The type comes from `reading.idl`, compiled by `zerodds-build` in
//! `build.rs`. The `#[derive(DdsType)]` bridge the earlier releases
//! carried is gone: it stated the type twice — once as a Rust struct,
//! once as a codec — and left `type_object()` at its default `None`, so
//! the endpoint announced a topic type name with no type behind it
//! (XTypes 1.3 §7.6.3.2).

#[allow(non_snake_case, unused_imports, dead_code, clippy::all)]
mod generated {
    include!(concat!(env!("OUT_DIR"), "/reading.rs"));
}

pub use generated::async_showcase::Reading;
pub use zerodds_dcps::DdsType; // trait (re-exported for convenience)
