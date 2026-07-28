// Verbatim from documentation/03-configuration/runtime-config.md.
//
// `main()` builds the full field-list fence (top of the page) and starts a
// real `DcpsRuntime` with it — proving every named field is real and the
// combination is accepted. The three "Common combinations" recipes further
// down the same page (Sandbox/test, Production-with-monitoring, Hard
// real-time — lines 93/101/110) are folded in here rather than getting
// their own `zerodds-examples/` projects, since each is a one-field-deep
// variation on the same `RuntimeConfig` struct literal; see the doc-comment
// on each function below and the README for the mapping back to the page.
use std::net::Ipv4Addr;
use std::sync::Arc;
use std::time::Duration;

use zerodds_dcps::runtime::{DcpsRuntime, RuntimeConfig};
use zerodds_foundation::observability::StderrJsonSink;
use zerodds_rtps::wire_types::GuidPrefix;

/// Doc fence (top of page): the full field list.
fn full_field_list_config() -> RuntimeConfig {
    RuntimeConfig {
        tick_period: Duration::from_millis(50),
        spdp_period: Duration::from_secs(5),
        spdp_multicast_group: Ipv4Addr::new(239, 255, 0, 1),
        multicast_interface: Ipv4Addr::UNSPECIFIED,
        announce_secure_endpoints: false,
        wlp_period: Duration::ZERO, // = lease/3
        participant_lease_duration: Duration::from_secs(100),
        observability: zerodds_foundation::observability::null_sink(),
        // … security fields when feature = "security"
        ..Default::default()
    }
}

/// "Common combinations" > "Sandbox / test" (line 93).
#[allow(dead_code)]
fn sandbox_test_config() -> RuntimeConfig {
    RuntimeConfig::default()
}

/// "Common combinations" > "Production server with monitoring" (line 101).
#[allow(dead_code)]
fn production_with_monitoring_config() -> RuntimeConfig {
    RuntimeConfig {
        observability: Arc::new(StderrJsonSink::new()),
        ..Default::default()
    }
}

/// "Common combinations" > "Hard real-time" (line 110).
#[allow(dead_code)]
fn hard_realtime_config() -> RuntimeConfig {
    RuntimeConfig {
        tick_period: Duration::from_millis(2),
        spdp_period: Duration::from_secs(60), // discovery is rare
        participant_lease_duration: Duration::from_secs(10),
        ..Default::default()
    }
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let domain_id = 11;
    let prefix = GuidPrefix::from_bytes([0x0b; 12]);
    let rt = DcpsRuntime::start(domain_id, prefix, full_field_list_config())?;
    println!(
        "runtime-config: DcpsRuntime started on domain {domain_id} with the full field-list config"
    );
    rt.shutdown();

    // The three recipe variants below are proven to compile by virtue of
    // being referenced here (`cargo build` type-checks all reachable code);
    // they are not started live since they only vary already-covered
    // fields (see README for details).
    let _ = sandbox_test_config();
    let _ = production_with_monitoring_config();
    let _ = hard_realtime_config();
    Ok(())
}
