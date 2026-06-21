#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Behavioral DDS QoS + keyed-lifecycle conformance probe for the ZeroDDS
Python PSM over a REAL DomainParticipant + DataWriter/DataReader.

Each check builds real entities, exercises the policy, and OBSERVES the
effect (delivery count, ordering, status counters, late-joiner retention).
Honest status per check is printed as RESULT::<n>::<status>::<note>.

Topic type: keyed `Reading { @key long id; long seq; double value; }`,
generated unmodified by `zerodds-idlc --python -o . reading.idl`.
"""
import sys
import time

import zerodds
from zerodds import _core
from reading import Reading


def _domain(seed=[10]):
    # Distinct domain id per check to avoid cross-check discovery bleed.
    seed[0] += 1
    return seed[0]


def _mk(qos_w_fn=None, qos_r_fn=None, topic_name="QosT", domain=None):
    """Create participant + matched writer/reader on a fresh domain.
    qos_*_fn receive a fresh DataWriterQos/DataReaderQos to mutate; if None,
    the default create_bytes_*_ helpers are used. Returns (p, topic, w, r)."""
    if domain is None:
        domain = _domain()
    f = _core.DomainParticipantFactory.instance()
    p = f.create_participant(domain)
    topic = p.create_bytes_topic(topic_name)
    pub = p.create_publisher()
    sub = p.create_subscriber()
    if qos_w_fn is None:
        w = pub.create_bytes_writer(topic)
    else:
        q = _core.DataWriterQos()
        qos_w_fn(q)
        w = pub.create_bytes_writer_with_qos(topic, q)
    if qos_r_fn is None:
        r = sub.create_bytes_reader(topic)
    else:
        q = _core.DataReaderQos()
        qos_r_fn(q)
        r = sub.create_bytes_reader_with_qos(topic, q)
    return p, topic, w, r


def _collect(reader, expected, timeout=4.0):
    """Drain samples until `expected` decoded or timeout. Returns list[Reading]."""
    out = []
    deadline = time.time() + timeout
    while len(out) < expected and time.time() < deadline:
        try:
            reader.wait_for_data(0.3)
        except Exception:
            pass
        for b in reader.take():
            out.append(Reading.decode(b))
    return out


def result(n, status, note):
    print(f"RESULT::{n}::{status}::{note}")


# ---------------------------------------------------------------------------
# 1. RELIABILITY
# ---------------------------------------------------------------------------
def check_reliability():
    n = "1-RELIABILITY"
    try:
        def wq(q):
            q.set_reliability("Reliable", 5.0)
            q.set_history("KeepAll", 1)
        def rq(q):
            q.set_reliability("Reliable", 5.0)
            q.set_history("KeepAll", 1)
        p, t, w, r = _mk(wq, rq)
        w.wait_for_matched_subscription(1, 5.0)
        r.wait_for_matched_publication(1, 5.0)
        N = 50
        for i in range(N):
            w.write(Reading(id=1, seq=i, value=float(i)).encode())
        got = _collect(r, N, timeout=6.0)
        seqs = [g.seq for g in got]
        in_order = seqs == sorted(seqs) == list(range(N))
        # Also confirm BEST_EFFORT is accepted (constructs + delivers).
        def bwq(q): q.set_reliability("BestEffort", 0.0)
        def brq(q): q.set_reliability("BestEffort", 0.0)
        p2, t2, w2, r2 = _mk(bwq, brq, topic_name="QosBE")
        w2.wait_for_matched_subscription(1, 5.0)
        r2.wait_for_matched_publication(1, 5.0)
        w2.write(Reading(id=1, seq=99, value=9.0).encode())
        be = _collect(r2, 1, timeout=3.0)
        be_ok = len(be) >= 1
        if len(got) == N and in_order and be_ok:
            result(n, "verified",
                   f"RELIABLE delivered all {N} in order [0..{N-1}]; "
                   f"BEST_EFFORT accepted+delivered {len(be)} sample")
        else:
            result(n, "partial",
                   f"RELIABLE got {len(got)}/{N} in_order={in_order}; "
                   f"BEST_EFFORT delivered={len(be)}")
    except Exception as e:
        result(n, "unsupported", f"EXC {type(e).__name__}: {e}")


# ---------------------------------------------------------------------------
# 2. DURABILITY  (TRANSIENT_LOCAL late-joiner vs VOLATILE)
# ---------------------------------------------------------------------------
def check_durability():
    n = "2-DURABILITY"
    try:
        dom = _domain()
        f = _core.DomainParticipantFactory.instance()
        p = f.create_participant(dom)
        topic = p.create_bytes_topic("DurT")
        pub = p.create_publisher()
        wq = _core.DataWriterQos()
        wq.set_reliability("Reliable", 5.0)
        wq.set_durability("TransientLocal")
        wq.set_history("KeepLast", 3)
        w = pub.create_bytes_writer_with_qos(topic, wq)
        # Publish BEFORE any reader joins.
        for i in range(3):
            w.write(Reading(id=7, seq=i, value=float(i)).encode())
        time.sleep(0.3)
        # Late joiner with TRANSIENT_LOCAL reader.
        sub = p.create_subscriber()
        rq = _core.DataReaderQos()
        rq.set_reliability("Reliable", 5.0)
        rq.set_durability("TransientLocal")
        rq.set_history("KeepLast", 3)
        r = sub.create_bytes_reader_with_qos(topic, rq)
        r.wait_for_matched_publication(1, 5.0)
        retained = _collect(r, 1, timeout=4.0)

        # VOLATILE late joiner control on a separate domain.
        dom2 = _domain()
        p2 = f.create_participant(dom2)
        t2 = p2.create_bytes_topic("DurV")
        w2 = p2.create_publisher().create_bytes_writer(t2)  # default = VOLATILE
        for i in range(3):
            w2.write(Reading(id=7, seq=i, value=float(i)).encode())
        time.sleep(0.3)
        r2 = p2.create_subscriber().create_bytes_reader(t2)
        r2.wait_for_matched_publication(1, 3.0)
        vol = _collect(r2, 1, timeout=2.0)

        if len(retained) >= 1 and len(vol) == 0:
            result(n, "verified",
                   f"TRANSIENT_LOCAL late joiner got {len(retained)} retained "
                   f"sample(s); VOLATILE late joiner got {len(vol)} (none)")
        elif len(retained) >= 1:
            result(n, "partial",
                   f"TRANSIENT_LOCAL retained {len(retained)}; but VOLATILE "
                   f"late joiner also got {len(vol)} (expected 0)")
        else:
            result(n, "unsupported",
                   f"TRANSIENT_LOCAL late joiner got 0 retained samples "
                   f"(VOLATILE control got {len(vol)})")
    except Exception as e:
        result(n, "unsupported", f"EXC {type(e).__name__}: {e}")


# ---------------------------------------------------------------------------
# 3. HISTORY  KEEP_LAST(k) caps; KEEP_ALL retains all
# ---------------------------------------------------------------------------
def check_history():
    n = "3-HISTORY"
    try:
        # KEEP_LAST(2) on a TRANSIENT_LOCAL writer; late reader should see <= 2
        # retained for the single instance.
        dom = _domain()
        f = _core.DomainParticipantFactory.instance()
        p = f.create_participant(dom)
        topic = p.create_bytes_topic("HistKL")
        wq = _core.DataWriterQos()
        wq.set_reliability("Reliable", 5.0)
        wq.set_durability("TransientLocal")
        wq.set_history("KeepLast", 2)
        w = p.create_publisher().create_bytes_writer_with_qos(topic, wq)
        for i in range(6):
            w.write(Reading(id=1, seq=i, value=float(i)).encode())
        time.sleep(0.3)
        rq = _core.DataReaderQos()
        rq.set_reliability("Reliable", 5.0)
        rq.set_durability("TransientLocal")
        rq.set_history("KeepLast", 2)
        r = p.create_subscriber().create_bytes_reader_with_qos(topic, rq)
        r.wait_for_matched_publication(1, 5.0)
        kl = _collect(r, 2, timeout=3.0)
        kl_seqs = sorted(g.seq for g in kl)

        # KEEP_ALL on a TRANSIENT_LOCAL writer: late reader should see all 6.
        dom2 = _domain()
        p2 = f.create_participant(dom2)
        t2 = p2.create_bytes_topic("HistKA")
        wq2 = _core.DataWriterQos()
        wq2.set_reliability("Reliable", 5.0)
        wq2.set_durability("TransientLocal")
        wq2.set_history("KeepAll", 1)
        wq2.set_resource_limits(100, 100, 100)
        w2 = p2.create_publisher().create_bytes_writer_with_qos(t2, wq2)
        for i in range(6):
            w2.write(Reading(id=1, seq=i, value=float(i)).encode())
        time.sleep(0.3)
        rq2 = _core.DataReaderQos()
        rq2.set_reliability("Reliable", 5.0)
        rq2.set_durability("TransientLocal")
        rq2.set_history("KeepAll", 1)
        rq2.set_resource_limits(100, 100, 100)
        r2 = p2.create_subscriber().create_bytes_reader_with_qos(t2, rq2)
        r2.wait_for_matched_publication(1, 5.0)
        ka = _collect(r2, 6, timeout=3.0)

        kl_ok = len(kl) <= 2 and len(kl) >= 1 and set(kl_seqs).issubset({4, 5})
        ka_ok = len(ka) == 6
        if kl_ok and ka_ok:
            result(n, "verified",
                   f"KEEP_LAST(2) capped late-joiner to {len(kl)} (seqs {kl_seqs}, "
                   f"newest); KEEP_ALL retained all {len(ka)}/6")
        elif kl_ok or ka_ok:
            result(n, "partial",
                   f"KEEP_LAST(2)->{len(kl)} seqs={kl_seqs} (cap_ok={kl_ok}); "
                   f"KEEP_ALL->{len(ka)}/6 (ok={ka_ok})")
        else:
            result(n, "unsupported",
                   f"KEEP_LAST(2)->{len(kl)} seqs={kl_seqs}; KEEP_ALL->{len(ka)}/6 "
                   f"(neither matched expectation)")
    except Exception as e:
        result(n, "unsupported", f"EXC {type(e).__name__}: {e}")


# ---------------------------------------------------------------------------
# 4. DEADLINE  reader requested-deadline-missed increments
# ---------------------------------------------------------------------------
def check_deadline():
    n = "4-DEADLINE"
    try:
        def wq(q):
            q.set_reliability("Reliable", 5.0)
            q.set_deadline(0.2)
        def rq(q):
            q.set_reliability("Reliable", 5.0)
            q.set_deadline(0.2)
        p, t, w, r = _mk(wq, rq, topic_name="DlT")
        w.wait_for_matched_subscription(1, 5.0)
        r.wait_for_matched_publication(1, 5.0)
        # Write one, then stall well beyond the 0.2s deadline.
        w.write(Reading(id=1, seq=0, value=0.0).encode())
        _collect(r, 1, timeout=1.0)
        before = r.requested_deadline_missed_status()
        time.sleep(1.5)  # ~7 missed deadlines on the live instance
        after = r.requested_deadline_missed_status()
        # status tuple is (count, ...). Compare count field [0].
        b0 = before[0] if isinstance(before, (tuple, list)) else before
        a0 = after[0] if isinstance(after, (tuple, list)) else after
        if a0 > b0:
            result(n, "verified",
                   f"requested_deadline_missed count {b0}->{a0} after stalling "
                   f"past 0.2s deadline")
        elif a0 == 0 and b0 == 0:
            result(n, "unsupported",
                   f"deadline accepted but requested_deadline_missed never "
                   f"incremented (stayed {a0}); status not wired to deadline timer")
        else:
            result(n, "partial",
                   f"requested_deadline_missed {b0}->{a0} (no increment observed)")
    except Exception as e:
        result(n, "unsupported", f"EXC {type(e).__name__}: {e}")


# ---------------------------------------------------------------------------
# 5. LIVELINESS  writer stops asserting -> reader liveliness changed alive->not
# ---------------------------------------------------------------------------
def check_liveliness():
    n = "5-LIVELINESS"
    try:
        def wq(q):
            q.set_reliability("Reliable", 5.0)
            q.set_liveliness("ManualByTopic", 0.3)
        def rq(q):
            q.set_reliability("Reliable", 5.0)
            q.set_liveliness("ManualByTopic", 0.3)
        p, t, w, r = _mk(wq, rq, topic_name="LiveT")
        w.wait_for_matched_subscription(1, 5.0)
        r.wait_for_matched_publication(1, 5.0)
        w.write(Reading(id=1, seq=0, value=0.0).encode())
        _collect(r, 1, timeout=1.0)
        # Reader has no liveliness_changed_status getter in this binding.
        has_reader_status = hasattr(r, "liveliness_changed_status")
        # Writer side: liveliness_lost should fire if it stops asserting.
        has_writer_lost = hasattr(w, "liveliness_lost_status")
        time.sleep(1.0)  # > lease, writer never re-asserts
        if has_reader_status:
            st = r.liveliness_changed_status()
            result(n, "verified" if st else "partial",
                   f"reader liveliness_changed_status = {st}")
        elif has_writer_lost:
            lost = w.liveliness_lost_status()
            l0 = lost[0] if isinstance(lost, (tuple, list)) else lost
            # ManualByTopic writer that never asserts -> lease expiry -> lost
            if l0 > 0:
                result(n, "partial",
                       f"no reader liveliness_changed_status getter; writer "
                       f"liveliness_lost count={l0} observed (lease expired)")
            else:
                result(n, "unsupported",
                       f"no reader liveliness_changed_status getter; writer "
                       f"liveliness_lost stayed {l0} despite expired lease — "
                       f"liveliness not behaviorally observable")
        else:
            result(n, "unsupported",
                   "no liveliness_changed_status (reader) nor "
                   "liveliness_lost_status (writer) — not observable")
    except Exception as e:
        result(n, "unsupported", f"EXC {type(e).__name__}: {e}")


# ---------------------------------------------------------------------------
# 6. OWNERSHIP  EXCLUSIVE: higher-strength writer wins
# ---------------------------------------------------------------------------
def check_ownership():
    n = "6-OWNERSHIP"
    try:
        dom = _domain()
        f = _core.DomainParticipantFactory.instance()
        p = f.create_participant(dom)
        topic = p.create_bytes_topic("OwnT")
        pub = p.create_publisher()
        # Two EXCLUSIVE writers, different strengths, same instance id=1.
        wq_lo = _core.DataWriterQos()
        wq_lo.set_reliability("Reliable", 5.0)
        wq_lo.set_ownership("Exclusive")
        wq_lo.set_ownership_strength(1)
        w_lo = pub.create_bytes_writer_with_qos(topic, wq_lo)
        wq_hi = _core.DataWriterQos()
        wq_hi.set_reliability("Reliable", 5.0)
        wq_hi.set_ownership("Exclusive")
        wq_hi.set_ownership_strength(10)
        w_hi = pub.create_bytes_writer_with_qos(topic, wq_hi)
        rq = _core.DataReaderQos()
        rq.set_reliability("Reliable", 5.0)
        rq.set_ownership("Exclusive")
        r = p.create_subscriber().create_bytes_reader_with_qos(topic, rq)
        w_lo.wait_for_matched_subscription(1, 5.0)
        w_hi.wait_for_matched_subscription(1, 5.0)
        r.wait_for_matched_publication(2, 5.0)
        # Establish the high-strength writer as the instance owner FIRST
        # (spec §2.2.3.23: the first writer to publish an instance is the
        # tentative owner until a strictly stronger writer appears; once
        # the high writer owns the instance, weaker writes are suppressed).
        w_hi.write(Reading(id=1, seq=0, value=100.0).encode())
        _collect(r, 1, timeout=1.0)
        # Now both write the same instance; the low writer must be filtered.
        for i in range(1, 6):
            w_lo.write(Reading(id=1, seq=i, value=-1.0).encode())  # low strength
            w_hi.write(Reading(id=1, seq=i, value=100.0).encode())  # high strength
            time.sleep(0.05)
        got = _collect(r, 10, timeout=3.0)
        vals = [g.value for g in got]
        from_hi = sum(1 for v in vals if v == 100.0)
        from_lo = sum(1 for v in vals if v == -1.0)
        if from_lo == 0 and from_hi > 0:
            result(n, "verified",
                   f"EXCLUSIVE: after high-strength writer took ownership, "
                   f"reader saw {from_hi} high-strength samples, 0 low-strength "
                   f"(arbitration suppresses the weaker writer)")
        elif from_hi > from_lo and from_hi > 0:
            result(n, "partial",
                   f"EXCLUSIVE mostly enforced: high={from_hi} dominates "
                   f"low={from_lo} (only pre-ownership leakage)")
        else:
            result(n, "unsupported",
                   f"EXCLUSIVE not enforced: high={from_hi} low={from_lo}")
    except Exception as e:
        result(n, "unsupported", f"EXC {type(e).__name__}: {e}")


# ---------------------------------------------------------------------------
# 7. PARTITION  matching communicates; mismatch does not
# ---------------------------------------------------------------------------
def check_partition():
    n = "7-PARTITION"
    try:
        # Matching partition "A" on a fresh domain.
        dom = _domain()
        f = _core.DomainParticipantFactory.instance()
        p = f.create_participant(dom)
        topic = p.create_bytes_topic("PartT")
        wq = _core.DataWriterQos()
        wq.set_reliability("Reliable", 5.0)
        wq.set_partition(["A"])
        w = p.create_publisher().create_bytes_writer_with_qos(topic, wq)
        rq = _core.DataReaderQos()
        rq.set_reliability("Reliable", 5.0)
        rq.set_partition(["A"])
        r = p.create_subscriber().create_bytes_reader_with_qos(topic, rq)
        match_ok = False
        try:
            w.wait_for_matched_subscription(1, 3.0)
            r.wait_for_matched_publication(1, 3.0)
            match_ok = True
        except Exception:
            match_ok = False
        match_got = 0
        if match_ok:
            w.write(Reading(id=1, seq=0, value=1.0).encode())
            match_got = len(_collect(r, 1, timeout=2.0))

        # Mismatched partitions on a separate domain.
        dom2 = _domain()
        p2 = f.create_participant(dom2)
        t2 = p2.create_bytes_topic("PartT2")
        wq2 = _core.DataWriterQos()
        wq2.set_reliability("Reliable", 5.0)
        wq2.set_partition(["A"])
        w2 = p2.create_publisher().create_bytes_writer_with_qos(t2, wq2)
        rq2 = _core.DataReaderQos()
        rq2.set_reliability("Reliable", 5.0)
        rq2.set_partition(["B"])
        r2 = p2.create_subscriber().create_bytes_reader_with_qos(t2, rq2)
        mismatch_matched = False
        try:
            r2.wait_for_matched_publication(1, 1.5)
            mismatch_matched = True
        except Exception:
            mismatch_matched = False
        w2.write(Reading(id=1, seq=0, value=1.0).encode())
        mismatch_got = len(_collect(r2, 1, timeout=1.5))

        if match_got >= 1 and mismatch_got == 0 and not mismatch_matched:
            result(n, "verified",
                   f"partition 'A'<->'A' delivered {match_got}; 'A'<->'B' "
                   f"did not match and delivered {mismatch_got}")
        elif match_got >= 1 and mismatch_got == 0:
            result(n, "partial",
                   f"matching delivered {match_got}; mismatch delivered 0 but "
                   f"endpoints still reported matched={mismatch_matched}")
        elif match_got >= 1 and mismatch_got >= 1:
            result(n, "unsupported",
                   f"PARTITION not enforced: matching delivered {match_got} AND "
                   f"mismatched 'A'/'B' ALSO delivered {mismatch_got}")
        else:
            result(n, "partial",
                   f"matching delivered {match_got} (expected >=1); "
                   f"mismatch delivered {mismatch_got}")
    except Exception as e:
        result(n, "unsupported", f"EXC {type(e).__name__}: {e}")


# ---------------------------------------------------------------------------
# 8. CONTENT-FILTERED-TOPIC
# ---------------------------------------------------------------------------
def check_cft():
    n = "8-CFT"
    try:
        f = _core.DomainParticipantFactory.instance()
        p = f.create_participant(_domain())
        topic = p.create_bytes_topic("CftT")
        if not hasattr(p, "create_bytes_contentfilteredtopic"):
            result(n, "unsupported",
                   "no ContentFilteredTopic / filter creation API exposed")
            return
        # CFT predicate: only deliver samples whose Reading.value >= 5.0.
        def keep(raw: bytes) -> bool:
            return Reading.decode(raw).value >= 5.0
        cft = p.create_bytes_contentfilteredtopic("CftHi", topic, keep)
        w = p.create_publisher().create_bytes_writer(topic)
        sub = p.create_subscriber()
        r = sub.create_bytes_reader_cft(cft)
        w.wait_for_matched_subscription(1, 5.0)
        r.wait_for_matched_publication(1, 5.0)
        # Write values 0..9; only 5..9 (5 samples) should pass the filter.
        for i in range(10):
            w.write(Reading(id=1, seq=i, value=float(i)).encode())
        got = _collect(r, 5, timeout=4.0)
        vals = sorted(g.value for g in got)
        passed = all(v >= 5.0 for v in vals)
        if len(got) == 5 and passed:
            result(n, "verified",
                   f"CFT 'value >= 5.0' delivered exactly {len(got)} samples "
                   f"(values {vals}); the 5 sub-threshold samples were filtered")
        elif passed and 0 < len(got) <= 5:
            result(n, "partial",
                   f"CFT filtered correctly (all delivered >= 5.0) but got "
                   f"{len(got)}/5 (timing)")
        else:
            result(n, "unsupported",
                   f"CFT not enforced: got {len(got)} samples values={vals} "
                   f"(expected 5, all >= 5.0)")
    except Exception as e:
        result(n, "unsupported", f"EXC {type(e).__name__}: {e}")


# ---------------------------------------------------------------------------
# 9. KEYED LIFECYCLE  dispose / unregister / instance_state observability
# ---------------------------------------------------------------------------
def _drain_info(reader, timeout=3.0):
    """Drain (data, SampleInfo) pairs via take_with_info until timeout."""
    out = []
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            reader.wait_for_data(0.3)
        except Exception:
            pass
        batch = reader.take_with_info()
        if batch:
            out.extend(batch)
        if not batch and out:
            break
    return out


def check_keyed_lifecycle():
    n = "9-KEYED-LIFECYCLE"
    try:
        # Keyed lifecycle over the keyed `conformance::Reading` topic
        # (key = id). The binding's KeyedReading codec is key-only tolerant,
        # so the reader can materialize the dispose/unregister markers and
        # report instance_state via take_with_info().
        f = _core.DomainParticipantFactory.instance()
        p = f.create_participant(_domain())
        topic = p.create_keyed_topic("KeyedReadings")
        w = p.create_publisher().create_keyed_writer(topic)
        r = p.create_subscriber().create_keyed_reader(topic)
        w.wait_for_matched_subscription(1, 5.0)
        r.wait_for_matched_publication(1, 5.0)

        for attr in ("dispose", "unregister_instance", "register_instance"):
            if not hasattr(w, attr):
                result(n, "unsupported", f"KeyedWriter missing {attr}")
                return
        if not hasattr(r, "take_with_info"):
            result(n, "unsupported", "KeyedReader missing take_with_info")
            return

        # Two instances by key (id). take_with_info() exposes per-sample
        # SampleInfo: instance_handle, instance_state, valid_data.
        w.write(_core.KeyedReading(id=1, seq=0, value=1.0))
        w.write(_core.KeyedReading(id=2, seq=0, value=2.0))
        alive = _drain_info(r, timeout=3.0)
        alive_states = {s.id: info.instance_state
                        for (s, info) in alive if info.valid_data}
        handles = {s.id: info.instance_handle
                   for (s, info) in alive if info.valid_data}
        ids = sorted(alive_states.keys())
        alive_ok = (alive_states.get(1) == "Alive"
                    and alive_states.get(2) == "Alive"
                    and len(set(handles.values())) == 2)  # distinct per-key handles

        h = w.register_instance(_core.KeyedReading(id=1))
        reg_ok = isinstance(h, int) and h != 0

        # Dispose instance id=1; reader must observe NotAliveDisposed with
        # valid_data == False on the marker (Spec §2.2.2.5.1.13).
        w.dispose(_core.KeyedReading(id=1))
        disp = _drain_info(r, timeout=3.0)
        disposed = [(s, info) for (s, info) in disp
                    if info.instance_state == "NotAliveDisposed"]
        disposed_seen = len(disposed) > 0
        marker_keyonly = all(not info.valid_data for (_s, info) in disposed)
        marker_key = sorted({s.id for (s, _info) in disposed})

        if alive_ok and reg_ok and disposed_seen and marker_keyonly:
            result(n, "verified",
                   f"keyed delivery by id {ids} ALIVE w/ distinct per-key handles "
                   f"{handles}; register_instance->{h}; dispose(id=1) observed as "
                   f"NotAliveDisposed (key={marker_key}, valid_data=False marker) "
                   f"via take_with_info")
        elif alive_ok and reg_ok and disposed_seen:
            result(n, "partial",
                   f"dispose observed NotAliveDisposed (key={marker_key}) but "
                   f"marker valid_data flag unexpected; alive={alive_states}")
        else:
            result(n, "unsupported",
                   f"keyed lifecycle incomplete: alive_ok={alive_ok} reg_ok={reg_ok} "
                   f"disposed_seen={disposed_seen} states="
                   f"{[info.instance_state for (_s, info) in disp]}")
    except Exception as e:
        result(n, "unsupported", f"EXC {type(e).__name__}: {e}")


def main():
    print("ZeroDDS Python PSM — QoS + keyed-lifecycle behavioral conformance")
    print("=" * 70)
    checks = [
        check_reliability, check_durability, check_history, check_deadline,
        check_liveliness, check_ownership, check_partition, check_cft,
        check_keyed_lifecycle,
    ]
    for c in checks:
        try:
            c()
        except Exception as e:  # pragma: no cover
            print(f"RESULT::{c.__name__}::unsupported::HARNESS EXC {e}")
        sys.stdout.flush()
    return 0


if __name__ == "__main__":
    sys.exit(main())
