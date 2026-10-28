// SPDX-License-Identifier: Apache-2.0
// Example receiver: a pure-Java endpoint receives a DATA message the hub pushes
// over UDP and decodes the SensorReading (ADR 0013).
//   java ReceiveUdp <port>
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.util.Arrays;

public class ReceiveUdp {
    public static void main(String[] a) throws Exception {
        int port = a.length > 0 ? Integer.parseInt(a[0]) : 7447;
        DatagramSocket s = new DatagramSocket(port);
        s.setSoTimeout(10000);
        System.out.println("java receiver: listening on udp/" + port);
        byte[] buf = new byte[2048];
        DatagramPacket p = new DatagramPacket(buf, buf.length);
        s.receive(p);
        byte[] frame = Arrays.copyOf(buf, p.getLength());
        byte[] body = ZdwEndpoint.xrceReadBody(frame);
        // The same version the publisher wrote; a reader capped at 8 would
        // read an XCDR2 double four octets late.
        Zdw.Reader r = new Zdw.Reader(body, Zdw.LE, Zdw.XCDR2);
        long id = r.u32(); int kind = r.u16(); int flags = r.u8();
        float value = r.f32(); long stamp = r.u64();
        String label = r.str(); byte[] raw = r.seqU8();
        if (id != 0xA1B2C3D4L || kind != 0x1234 || flags != 0x5A
            || value != 3.5f || stamp != 0x0102030405060708L || !"bay-12".equals(label)
            || !Arrays.equals(raw, new byte[] {(byte)0xDE, (byte)0xAD, (byte)0xBE, (byte)0xEF}))
            throw new IllegalStateException("mismatch");
        System.out.println("JAVA RECEIVER OK: id=0x" + Long.toHexString(id).toUpperCase()
            + " label=" + label + " value=" + value);
        s.close();
    }
}
