// SPDX-License-Identifier: Apache-2.0
//! Shared keyed topic type for the async showcase. A `@key` field is what makes
//! `register_instance` / `dispose` / `unregister_instance` meaningful.

use zerodds_cdr_derive::DdsType; // derive macro
pub use zerodds_dcps::DdsType; // trait (re-exported for convenience)

/// A keyed sample: `id` is the instance key, `value` the payload.
#[derive(DdsType, Debug, Clone, PartialEq)]
#[dds(type_name = "async_showcase::Reading")]
pub struct Reading {
    #[dds(key)]
    pub id: u32,
    pub value: i32,
}
