// SPDX-License-Identifier: Apache-2.0
// Minimal Omg.Types / ZeroDDS.Cdr shim for the QoS-conformance harness.
//
// The `idlc --csharp` output for reading.idl references two support symbols
// the ZeroDDS binding under test does not ship:
//   * Omg.Types.ITopicType<T>  — empty marker the generated record implements
//   * ZeroDDS.Cdr.[Key]        — declarative @key attribute marker
// These are declarative-only (the XCDR2 codec path does not consult them), so
// empty markers suffice for the generated code to compile against the binding.
// Same pattern as the sibling idl-conformance/csharp example.

using System;

namespace ZeroDDS.Cdr
{
    /// <summary>Marks an IDL <c>@key</c> member.</summary>
    [AttributeUsage(AttributeTargets.Property | AttributeTargets.Field)]
    public sealed class KeyAttribute : Attribute { }
}

namespace Omg.Types
{
    /// <summary>Marker for IDL topic types (matches Omg.Types.ITopicType&lt;T&gt;).</summary>
    public interface ITopicType<T>
    {
    }
}
