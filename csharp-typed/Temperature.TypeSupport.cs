// Generated typed DDS support. Do not edit.
// Compile with the matching generated codec, ZeroDds.Wire and ZeroDDS.Cdr.
#nullable enable
namespace sensor_msgs.msg {

public sealed class TemperatureTypeSupport : global::ZeroDDS.Cdr.IDdsTopicType<global::sensor_msgs.msg.Temperature>, global::ZeroDDS.Cdr.IGeneratedWireTypeSupport
{
    public static readonly TemperatureTypeSupport Instance = new();
    public string TypeName => "sensor_msgs::msg::Temperature";
    public bool IsKeyed => false;
    public global::Omg.Types.ExtensibilityKind Extensibility => global::Omg.Types.ExtensibilityKind.Appendable;
    public int Representation => 1;
    public byte[] TypeObject => new byte[] { 0x70, 0x00, 0x00, 0x00, 0xf2, 0x51, 0x02, 0x00, 0x26, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x73, 0x65, 0x6e, 0x73, 0x6f, 0x72, 0x5f, 0x6d, 0x73, 0x67, 0x73, 0x3a, 0x3a, 0x6d, 0x73, 0x67, 0x3a, 0x3a, 0x54, 0x65, 0x6d, 0x70, 0x65, 0x72, 0x61, 0x74, 0x75, 0x72, 0x65, 0x00, 0x00, 0x00, 0x3c, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x63, 0x65, 0x6c, 0x73, 0x69, 0x75, 0x73, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x70, 0x00, 0x0a, 0x00, 0x00, 0x00, 0x73, 0x65, 0x6e, 0x73, 0x6f, 0x72, 0x5f, 0x69, 0x64, 0x00, 0x00, 0x00 };

    private static global::ZeroDds.Wire.Endianness Order(global::ZeroDDS.Cdr.EndianMode endian) => endian switch
    {
        global::ZeroDDS.Cdr.EndianMode.LittleEndian => global::ZeroDds.Wire.Endianness.Little,
        global::ZeroDDS.Cdr.EndianMode.BigEndian => global::ZeroDds.Wire.Endianness.Big,
        _ => throw new global::System.ArgumentOutOfRangeException(nameof(endian))
    };

    private static void RequireRepresentation(int representation)
    {
        if (representation != 1)
            throw new global::System.NotSupportedException("This generated codec does not implement the requested XCDR representation.");
    }

    public byte[] Encode(global::sensor_msgs.msg.Temperature sample) => Encode(sample, global::ZeroDDS.Cdr.EndianMode.LittleEndian, 1);
    public byte[] Encode(global::sensor_msgs.msg.Temperature sample, global::ZeroDDS.Cdr.EndianMode endian) => Encode(sample, endian, 1);
    public byte[] Encode(global::sensor_msgs.msg.Temperature sample, global::ZeroDDS.Cdr.EndianMode endian, int representation)
    {
        RequireRepresentation(representation);
        global::System.ArgumentNullException.ThrowIfNull(sample);
        var w = new global::ZeroDds.Wire.Writer(Order(endian), 4, new global::ZeroDds.Wire.PlCdr1Framing(global::ZeroDds.Wire.PidFlagPolicy.SentinelMustUnderstand, 16128, true, true, true, true), false);
        global::ZdCodec.zd_enc_sensor_msgs_msg_Temperature(w, sample);
        return w.Finish();
    }

    public global::sensor_msgs.msg.Temperature Decode(global::System.ReadOnlySpan<byte> bytes) => Decode(bytes, global::ZeroDDS.Cdr.EndianMode.LittleEndian, 1);
    public global::sensor_msgs.msg.Temperature Decode(global::System.ReadOnlySpan<byte> bytes, global::ZeroDDS.Cdr.EndianMode endian) => Decode(bytes, endian, 1);
    public global::sensor_msgs.msg.Temperature Decode(global::System.ReadOnlySpan<byte> bytes, global::ZeroDDS.Cdr.EndianMode endian, int representation)
    {
        RequireRepresentation(representation);
        var r = new global::ZeroDds.Wire.Reader(bytes.ToArray(), Order(endian), 4, new global::ZeroDds.Wire.PlCdr1Framing(global::ZeroDds.Wire.PidFlagPolicy.SentinelMustUnderstand, 16128, true, true, true, true));
        var sample = global::ZdCodec.zd_dec_sensor_msgs_msg_Temperature(r, global::ZeroDds.Wire.TryConstruct.Discard);
        if (r.ReaderStatus() != global::ZeroDds.Wire.Status.Ok || r.Remaining() != 0)
            throw new global::System.IO.InvalidDataException("Generated decoder refused the sample or did not consume its complete body.");
        return sample;
    }

    public byte[] KeyHash(global::sensor_msgs.msg.Temperature sample)
    {
        global::System.ArgumentNullException.ThrowIfNull(sample);
        return new byte[16];
    }

}
}

