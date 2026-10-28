# dcps-time-service-clock

OMG Time Service 1.1 as the DCPS time source. The participant's
`RuntimeConfig::time_source` is a `TimeServiceClock` over a
`zerodds_time_service::TimeService`, so SOURCE_TIMESTAMP comes from that
service and not from the system clock (DDS 1.4 §2.2.2.4.2.11: `write`
stamps with the current time of the service).

The service reads the system clock one hour ahead — a stand-in for a
disciplined clock (GNSS, PTP) that a `TimeService::with_source` closure
would read. One sample is written and taken in one participant.

## Run it

```sh
cargo run --release
```

`ZERODDS_EXAMPLE_DOMAIN` picks the domain (default 0).

Output of the run this README was written from:

```
source timestamp 1790706507.640920599 s, system clock 1790702907.640934568 s: +3600 s
SOURCE_TIMESTAMP follows the time service
```

Path deps into the local zero-dds checkout: `zerodds-dcps`,
`zerodds-time-service`.
