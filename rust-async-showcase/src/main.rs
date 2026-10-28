// SPDX-License-Identifier: Apache-2.0
//! ZeroDDS async showcase — every function of the `zerodds-dcps-async`
//! (`zerodds-async-1.0`) surface in one runnable walkthrough.
//!
//! The async API is **runtime-agnostic** (only `futures-core` at its core);
//! this example drives it with Tokio. It wraps the sync DCPS API without state
//! duplication or overhead — the async newtypes share the same `Arc`.
//!
//! Covered here:
//!   * `AsyncDomainParticipantFactory` + participant / topic / pub / sub creation
//!   * `AsyncDataWriter`: `write`, `register_instance`, `dispose`,
//!     `unregister_instance`, `wait_for_matched_subscription`, counts, `qos`
//!   * `AsyncDataReader`: `take`, `read`, `take_with_info`, `read_with_info`,
//!     `wait_for_matched_publication`, counts, `qos`
//!   * Streams: `take_stream`, `take_stream_with_info`, `data_available_stream`,
//!     `subscription_matched_stream`
//!   * Content filtering: `create_datareader_filtered` (closure) and a SQL
//!     `ContentFilteredTopic` via `create_contentfilteredtopic` +
//!     `create_datareader_cft`
//!   * `spawn_in_tokio` (live participant driven by the Tokio runtime)
//!
//! Run: `cargo run`   Test: `cargo test`

use std::time::Duration;

use futures::StreamExt;
use rust_async_showcase::Reading;
use tokio::time::timeout;
use zerodds_dcps_async::{
    AsyncDomainParticipantFactory, DataReaderQos, DataWriterQos, PublisherQos, Result,
    SubscriberQos, TopicQos,
};

const T: Duration = Duration::from_secs(5);

