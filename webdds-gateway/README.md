# webdds-gateway

`zerodds-webdds-bridged`, the OMG Web-Enabled DDS 1.0 gateway, between
HTTP and an ordinary DDS program. Samples written over REST and over SOAP
arrive at the program; what the program writes is read back over REST.

- `idl/Sensor.idl` — `Demo::Sensor`, the one type both sides use: the
  gateway reads the file through `--type`, the program is built from the
  code idlc generates from it (`build.rs`).
- `src/main.rs` — the DDS side (`dds_side sub SECONDS | pub COUNT`),
  domain `ZERODDS_EXAMPLE_DOMAIN`.
- `run.sh` — starts the gateway, creates application, participant, topic,
  writer and reader over REST, and checks both directions.

## Run it

```sh
./run.sh
```

`ZERODDS_EXAMPLE_DOMAIN` (default 0) and `WEBDDS_PORT` (default 8080)
change the domain and the HTTP port. The script builds the gateway from
this tree.

Output of the runs this README was written from (Linux x86_64, three runs,
domain 63):

```
dds_side: Sensor sensor_id=6 value=6.5
dds_side: took 6 Sensor
dds_side: wrote 3 Sensor
<data><sensor_id>100</sensor_id><value>10</value></data>
WEBDDS GATEWAY OK: HTTP->DDS 6 of 6, DDS->HTTP 3 of 3
```

`sensor_id=6` is the sample written over SOAP; 1 to 5 went over REST.

## The requests

Every REST request names `Accept: application/dds-web+xml`, and one with
a body names it as `Content-Type` too: `curl -d` alone sends
`application/x-www-form-urlencoded`, which the gateway answers with 415.

A sample is its type's members as elements, the same over REST and SOAP:

```
POST /dds/rest1/applications/app1/domain_participants/p1/publishers/pub1/data_writers/dw1
<data><sensor_id>1</sensor_id><value>1.5</value></data>
```

```xml
<soap:Envelope xmlns:soap="http://www.w3.org/2003/05/soap-envelope"><soap:Body>
  <WriteSample><app>app1</app><participant>p1</participant><publisher>pub1</publisher>
    <writer>dw1</writer><data><sensor_id>6</sensor_id><value>6.5</value></data></WriteSample>
</soap:Body></soap:Envelope>
```

A GET on the data reader returns what it holds as
`<read_sample_seq><data>…</data>…</read_sample_seq>`.
