//! Demo for the `website/topics/transport-tuning/index.html` snippets:
//! the four `RuntimeConfig { user_transports: vec![...] }` recipes published
//! in the "Transport-Tuning Cookbook" (recipe 2 TCP, recipe 3 SHM, recipe 4
//! UDS, recipe 6 multi-transport failover).
//!
//! `UserTransportKind::Shm`/`Uds` are feature-gated in zerodds-dcps behind
//! `same-host-shm` / `same-host-uds`; the Cargo.toml enables both so the
//! snippets compile as published. Each recipe is reproduced VERBATIM from the
//! website and `main` builds all four and prints their `user_transports` len.

use zerodds_dcps::runtime::{RuntimeConfig, UserTransportKind};

/// Recipe 2 — TCP for NAT traversal (website §recipe-tcp-nat).
fn recipe_tcp() -> RuntimeConfig {
    let cfg = RuntimeConfig {
        user_transports: vec![
            UserTransportKind::TcpV4, // TCP for WAN / firewall traversal
        ],
        ..Default::default()
    };
    cfg
}

/// Recipe 3 — Shared-Memory for in-box IPC (website §recipe-shm).
fn recipe_shm() -> RuntimeConfig {
    // Cargo.toml: zerodds-dcps = { features = ["same-host-shm"] }
    let cfg = RuntimeConfig {
        // Preference order: first kind that matches a peer locator wins.
        user_transports: vec![
            UserTransportKind::Shm,   // SHM for same-host peers (fast path)
            UserTransportKind::UdpV4, // UDP fallback for cross-host
        ],
        ..Default::default()
    };
    cfg
}

/// Recipe 4 — UDS for container sidecar (website §recipe-uds).
fn recipe_uds() -> RuntimeConfig {
    // Cargo.toml: zerodds-dcps = { features = ["same-host-uds"] }
    let cfg = RuntimeConfig {
        user_transports: vec![
            UserTransportKind::Uds, // UDS for container sidecars
        ],
        ..Default::default()
    };
    cfg
}

/// Recipe 6 — Multi-transport failover (website §recipe-multi).
fn recipe_multi() -> RuntimeConfig {
    // Cargo.toml: zerodds-dcps = { features = ["same-host-shm", "same-host-uds"] }
    let cfg = RuntimeConfig {
        // Preference order: first kind that matches a peer locator wins.
        user_transports: vec![
            UserTransportKind::Shm,   // SHM for same-host peers (fast path)
            UserTransportKind::Uds,   // UDS for container sidecars
            UserTransportKind::UdpV4, // UDP fallback for cross-host
        ],
        ..Default::default()
    };
    cfg
}

fn main() {
    let tcp = recipe_tcp();
    let shm = recipe_shm();
    let uds = recipe_uds();
    let multi = recipe_multi();

    assert_eq!(tcp.user_transports.len(), 1);
    assert_eq!(shm.user_transports.len(), 2);
    assert_eq!(uds.user_transports.len(), 1);
    assert_eq!(multi.user_transports.len(), 3);

    println!("recipe 2 (TCP for NAT traversal):   {:?}", tcp.user_transports);
    println!("recipe 3 (SHM for in-box IPC):      {:?}", shm.user_transports);
    println!("recipe 4 (UDS for container sidecar): {:?}", uds.user_transports);
    println!("recipe 6 (multi-transport failover): {:?}", multi.user_transports);
    println!("all four RuntimeConfig recipes built — snippets compile as published");
}
