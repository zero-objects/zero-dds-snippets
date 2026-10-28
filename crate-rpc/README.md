# crate-rpc

The website's docs/rpc.html usage snippet, made runnable — and corrected.

The snippet as published does not compile against any release. Five points:

 1. `use zerodds_rpc::{Requester, Replier, RpcQos, FnHandler}` — none of the
    four is re-exported at the crate root, at the v1.0.0-rc.6 tag or in the
    current tree. They live in `requester`, `replier` and `qos_profile`.
 2. `DomainParticipant::new(0)?` — `DomainParticipant::new` is `pub(crate)`.
    A participant comes from `DomainParticipantFactory`.
 3. `Replier::new(..., FnHandler::new(...))` — the handler parameter is
    `Arc<dyn ReplierHandler<TIn, TOut>>`, so the adapter has to be wrapped.
 4. The closure must return `Result<TOut, RemoteExceptionCode>`, not `TOut`.
 5. `MyRequest` / `MyReply` have to be DDS types; here they come from
    `calculator.idl` through `zerodds-build`.

Both endpoints sit on one offline participant and the frames are pumped
between them in-process, which is what makes the run deterministic: an
online pair would depend on discovery timing.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-rpc`, `zerodds-dcps`, `zerodds-wire`, `zerodds-types`, `zerodds-sql-filter`, `zerodds-build`.

Output of the run this README was written from:

```
rpc: Calculator(1, 2) = 3
```

## Reference

- Explained on the website: [RPC over DDS](https://zerodds.org/docs/rpc.html)
- This example in the repository: [zero-dds-snippets/crate-rpc](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-rpc)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
