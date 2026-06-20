// Snippet from website/topics/audit-reference/index.html (OTLP exporter).
// The website snippet elides the Span/Histogram/Event bodies with `/* ... */`,
// so the load-bearing, real-code part verified here is the config + exporter
// construction and the flush() call (the API surface the snippet asserts exists).
use std::time::Duration;
use zerodds_observability_otlp::{OtlpConfig, OtlpExporter};

fn main() {
    let cfg = OtlpConfig {
        host: "otel-collector.internal".into(),
        port: 4318,
        service_name: "zerodds-publisher".into(),
        service_version: env!("CARGO_PKG_VERSION").into(),
        timeout: Duration::from_secs(5),
    };
    let exporter = OtlpExporter::new(cfg);

    // exporter.add_span(Span { /* ... */ });        // shown in the docs
    // exporter.add_histogram(Histogram { /* ... */ });
    // exporter.add_event(Event { /* ... */ });

    // flush() POSTs to the collector only if buffers are non-empty; with an
    // empty buffer it is a no-op that returns Ok — proving the signature.
    exporter.flush().expect("flush");
    println!("otlp exporter flush ok");
}
