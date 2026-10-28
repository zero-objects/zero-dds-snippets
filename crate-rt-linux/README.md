# crate-rt-linux

The website's docs/rt-linux.html usage snippet, made runnable — and
corrected. The page writes `SchedulerProfile::fifo(80)`. No release has such
a constructor: `SchedulerProfile` is an enum with a `RealtimeFifo { priority }`
variant, at the v1.0.0-rc.6 tag and in the current tree, and building a
variant returns the value itself, not a `Result` to `.expect()` on.

Linux only: SCHED_FIFO and sched_setaffinity are Linux system calls, so the
crate compiles nowhere else. The snippet asks for FIFO priority 80 and pins
to CPUs 2 and 3. FIFO needs CAP_SYS_NICE; where a cgroup or container does not
allow CPUs 2 and 3, the demo pins to the first two CPUs the thread may run on
and reads the mask back. It says which requirement was not met rather than
reporting success.

## Run it

```sh
cargo run
```

Path deps into the local zero-dds checkout: `zerodds-rt-linux`.

**Linux only.** `SchedulerProfile::apply_to_current_thread` and `pin_to_cpus` are `sched_setscheduler` / `sched_setaffinity`, which exist nowhere else. On macOS the demo prints which requirement was not met and exits non-zero; it is not a pass there.

Output of the run this README was written from — on macOS, where the crate's
own precondition is not met:

```
rt-linux: SCHED_FIFO refused (SchedulerProfile::apply_to_current_thread requires Linux) — CAP_SYS_NICE is required
```

That is not a pass. `ci/run-examples.sh` reports this example as **unverified**
on a non-Linux host and names the reason; only a Linux host with `CAP_SYS_NICE`
and two CPUs its thread may run on makes it a run. There it prints
`rt-linux: this thread runs SCHED_FIFO at priority 80` and then
`rt-linux: this thread is pinned to CPUs 2 and 3` (or the two CPUs it used
instead); `rt-linux: this thread is pinned to CPUs` is what the manifest checks
for.

## Reference

- Explained on the website: [Real-time Linux](https://zerodds.org/docs/rt-linux.html)
- This example in the repository: [zero-dds-snippets/crate-rt-linux](https://github.com/zero-objects/zero-dds-snippets/tree/main/crate-rt-linux)

Part of **[ZeroDDS](https://zerodds.org)** — the pure-Rust OMG DDS implementation.
