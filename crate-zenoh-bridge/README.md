# crate-zenoh-bridge

The website's docs/zenoh-bridge.html usage snippet, made runnable — and
corrected. The snippet as published does not compile against any release:

 1. `TopicMap::add` takes one `TopicMapEntry`, not a (topic, key) pair —
    the same at the v1.0.0-rc.6 tag and in the current tree. `TopicMapEntry`
    was not re-exported from the crate root either, so `add` took a type no
    caller could name; the `pub use` was added while writing this.
 2. `key_expr_for_topic` takes (scope, partition, topic) — the scope as
    `Option<&str>` — and returns a `Result`: a segment holding a
    Zenoh-reserved character (`* ? # $ /`) is refused, not rewritten.
 3. A '.' in a DDS topic name is no reserved character and stays a '.':
    `key_expr_for_topic(Some("fleet"), "", "VehicleTracking.TrackUpdate")`
    is "fleet/VehicleTracking.TrackUpdate".

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-zenoh-bridge`, `zerodds-qos`.

Output of the run this README was written from:

```
zenoh-bridge: mapped topic -> fleet/tracking
zenoh-bridge: key expression = fleet/VehicleTracking.TrackUpdate
```

## Reference

- Explained on the website: [Zenoh bridge](https://zerodds.org/docs/zenoh-bridge.html)
- This example in the repository: [zero-dds-snippets/crate-zenoh-bridge](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-zenoh-bridge)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
