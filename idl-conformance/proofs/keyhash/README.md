# KeyHash conformance proof (cross-vendor)

This example proves that **ZeroDDS computes the DDSI-RTPS instance KeyHash
(PID_KEY_HASH, parameter id `0x0070`) byte-for-byte identically to CycloneDDS,
RTI Connext, and eProsima Fast DDS** — for both the short (zero-pad) and the
long (MD5) branch of the algorithm.

The KeyHash is the 16-byte instance identity carried in the inline QoS of a
DATA / DATA_FRAG submessage. Readers, persistence/durability services, and
recorders use it to correlate samples of the same instance **without
deserializing the payload**. If two vendors disagree on the KeyHash, the same
logical instance looks like two different instances across the wire — keyed QoS
(history-per-instance, ownership, instance lifecycle) silently breaks. That
makes the KeyHash an interop-critical wire surface.

## The rule (DDSI-RTPS 2.5 §9.6.3.8 / OMG XTypes 1.3 §7.6.8)

1. Build a *KeyHolder*: the `@key` members only, in ascending member-id order.
   Nested aggregate keys are expanded recursively into their own `@key`
   members (Step 3).
2. Serialize the KeyHolder in **big-endian PLAIN_CDR2** — no encapsulation
   header, no member headers, max alignment 4.
3. Decide on the **statically known maximum** KeyHolder size (a property of the
   *type*, not of the concrete instance):
   * **max ≤ 16 bytes** → KeyHash = the serialized bytes, zero-padded to 16.
   * **max > 16 bytes** → KeyHash = `MD5(serialized bytes)`.

Because the decision is static, a type whose key *could* exceed 16 bytes always
takes the MD5 branch, even for a short concrete value.

## The four canonical cases

| Case | Type        | Key members                                  | Branch    |
|------|-------------|----------------------------------------------|-----------|
| (a)  | `SingleKey` | `@key long id`                               | zero-pad (4 B) |
| (b)  | `BigKey`    | `@key string name` + `@key long region`      | **MD5** (unbounded string ⇒ no static ≤16 max) |
| (c)  | `MixedKeys` | `@key octet`, `short`, `long`, `char`        | zero-pad (9 B incl. alignment) |
| (d)  | `OuterKey`  | `@key NestedKey k` (`hi`, `lo` both `@key`)  | zero-pad (8 B, nested expansion) |

IDL: [`idl/keyhash_cases.idl`](idl/keyhash_cases.idl) (cases a–c, generated for
the Rust harness) and [`vendor/keyhash_full.idl`](vendor/keyhash_full.idl) (all
four cases, used for the vendors).

## Result

For the fixed instances below, **all four implementations produce the same
16-byte KeyHash**:

```
case  KeyHash (hex)                       branch     ZeroDDS Cyclone RTI(wire) FastDDS
(a)   11223344000000000000000000000000   zero-pad      OK     OK      OK       OK
(b)   492ce15eccc6c49993afe63223435b91   MD5           OK     OK      OK       OK
(c)   aa001234556677885a00000000000000   zero-pad      OK     OK      OK       OK
(d)   01020304050607080000000000000000   zero-pad      OK     OK      OK       OK
```

The recorded vendor values, exact versions, and capture method are in
[`vendor/RECORDED-KEYHASHES.txt`](vendor/RECORDED-KEYHASHES.txt). RTI's value is
a genuine on-the-wire capture of the `PID_KEY_HASH` parameter.

## Run the ZeroDDS side

```sh
cd zerodds-examples/idl-conformance/proofs/keyhash
cargo run
```

This prints ZeroDDS's 16-byte KeyHash for each instance, plus the intermediate
big-endian keyholder bytes (so the MD5 input for case (b) is fully visible).
The generated types in [`generated/keyhash_cases.rs`](generated/keyhash_cases.rs)
were produced with `zerodds-idlc --rust` from the IDL; the harness in
[`src/main.rs`](src/main.rs) calls `DdsType::compute_key_hash()`.

## Reproduce the vendor side (on a host with the three SDKs)

### CycloneDDS — read the key via the public CDR-stream API
CycloneDDS does **not** put `PID_KEY_HASH` inline on the wire by default (it
deserializes the key on the reader), so we read the key from the same engine it
would use, via `dds_stream_extract_keyBE_from_data`, then apply the rule.

