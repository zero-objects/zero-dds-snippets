// Stands in for the idl-java-generated Pose (zerodds-idlc Robot.idl --java).
// A record gives the (String, double, double, double) canonical constructor the
// docs snippet relies on; ReflectionTypeSupport marshals it via its canonical ctor.
public record Pose(String id, double x, double y, double z) {}
