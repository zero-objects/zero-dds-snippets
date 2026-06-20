package sensor_msgs.msg;

// Stands in for gen/sensor_msgs/msg/Temperature.java emitted by `idlc java`
// from the temperature.idl struct { long celsius; string sensor_id; }.
// Default constructor + bean getters/setters, IDL field names unchanged.
public class Temperature {
    private int celsius;
    private String sensor_id;

    public Temperature() {}

    public int getCelsius() { return celsius; }
    public void setCelsius(int celsius) { this.celsius = celsius; }

    public String getSensor_id() { return sensor_id; }
    public void setSensor_id(String sensor_id) { this.sensor_id = sensor_id; }
}
