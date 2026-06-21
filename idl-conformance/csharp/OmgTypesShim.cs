// SPDX-License-Identifier: Apache-2.0
// Minimal Omg.Types shim for this conformance example.
//
// The `idlc --csharp` output references a small set of `Omg.Types` marker /
// collection types that are not (yet) shipped inside the ZeroDDS C# binding
// under test:
//
//   * ITopicType<T>          — empty marker the data record implements
//   * ISequence<T>           — read interface for unbounded IDL sequences
//   * IBoundedSequence<T>    — read interface for bounded IDL sequences
//   * SequenceList<T>        — concrete List-backed sequence the decoder builds
//
// This file provides exactly those four, just enough for the generated
// conformance_combo.cs to compile against the binding and round-trip over a
// real DCPS loopback. (The csharp-typed website example ships the same kind of
// marker shim for ITopicType<T>.)

using System;
using System.Collections;
using System.Collections.Generic;

namespace ZeroDDS.Cdr
{
    // The `idlc --csharp` output decorates generated records with these XTypes
    // annotation attributes. They are declarative metadata only (not consulted
    // by the XCDR2 codec path used in this round-trip), so empty marker
    // attributes are sufficient for the generated code to compile against the
    // binding under test.

    /// <summary>Marks an IDL <c>@key</c> member.</summary>
    [AttributeUsage(AttributeTargets.Property | AttributeTargets.Field)]
    public sealed class KeyAttribute : Attribute { }

    /// <summary>Marks an IDL <c>@optional</c> member.</summary>
    [AttributeUsage(AttributeTargets.Property | AttributeTargets.Field)]
    public sealed class OptionalAttribute : Attribute { }

    /// <summary>Carries the IDL extensibility kind of a generated type.</summary>
    [AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct)]
    public sealed class ExtensibilityAttribute : Attribute
    {
        public ExtensibilityKind Kind { get; }
        public ExtensibilityAttribute(ExtensibilityKind kind) => Kind = kind;
    }
}

namespace Omg.Types
{
    /// <summary>Marker for IDL topic types (matches Omg.Types.ITopicType&lt;T&gt;).</summary>
    public interface ITopicType<T>
    {
    }

    /// <summary>Read view of a bounded IDL <c>sequence&lt;T, N&gt;</c>.</summary>
    public interface IBoundedSequence<T> : IReadOnlyList<T>
    {
    }

    // NOTE: ISequence derives FROM IBoundedSequence (not the other way around)
    // so that the generated decoder — which always types its sequence temp as
    // ISequence<T> — can assign that value into a field declared as the more
    // specific IBoundedSequence<T> (bounded sequence<T,N> members). A single
    // concrete SequenceList<T> then satisfies both field shapes.

    /// <summary>Read view of an unbounded IDL <c>sequence&lt;T&gt;</c>.</summary>
    public interface ISequence<T> : IBoundedSequence<T>
    {
    }

    /// <summary>
    /// Concrete List-backed sequence the generated decoder instantiates.
    /// Implements both <see cref="ISequence{T}"/> and
    /// <see cref="IBoundedSequence{T}"/> so a single type satisfies the field
    /// declarations for bounded and unbounded sequences alike.
    /// </summary>
    public sealed class SequenceList<T> : ISequence<T>
    {
        private readonly List<T> _items = new();

        public SequenceList() { }
        public SequenceList(IEnumerable<T> items) => _items.AddRange(items);

        public void Add(T item) => _items.Add(item);

        public T this[int index] => _items[index];
        public int Count => _items.Count;

        public IEnumerator<T> GetEnumerator() => _items.GetEnumerator();
        IEnumerator IEnumerable.GetEnumerator() => _items.GetEnumerator();
    }
}
