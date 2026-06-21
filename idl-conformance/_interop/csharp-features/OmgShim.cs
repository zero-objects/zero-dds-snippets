// Minimal Omg.Types container shim for the wire-level interop test.
// The XCDR2 wire logic lives entirely in ZeroDDS.Cdr; this only provides the
// data-record plumbing the generated combo.cs references. ExtensibilityKind /
// ExtensibilityAttribute deliberately come from ZeroDDS.Cdr (do NOT redefine
// them here or the generated [Extensibility(ExtensibilityKind.X)] is ambiguous).
namespace Omg.Types
{
    using System.Collections;
    using System.Collections.Generic;

    public interface ITopicType<T> { }
    public sealed class Any { public string? TypeId; public object? Value; }

    [System.AttributeUsage(System.AttributeTargets.Property)]
    public sealed class KeyAttribute : System.Attribute { }
    [System.AttributeUsage(System.AttributeTargets.Property)]
    public sealed class OptionalAttribute : System.Attribute { }
    [System.AttributeUsage(System.AttributeTargets.Property)]
    public sealed class MustUnderstandAttribute : System.Attribute { }
    [System.AttributeUsage(System.AttributeTargets.Property)]
    public sealed class IdAttribute : System.Attribute { public IdAttribute(uint id) { } }
    [System.AttributeUsage(System.AttributeTargets.Property)]
    public sealed class ExternalAttribute : System.Attribute { }
    [System.AttributeUsage(System.AttributeTargets.Class)]
    public sealed class NestedAttribute : System.Attribute { }
    [System.AttributeUsage(System.AttributeTargets.Class | System.AttributeTargets.Struct)]
    public sealed class ExtensibilityAttribute : System.Attribute
    {
        public ExtensibilityAttribute(ZeroDDS.Cdr.ExtensibilityKind kind) { }
    }

    public interface ISequence<T> : System.Collections.Generic.IList<T> { }
    public interface IBoundedSequence<T> : ISequence<T> { int Bound { get; } }

    public sealed class BoundedList<T> : IBoundedSequence<T>
    {
        private readonly List<T> _items;
        public BoundedList(int bound) { Bound = bound; _items = new List<T>(bound); }
        public int Bound { get; }
        public int Count => _items.Count;
        public bool IsReadOnly => false;
        public T this[int i] { get => _items[i]; set => _items[i] = value; }
        public void Add(T x) { if (_items.Count >= Bound) throw new System.ArgumentOutOfRangeException(nameof(x)); _items.Add(x); }
        public void Insert(int i, T x) { if (_items.Count >= Bound) throw new System.ArgumentOutOfRangeException(nameof(x)); _items.Insert(i, x); }
        public void Clear() => _items.Clear();
        public bool Contains(T x) => _items.Contains(x);
        public void CopyTo(T[] a, int i) => _items.CopyTo(a, i);
        public IEnumerator<T> GetEnumerator() => _items.GetEnumerator();
        public int IndexOf(T x) => _items.IndexOf(x);
        public bool Remove(T x) => _items.Remove(x);
        public void RemoveAt(int i) => _items.RemoveAt(i);
        IEnumerator IEnumerable.GetEnumerator() => _items.GetEnumerator();
    }

    public sealed class SequenceList<T> : ISequence<T>
    {
        private readonly List<T> _items = new();
        public SequenceList() { }
        public SequenceList(IEnumerable<T> items) { _items.AddRange(items); }
        public int Count => _items.Count;
        public bool IsReadOnly => false;
        public T this[int i] { get => _items[i]; set => _items[i] = value; }
        public void Add(T x) => _items.Add(x);
        public void Insert(int i, T x) => _items.Insert(i, x);
        public void Clear() => _items.Clear();
        public bool Contains(T x) => _items.Contains(x);
        public void CopyTo(T[] a, int i) => _items.CopyTo(a, i);
        public IEnumerator<T> GetEnumerator() => _items.GetEnumerator();
        public int IndexOf(T x) => _items.IndexOf(x);
        public bool Remove(T x) => _items.Remove(x);
        public void RemoveAt(int i) => _items.RemoveAt(i);
        IEnumerator IEnumerable.GetEnumerator() => _items.GetEnumerator();
    }
}
