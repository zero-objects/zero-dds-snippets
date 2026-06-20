//! qos-cookbook snippets verified against published crates.
use zerodds_qos::Duration;
use zerodds_dcps::*;
use zerodds_dcps::qos::*;

#[allow(dead_code, unused_variables)]
fn snippet_0() {
    let writer_qos = DataWriterQos {
        reliability: ReliabilityQosPolicy {
            kind: ReliabilityKind::BestEffort,
            ..Default::default()
        },
        durability: DurabilityQosPolicy {
            kind: DurabilityKind::Volatile,
        },
        history: HistoryQosPolicy {
            kind: HistoryKind::KeepLast,
            depth: 1,
        },
        ..Default::default()
    };
}

#[allow(dead_code, unused_variables)]
fn snippet_1() {
    let writer_qos = DataWriterQos {
        reliability: ReliabilityQosPolicy {
            kind: ReliabilityKind::Reliable,
            max_blocking_time: Duration::from_millis(100),
        },
        durability: DurabilityQosPolicy {
            kind: DurabilityKind::TransientLocal,
        },
        history: HistoryQosPolicy {
            kind: HistoryKind::KeepLast,
            depth: 1,
        },
        ..Default::default()
    };
}

#[allow(dead_code, unused_variables)]
fn snippet_2() {
    let primary_qos = DataWriterQos {
        ownership: OwnershipQosPolicy {
            kind: OwnershipKind::Exclusive,
        },
        ownership_strength: OwnershipStrengthQosPolicy { value: 100 },
        liveliness: LivelinessQosPolicy {
            kind: LivelinessKind::ManualByTopic,
            lease_duration: Duration::from_secs(1),
        },
        ..Default::default()
    };
    // Backup: gleiche Topic + ownership=Exclusive, strength=50
}

#[allow(dead_code, unused_variables)]
fn snippet_3() {
    let reader_qos = DataReaderQos {
        deadline: DeadlineQosPolicy {
            period: Duration::from_millis(10), // 100 Hz
        },
        latency_budget: LatencyBudgetQosPolicy {
            duration: Duration::from_micros(500),
        },
        reliability: ReliabilityQosPolicy {
            kind: ReliabilityKind::Reliable,
            max_blocking_time: Duration::from_millis(1),
        },
        history: HistoryQosPolicy {
            kind: HistoryKind::KeepLast,
            depth: 4, // 40 ms headroom
        },
        ..Default::default()
    };
}

#[allow(dead_code, unused_variables)]
fn snippet_4() {
    let publisher_qos = PublisherQos {
        partition: PartitionQosPolicy {
            names: vec!["customer-A".into()],
        },
        ..Default::default()
    };
    // Subscriber-Side: partition = ["customer-A"] oder ["customer-*"] (Pattern)
}

fn main() {}