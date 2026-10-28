//! Safety classification: **SAFE**
//!
//! The three `zerodds-lint` pragmas the website's docs/lint.html snippet shows,
//! on code that actually triggers the lints they suppress or satisfy.
//!
//! zerodds-lint: allow no_dyn_in_safe

/// A `Box<dyn Fn()>` in a safety-classified file. `no_dyn_in_safe` reports it
/// unless the file carries the allow marker above.
pub fn callback(handler: Box<dyn Fn()>) {
    handler();
}

/// zerodds-lint: hot-path
///
/// A hot path allocates nothing: no `Vec::new`, no `Box::new`, no `format!`,
/// no `to_string`. `no_alloc_in_hot_path` and `no_realloc_in_hot_path` check
/// the body of a function carrying this marker.
pub fn write_user_sample(dst: &mut [u8], src: &[u8]) -> usize {
    let n = core::cmp::min(dst.len(), src.len());
    dst[..n].copy_from_slice(&src[..n]);
    n
}

/// A module tree the recursive walk below descends.
pub struct Module {
    /// Nested sub-modules.
    pub children: Vec<Module>,
}

/// zerodds-lint: recursion-depth 16
///
/// A recursive function must state its bound. `bounded_recursion` reports one
/// that does not.
pub fn parse_module(module: &Module) -> usize {
    let mut n = 1;
    for child in &module.children {
        n += parse_module(child);
    }
    n
}
