// SPDX-License-Identifier: Apache-2.0
// Minimal Omg.Types marker shim for this demo.
//
// `idlc csharp` output references `Omg.Types.ITopicType<T>` (an empty marker
// the data record implements). The full Omg.Types runtime ALSO declares its
// own `ExtensibilityKind`, which collides with `ZeroDDS.Cdr.ExtensibilityKind`
// that the generated TypeSupport returns. This demo provides only the marker
// the generated Temperature.cs actually needs, so the verbatim codegen output
// compiles against the ZeroDDS binding under test.

namespace Omg.Types
{
    /// <summary>Marker for IDL topic types (matches Omg.Types.ITopicType&lt;T&gt;).</summary>
    public interface ITopicType<T>
    {
    }
}