#[tokio::main]
async fn main() -> Result<()> {
    // 1. Factory + participant. A live participant runs discovery over the
    //    loopback so the writer and reader below match.
    let factory = AsyncDomainParticipantFactory::instance();
    let participant = factory.create_participant(0)?;

    let topic = participant.create_topic::<Reading>("Async/Showcase", TopicQos::default())?;
    let publisher = participant.create_publisher(PublisherQos::default());
    let subscriber = participant.create_subscriber(SubscriberQos::default());
    let writer = publisher.create_datawriter::<Reading>(&topic, DataWriterQos::default())?;
    let reader = subscriber.create_datareader::<Reading>(&topic, DataReaderQos::default())?;

    // 2. Matched-handshake as awaitable futures (no thread blocking).
    writer.wait_for_matched_subscription(1, T).await?;
    reader.wait_for_matched_publication(1, T).await?;
    println!(
        "1) matched: writer sees {} subscription(s), reader sees {} publication(s)",
        writer.matched_subscription_count(),
        reader.matched_publication_count()
    );

    // 3. write().await + take(timeout).await
    writer.write(&Reading { id: 1, value: 100 }).await?;
    let batch = reader.take(Duration::from_secs(2)).await?;
    println!("2) take(): {} sample(s), first = {:?}", batch.len(), batch[0]);

    // 4. take_with_info(): each sample carries its SampleInfo (spec §2.2).
    writer.write(&Reading { id: 1, value: 110 }).await?;
    let with_info = reader.take_with_info(Duration::from_secs(2)).await?;
    if let Some(s) = with_info.first() {
        println!(
            "3) take_with_info(): value={} valid_data={} instance_state={:?}",
            s.data.value, s.info.valid_data, s.info.instance_state
        );
    }

    // 5. read(): non-consuming — leaves the sample in the reader.
    writer.write(&Reading { id: 1, value: 120 }).await?;
    let seen = reader.read(Duration::from_secs(2)).await?;
    let drained = reader.take(Duration::from_secs(2)).await?;
    println!(
        "4) read() (non-consuming): saw {}, then take() drained {}",
        seen.len(),
        drained.len()
    );

    // 6. take_stream(): a `Stream<Item = T>` that yields samples as they arrive.
    let mut samples = reader.take_stream();
    writer.write(&Reading { id: 1, value: 200 }).await?;
    if let Ok(Some(s)) = timeout(T, samples.next()).await {
        println!("5) take_stream(): got {s:?}");
    }
    drop(samples);

    // 7. take_stream_with_info(): `Stream<Item = Sample<T>>` (data + SampleInfo).
    let mut info_stream = reader.take_stream_with_info();
    writer.write(&Reading { id: 1, value: 210 }).await?;
    if let Ok(Some(s)) = timeout(T, info_stream.next()).await {
        println!("6) take_stream_with_info(): value={} valid_data={}", s.data.value, s.info.valid_data);
    }
    drop(info_stream);

    // 8. data_available_stream(): fired per arrival; signals without consuming.
    let mut available = reader.data_available_stream();
    writer.write(&Reading { id: 1, value: 300 }).await?;
    if timeout(T, available.next()).await.is_ok() {
        let fetched = reader.take(Duration::from_millis(500)).await?;
        println!("7) data_available_stream(): signalled, then took {} sample(s)", fetched.len());
    }
    drop(available);

    // 9. subscription_matched_stream(): fired when the reader's matched-writer count changes.
    let mut matched = reader.subscription_matched_stream();
    if let Ok(Some(status)) = timeout(T, matched.next()).await {
        println!(
            "8) subscription_matched_stream(): current={} total={}",
            status.current_count, status.total_count
        );
    }
    drop(matched);

    // 10. Closure content filter: only samples with value > 100 delivered.
    let filtered = subscriber.create_datareader_filtered::<Reading, _>(
        &topic,
        DataReaderQos::default(),
        |r| r.value > 100,
    )?;
    filtered.wait_for_matched_publication(1, T).await?;
    writer.write(&Reading { id: 9, value: 50 }).await?; // filtered out
    writer.write(&Reading { id: 9, value: 500 }).await?; // delivered
    let closure_batch = filtered.take(Duration::from_secs(2)).await?;
    println!(
        "9) create_datareader_filtered(value>100): {} sample(s), value(s) = {:?}",
        closure_batch.len(),
        closure_batch.iter().map(|r| r.value).collect::<Vec<_>>()
    );

    // 11. SQL content-filtered topic: same idea via a filter expression.
    let cft = participant.create_contentfilteredtopic::<Reading>(
        "Async/Showcase/CFT",
        &topic,
        "value > %0",
        vec!["100".to_string()],
    )?;
    let sql_reader = subscriber.create_datareader_cft::<Reading>(&cft, DataReaderQos::default())?;
    sql_reader.wait_for_matched_publication(1, T).await?;
    writer.write(&Reading { id: 8, value: 25 }).await?; // filtered out
    writer.write(&Reading { id: 8, value: 777 }).await?; // delivered
    let sql_batch = sql_reader.take(Duration::from_secs(2)).await?;
    println!(
        "10) create_datareader_cft(\"value > 100\"): {} sample(s), value(s) = {:?}",
        sql_batch.len(),
        sql_batch.iter().map(|r| r.value).collect::<Vec<_>>()
    );

    // 12. Instance lifecycle: register / write / dispose / unregister (keyed).
    let sample = Reading { id: 42, value: 7 };
    let handle = writer.register_instance(&sample).await?;
    writer.write(&sample).await?;
    writer.dispose(&sample, handle).await?;
    writer.unregister_instance(&sample, handle).await?;
    println!("11) instance lifecycle: register -> write -> dispose -> unregister ok");

    // 13. QoS accessors (sync, cheap).
    let _ = reader.qos();
    let _ = writer.qos();
    println!("12) qos(): reader + writer QoS retrieved");

    // 14. spawn_in_tokio: a *live* participant whose event loop is driven by the
    //     Tokio runtime, so the streams above become waker-driven (no polling).
    let handle = tokio::runtime::Handle::current();
    let live = factory.spawn_in_tokio(0, &handle)?;
    let _live_topic = live.create_topic::<Reading>("Async/Live", TopicQos::default())?;
    println!("13) spawn_in_tokio(): live async participant running on the Tokio runtime");

    println!("\nALL OK");
    Ok(())
}
