// SPDX-License-Identifier: Apache-2.0
//
// native-frontends/go ping-pong: an idlc-typed Ping/Pong exchange over the Go
// endpoint SDK, both sync and async. The generated types (types.go) marshal a
// typed `Ping` to XCDR2; the SDK frames it as an XRCE WRITE_DATA and hands it to
// a UDP transport; the ZeroDDS peer decodes it and replies a typed `Pong`.
//
//	go run . <sync|async> <peer-port>
package main

import (
	"fmt"
	"net"
	"os"
	"time"

	zdgen "pingpong/generated"
	"zdwire"
	zerodds "zeroddsendpoint"
)

// udpTransport is a live UDP zerodds.Transport (Deliver + non-blocking Receive).
type udpTransport struct {
	conn *net.UDPConn
	peer *net.UDPAddr
}

func (u *udpTransport) Deliver(frame []byte) error {
	_, err := u.conn.WriteToUDP(frame, u.peer)
	return err
}

func (u *udpTransport) Receive(buf []byte) (int, bool, error) {
	_ = u.conn.SetReadDeadline(time.Now().Add(10 * time.Millisecond))
	n, _, err := u.conn.ReadFromUDP(buf)
	if err != nil {
		if ne, ok := err.(net.Error); ok && ne.Timeout() {
			return 0, true, nil
		}
		return 0, false, err
	}
	return n, false, nil
}

func main() {
	if len(os.Args) < 3 {
		fmt.Fprintln(os.Stderr, "usage: go run . <sync|async> <peer-port>")
		os.Exit(2)
	}
	mode := os.Args[1]
	if mode != "sync" && mode != "async" {
		panic("mode must be sync or async")
	}
	peer, err := net.ResolveUDPAddr("udp", "127.0.0.1:"+os.Args[2])
	if err != nil {
		panic(err)
	}
	conn, err := net.ListenUDP("udp", nil)
	if err != nil {
		panic(err)
	}
	defer conn.Close()
	tr := &udpTransport{conn: conn, peer: peer}

	// idlc-typed: marshal a real Ping (the generated type), not raw bytes.
	wire := zdwire.NewWriter(zdwire.LittleEndian, 4, zdwire.SpecificationFraming(), false)
	zdgen.Zd_enc_Ping(wire, zdgen.Ping{Seq: 1, Msg: "hello from app"})
	sample := wire.Finish()

	var body []byte
	if mode == "async" {
		reader := zerodds.NewAsyncReader(tr)
		defer reader.Close()
		if err := zerodds.NewAsyncWriter(tr, 0x0015).Write(sample); err != nil {
			panic(err)
		}
		select {
		case body = <-reader.Samples:
		case <-time.After(10 * time.Second):
			panic("async: no pong")
		}
	} else {
		client := zerodds.NewClient(tr, 0x0015)
		if err := client.Write(sample); err != nil {
			panic(err)
		}
		var ok bool
		body, ok, err = client.Receive(10 * time.Second)
		if err != nil {
			panic(err)
		}
		if !ok {
			panic("sync: no pong")
		}
	}

	reader := zdwire.NewReader(body, zdwire.LittleEndian, 4, "sentinel-must-understand", zdwire.PidShortMaxID)
	pong := zdgen.Zd_dec_Pong(reader, zdwire.TryConstructDiscard)
	if reader.ReaderStatus() != zdwire.StatusOk || reader.ReaderRemaining() != 0 {
		panic("Pong refused or trailing bytes remain")
	}
	if pong.Seq != 1 || pong.Reply != "pong:hello from app" {
		panic("Pong values differ from the expected reply")
	}
	fmt.Printf("PONG seq=%d reply=%s\n", pong.Seq, pong.Reply)
}
