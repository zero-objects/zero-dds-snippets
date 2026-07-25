// SPDX-License-Identifier: Apache-2.0
//! Functional tests for the async showcase: each `zerodds-dcps-async` function
//! is exercised and its effect asserted. Live participant over the loopback.

use std::time::Duration;

use futures::StreamExt;
use rust_async_showcase::Reading;
use tokio::time::timeout;
use zerodds_dcps_async::{
    AsyncDataReader, AsyncDataWriter, AsyncDomainParticipantFactory, DataReaderQos, DataWriterQos,
    PublisherQos, SubscriberQos, TopicQos,
};

const T: Duration = Duration::from_secs(5);

/// A matched writer/reader pair on a live participant.
async fn pair() -> (AsyncDataWriter<Reading>, AsyncDataReader<Reading>) {
    let factory = AsyncDomainParticipantFactory::instance();
    let p = factory.create_participant(0).expect("participant");
    let topic = p
        .create_topic::<Reading>("Async/Test", TopicQos::default())
        .expect("topic");
    let w = p
        .create_publisher(PublisherQos::default())
        .create_datawriter::<Reading>(&topic, DataWriterQos::default())
        .expect("writer");
    let r = p
        .create_subscriber(SubscriberQos::default())
        .create_datareader::<Reading>(&topic, DataReaderQos::default())
        .expect("reader");
    w.wait_for_matched_subscription(1, T).await.expect("w match");
    r.wait_for_matched_publication(1, T).await.expect("r match");
    (w, r)
}

#[tokio::test]
async fn write_then_take_roundtrips() {
    let (w, r) = pair().await;
    assert_eq!(w.matched_subscription_count(), 1);
    assert_eq!(r.matched_publication_count(), 1);

    w.write(&Reading { id: 1, value: 100 }).await.expect("write");
    let got = r.take(Duration::from_secs(2)).await.expect("take");
    assert_eq!(got.len(), 1);
    assert_eq!(got[0], Reading { id: 1, value: 100 });
}

#[tokio::test]
async fn take_stream_yields_samples() {
    let (w, r) = pair().await;
    let mut s = r.take_stream();
    w.write(&Reading { id: 2, value: 222 }).await.expect("write");
    let item = timeout(T, s.next()).await.expect("no timeout").expect("some");
    assert_eq!(item, Reading { id: 2, value: 222 });
}

#[tokio::test]
async fn data_available_stream_signals() {
    let (w, r) = pair().await;
    let mut avail = r.data_available_stream();
    w.write(&Reading { id: 3, value: 333 }).await.expect("write");
    timeout(T, avail.next()).await.expect("no timeout").expect("signal");
    // signalled but not consumed -> the sample is still there
    let got = r.take(Duration::from_secs(1)).await.expect("take");
    assert_eq!(got.len(), 1);
}

#[tokio::test]
async fn publication_matched_stream_reports_status() {
    let (_w, r) = pair().await;
    let mut m = r.publication_matched_stream();
    let status = timeout(T, m.next()).await.expect("no timeout").expect("status");
    assert!(status.current_count >= 1);
}

#[tokio::test]
async fn take_with_info_carries_sample_info() {
    let (w, r) = pair().await;
    w.write(&Reading { id: 5, value: 55 }).await.expect("write");
    let got = r.take_with_info(Duration::from_secs(2)).await.expect("take_with_info");
    assert_eq!(got.len(), 1);
    assert_eq!(got[0].data, Reading { id: 5, value: 55 });
    assert!(got[0].info.valid_data, "a live sample carries valid data");
}

#[tokio::test]
async fn read_is_non_consuming() {
    let (w, r) = pair().await;
    w.write(&Reading { id: 6, value: 66 }).await.expect("write");
    let seen = r.read(Duration::from_secs(2)).await.expect("read");
    assert_eq!(seen.len(), 1);
    // still there for take()
    let taken = r.take(Duration::from_secs(2)).await.expect("take");
    assert_eq!(taken.len(), 1);
}

#[tokio::test]
async fn take_stream_with_info_yields_sample_info() {
    let (w, r) = pair().await;
    let mut s = r.take_stream_with_info();
    w.write(&Reading { id: 7, value: 77 }).await.expect("write");
    let item = timeout(T, s.next()).await.expect("no timeout").expect("some");
    assert_eq!(item.data.value, 77);
    assert!(item.info.valid_data);
}

#[tokio::test]
async fn sql_content_filtered_reader_delivers_only_matching() {
    let factory = AsyncDomainParticipantFactory::instance();
    let p = factory.create_participant(0).expect("participant");
    let topic = p
        .create_topic::<Reading>("Async/Sql", TopicQos::default())
        .expect("topic");
    let w = p
        .create_publisher(PublisherQos::default())
        .create_datawriter::<Reading>(&topic, DataWriterQos::default())
        .expect("writer");
    let cft = p
        .create_contentfilteredtopic::<Reading>("Async/Sql/CFT", &topic, "value > %0", vec!["100".to_string()])
        .expect("cft");
    let r = p
        .create_subscriber(SubscriberQos::default())
        .create_datareader_cft::<Reading>(&cft, DataReaderQos::default())
        .expect("cft reader");
    w.wait_for_matched_subscription(1, T).await.expect("w match");
    r.wait_for_matched_publication(1, T).await.expect("r match");

    w.write(&Reading { id: 1, value: 25 }).await.expect("write low");
    w.write(&Reading { id: 1, value: 777 }).await.expect("write high");
    let got = r.take(Duration::from_secs(2)).await.expect("take");
    assert_eq!(got.len(), 1, "only value>100 delivered via SQL filter");
    assert_eq!(got[0].value, 777);
}

#[tokio::test]
async fn filtered_reader_delivers_only_matching() {
    let factory = AsyncDomainParticipantFactory::instance();
    let p = factory.create_participant(0).expect("participant");
    let topic = p
        .create_topic::<Reading>("Async/Filter", TopicQos::default())
        .expect("topic");
    let w = p
        .create_publisher(PublisherQos::default())
        .create_datawriter::<Reading>(&topic, DataWriterQos::default())
        .expect("writer");
    let r = p
        .create_subscriber(SubscriberQos::default())
        .create_datareader_filtered::<Reading, _>(&topic, DataReaderQos::default(), |x| x.value > 100)
        .expect("filtered reader");
    w.wait_for_matched_subscription(1, T).await.expect("w match");
    r.wait_for_matched_publication(1, T).await.expect("r match");

    w.write(&Reading { id: 1, value: 50 }).await.expect("write low");
    w.write(&Reading { id: 1, value: 500 }).await.expect("write high");
    let got = r.take(Duration::from_secs(2)).await.expect("take");
    assert_eq!(got.len(), 1, "only the value>100 sample is delivered");
    assert_eq!(got[0].value, 500);
}

#[tokio::test]
async fn instance_lifecycle_completes() {
    let (w, _r) = pair().await;
    let sample = Reading { id: 42, value: 7 };
    let handle = w.register_instance(&sample).await.expect("register");
    w.write(&sample).await.expect("write");
    w.dispose(&sample, handle).await.expect("dispose");
    w.unregister_instance(&sample, handle).await.expect("unregister");
}

#[tokio::test]
async fn spawn_in_tokio_creates_live_participant() {
    let factory = AsyncDomainParticipantFactory::instance();
    let handle = tokio::runtime::Handle::current();
    let live = factory.spawn_in_tokio(0, &handle).expect("spawn");
    let _t = live
        .create_topic::<Reading>("Async/Live", TopicQos::default())
        .expect("topic on live participant");
}
