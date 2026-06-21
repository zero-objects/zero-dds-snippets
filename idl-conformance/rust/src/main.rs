//! Real-DCPS IDL conformance roundtrip for the ZeroDDS Rust binding.
//!
//! Generates a richly-combined IDL type with `zerodds-idlc --rust`, then over a
//! REAL DomainParticipant + pub/sub (UDP loopback, two participants in one
//! process, same domain) publishes one fully-populated sample and reads it
//! back, asserting every field survived the XCDR2 wire roundtrip.
//!
//! Combined IDL features exercised by `combo::Telemetry` (see
//! `idl/combo_no_array.idl`, derived from
//! `tools/idlc/tests/conformance/fixtures/20_mixed_combo.idl`):
//!   * enum                 (`Mode`)
//!   * typedef-to-primitive (`CurrentInAmpsType` = double)
//!   * nested struct        (`Sample`)
//!   * sequence-of-struct   (`sequence<Sample> history`)
//!   * bounded string       (`string<32> region`, `string<16>` union arm)
//!   * union over enum disc. (`Reading switch(Mode)`)
//!   * map                  (`map<string,long> counters`)
//!   * @optional            (`@optional double calibration`)
//!   * @key                 (`@key long unitId`, `@key string<32> region`)
//!   * @appendable          (struct extensibility)
//!
//! The one upstream feature NOT included is the fixed array `long window[4]` —
//! the Rust backend currently mis-generates the array *decode* path. See the
//! README "Known limitations".

use std::time::Duration;

use zerodds_dcps::runtime::RuntimeConfig;
use zerodds_dcps::{
    DataReaderQos, DataWriterQos, DomainParticipantFactory, DomainParticipantQos, PublisherQos,
    SubscriberQos, TopicQos,
};

// Pull the generated module in as a child module of this crate.
#[path = "../generated/combo_no_array.rs"]
mod generated;

use generated::combo::{Mode, Reading, Sample, Telemetry};

fn make_sample() -> Telemetry {
    Telemetry {
        unitId: 4242,
        region: "eu-central-1".to_string(),
        mode: Mode::MODE_ACTIVE,
        batteryCurrent: 13.75_f64,
        history: vec![
            Sample { seq: 1, value: 0.5 },
            Sample { seq: 2, value: 1.5 },
            Sample { seq: 3, value: -2.25 },
        ],
        reading: Reading::FaultCode("E_OVERHEAT".to_string()),
        counters: {
            let mut m = std::collections::BTreeMap::new();
            m.insert("packets".to_string(), 100_i32);
            m.insert("drops".to_string(), 3_i32);
            m
        },
        calibration: Some(0.001_f64),
    }
}

fn main() {
    // Two participants, one process, same domain, exchanging over UDP loopback.
    let cfg = RuntimeConfig {
        tick_period: Duration::from_millis(20),
        spdp_period: Duration::from_millis(100),
        ..RuntimeConfig::default()
    };
    let factory = DomainParticipantFactory::instance();
    let domain = 0;

    let pub_p = factory
        .create_participant_with_config(domain, DomainParticipantQos::default(), cfg.clone())
        .expect("pub participant");
    let sub_p = factory
        .create_participant_with_config(domain, DomainParticipantQos::default(), cfg)
        .expect("sub participant");

    let pub_topic = pub_p
        .create_topic::<Telemetry>("Telemetry", TopicQos::default())
        .expect("pub topic");
    let sub_topic = sub_p
        .create_topic::<Telemetry>("Telemetry", TopicQos::default())
        .expect("sub topic");

    let writer = pub_p
        .create_publisher(PublisherQos::default())
        .create_datawriter::<Telemetry>(&pub_topic, DataWriterQos::default())
        .expect("writer");
    let reader = sub_p
        .create_subscriber(SubscriberQos::default())
        .create_datareader::<Telemetry>(&sub_topic, DataReaderQos::default())
        .expect("reader");

    // Discovery handshake (event-driven, no busy-poll).
    writer
        .wait_for_matched_subscription(1, Duration::from_secs(5))
        .expect("writer matched");
    reader
        .wait_for_matched_publication(1, Duration::from_secs(5))
        .expect("reader matched");

    let sent = make_sample();
    println!("PUB  -> {sent:?}");
    writer.write(&sent).expect("write");

    reader
        .wait_for_data(Duration::from_secs(3))
        .expect("sample arrives");
    let samples = reader.take().expect("take");
    assert_eq!(samples.len(), 1, "expected exactly one sample");
    let got = &samples[0];
    println!("SUB  <- {got:?}");

    // Field-by-field equality assertions over the wire roundtrip.
    assert_eq!(got.unitId, sent.unitId, "unitId (@key long)");
    assert_eq!(got.region, sent.region, "region (@key string<32>)");
    assert_eq!(got.mode, sent.mode, "mode (enum)");
    assert_eq!(
        got.batteryCurrent, sent.batteryCurrent,
        "batteryCurrent (typedef double)"
    );
    assert_eq!(got.history, sent.history, "history (sequence<Sample>)");
    assert_eq!(got.reading, sent.reading, "reading (union over enum)");
    assert_eq!(got.counters, sent.counters, "counters (map<string,long>)");
    assert_eq!(
        got.calibration, sent.calibration,
        "calibration (@optional double)"
    );
    // Whole-struct equality as a belt-and-suspenders final check.
    assert_eq!(got, &sent, "full Telemetry struct roundtrip");

    println!(
        "\nOK: DCPS pub->sub roundtrip recovered the sample; all {} field groups matched.",
        8
    );
}
