// The website's docs/monitor.html usage snippets, made runnable — both of them:
// the metric registry with its Prometheus rendering, and the W3C traceparent
// carried as an inline QoS parameter.
use zerodds_foundation::tracing::{SpanId, TraceId};
use zerodds_monitor::{default_registry, metric_names, Labels, TraceContextPid, TraceParent};

fn main() {
    let reg = default_registry();
    let counter = reg.counter(
        metric_names::DDS_DCPS_SAMPLES_WRITTEN_TOTAL,
        Labels::new().with("topic", "VehicleTracking.TrackUpdate"),
    );
    counter.inc();
    counter.add(5);

    let rendered = reg.render_prometheus();
    assert!(rendered.contains(metric_names::DDS_DCPS_SAMPLES_WRITTEN_TOTAL));
    println!("{rendered}");

    let tp = TraceParent::new(TraceId([0x11; 16]), SpanId([0x22; 8]), 1);
    let pid = TraceContextPid::new(tp.clone(), None);
    let mut buf = Vec::new();
    pid.encode_inline_qos(&mut buf);
    let decoded = TraceContextPid::decode_inline_qos(&buf).expect("roundtrip");
    assert_eq!(decoded.traceparent.trace_id, tp.trace_id);
    println!("monitor: traceparent roundtripped through {} bytes of inline QoS", buf.len());
}