```sh
idlc keyhash_full.idl
cc -I$CYC/include cyclone_keyhash.c keyhash_full.c -o cyc_keyhash -L$CYC/lib -lddsc
./cyc_keyhash
```
Source: [`vendor/cyclone_keyhash.c`](vendor/cyclone_keyhash.c).

### RTI Connext — capture `PID_KEY_HASH` on the wire (authoritative)
RTI emits the keyhash inline by default. Publish one instance of each type over
UDPv4 loopback and read parameter `0x0070` from the DATA submessage.

```sh
rtiddsgen -language C++11 -unboundedSupport -d rti_gen keyhash_full.idl
# build rti_pubsub.cxx against $NDDSHOME (see RECORDED-KEYHASHES.txt for flags)
NDDS_QOS_PROFILES=file://$PWD/rti_loopback_qos.xml \
  tcpdump -i lo -w rti.pcap -U udp & ./rti_pubsub
tshark -r rti.pcap -Y "rtps.param.id==0x0070" -V | grep -A4 PID_KEY_HASH
```
tshark renders the 16-byte keyhash as a `guid:` field. Sources:
[`vendor/rti_pubsub.cxx`](vendor/rti_pubsub.cxx),
[`vendor/rti_loopback_qos.xml`](vendor/rti_loopback_qos.xml).

### Fast DDS — replicate the generated `compute_key()`
Serialize the key with Fast DDS' own FastCDR (`Cdr`, `BIG_ENDIANNESS`,
`XCDRv2`, no encapsulation) and hash with Fast DDS' own `MD5` class.

```sh
g++ -std=c++17 -I$FAST/include fastdds_keyhash.cpp -o fast_keyhash \
    -L$FAST/lib -lfastcdr -lfastdds
./fast_keyhash
```
Source: [`vendor/fastdds_keyhash.cpp`](vendor/fastdds_keyhash.cpp).

## Findings surfaced by this proof

1. **ZeroDDS Rust codegen rejects a nested-struct `@key` member.**
   `@key NestedKey k` (case (d)) fails code generation with
   `unsupported IDL construct 'complex @key field (sequence or nested struct)'`
   in `crates/idl-rust/src/struct_emit.rs` (`emit_key_field_write`). CycloneDDS,
   RTI, and Fast DDS all generate it. The *runtime* (`PlainCdr2BeKeyHolder`)
   handles it fine, and case (d) here proves the bytes match — the gap is purely
   in the Rust emitter. The other ZeroDDS language backends should be checked
   for the same limitation.

2. **Bounded `@key string<N>` always takes the MD5 branch in ZeroDDS.**
   `wire_size_bound` returns `None` for *all* strings (bounded or not), so the
   emitted `KEY_HOLDER_MAX_SIZE` is `None` ⇒ MD5. The spec says a bounded
   `string<N>` whose `4 + N + 1 ≤ 16` should take the **zero-pad** branch. A
   vendor that computes the static bound would zero-pad while ZeroDDS MD5s —
   the two would then never correlate the instance. (Case (b) here uses an
   *unbounded* string, where MD5 is correct for everyone, so all four still
   agree; the divergence appears only with a small bounded string key.)

   **Demonstrated divergence** — type `@final struct BS { @key string<8> code; }`,
   instance `code = "AB"`. The BE keyholder is `00 00 00 03 41 42 00` (7 bytes,
   max `4 + 8 + 1 = 13 ≤ 16`):
   * CycloneDDS (verified via its own CDR engine): zero-pad →
     `00000003414200000000000000000000`
   * ZeroDDS: MD5 → `afe1514800e44535d0f8280b7d11b82f`

   These never match, so the same instance correlates as two different
   instances across the wire. Fix: `wire_size_bound` (in
   `crates/idl-rust/src/type_map.rs`) must return `Some(4 + N + 1)` for a
   bounded `string<N>` (and the analogous `wstring<N>`), so
   `compute_key_holder_max_size` can pick the zero-pad branch when the total
   is ≤ 16. (Verify the other language backends compute the bound too.)

3. **Fast DDS does not zero alignment-pad bytes in the key buffer.**
   FastCDR leaves the inter-field alignment padding as whatever the
   InstanceHandle buffer held; the generated `compute_key` relies on that buffer
   being pre-zeroed. ZeroDDS' `PlainCdr2BeKeyHolder::pad_to` explicitly writes
   zero pad bytes, so it is robust by construction. With a zeroed buffer all
   three agree (demonstrated in the harness).

All three findings are reported, not fixed here, because they touch shared
codegen crates; this directory only adds a self-contained proof.
