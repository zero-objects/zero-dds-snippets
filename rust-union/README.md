# A union whose discriminator has values without a member

From GitHub #33. In `idl/message_payload.idl` the discriminator of
`MessagePayload` has four values, and only two of them select a member:

```idl
@final
union MessagePayload switch (MessagePayloadDiscriminator) {
    case MESSAGE_PAYLOAD_DISCRIMINATOR_POSITION:
        Vec3F32 position;
    case MESSAGE_PAYLOAD_DISCRIMINATOR_VELOCITY:
        Vec3F32 velocity;
};
```

`HEARTBEAT` and `ACKNOWLEDGEMENT` select no member (IDL 4.2 §7.4.1.4.4.4).
A sample with one of them is the discriminator alone. `derive(DdsType)`
takes such values in two forms.

One unit variant per value:

```rust
#[derive(DdsType)]
#[dds(name = "MessagePayload", extensibility = "final", discriminator = "MessagePayloadDiscriminator")]
enum MessagePayload {
    #[dds(case = MessagePayloadDiscriminator::Position, name = "position")]
    Position(Vec3F32),
    #[dds(case = MessagePayloadDiscriminator::Velocity, name = "velocity")]
    Velocity(Vec3F32),
    #[dds(case = MessagePayloadDiscriminator::Heartbeat)]
    Heartbeat,
    #[dds(case = MessagePayloadDiscriminator::Acknowledgement)]
    Acknowledgement,
}
```

One variant that holds every value no case takes:

```rust
#[dds(no_member)]
Signal(MessagePayloadDiscriminator),
```

The two forms can be combined. A unit variant cannot be combined with a
`default` variant. The `no_member` variant refuses to write a value that a
case with a member takes, or that a unit variant names. The enum takes
`extensibility = "final"` like the IDL's `@final enum`.

Both forms are the type of the IDL. The values without a member declare
no branch, so they appear only as literals of the enum. `build.rs` compiles the IDL
through `zerodds-build`, and `cargo run` checks both forms against it:

- the same TypeIdentifier, TypeObject and dependencies;
- the same octets in XCDR2 and XCDR1, little and big endian;
- each form reads what the others wrote.

It prints the octets of four samples, the value `Signal(Position)` refused
with its reason, and the TypeInformation a peer receives: the minimal and
complete TypeIdentifiers with their sizes and dependencies.

```sh
cargo run
```

## Dependencies

The crates of GitHub #33 (`zerodds-codec`, `zerodds-derive`) are newer than
the latest crates.io release. The path dependencies reach into a ZeroDDS
source tree that holds this repository, the layout of `ci/run-examples.sh`
(see the repository README). With RC8 the example switches to the published
crates.
