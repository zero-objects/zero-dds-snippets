# crate-monitor

The website's docs/monitor.html usage snippets, made runnable — both of them:
the metric registry with its Prometheus rendering, and the W3C traceparent
carried as an inline QoS parameter.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-monitor`, `zerodds-foundation`.

Output of the run this README was written from:

```
# TYPE dds_dcps_samples_written_total counter
dds_dcps_samples_written_total{topic="VehicleTracking.TrackUpdate"} 6

monitor: traceparent roundtripped through 68 bytes of inline QoS
```

## Reference

- Explained on the website: [Monitor](https://zerodds.org/docs/monitor.html)
- This example in the repository: [zero-dds-snippets/crate-monitor](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-monitor)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
