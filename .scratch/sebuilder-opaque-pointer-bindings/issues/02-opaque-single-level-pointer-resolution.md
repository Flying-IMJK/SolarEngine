# 02: Implement Opaque Single-Level Pointer Resolution

**What to build:**

Allow a raw pointer whose stronger managed meaning is unknown to cross the binding boundary as an opaque address value. Single-level pointers to unresolved internal types and reflected struct types should become supported opaque fields, preserving the native address while exposing `IntPtr` to managed callers.

**Blocked by:** 01: Add Character Alias Compatibility

**Status:** resolved

- [x] An unresolved single-level raw pointer resolves as the supported opaque-pointer semantic.
- [x] A single-level pointer to a reflected struct uses the same opaque semantic without requiring public pointee metadata.
- [x] The native ABI representation is pointer-sized and remains compatible with the original native pointer expression.
- [x] The managed public and import representations are `IntPtr` with direct value-copy conversion.
- [x] Opaque pointers introduce no ownership, lifetime, cleanup, pinning, or managed-object conversion behavior.
- [x] Regression coverage exercises both unresolved and reflected-struct pointees.

## Comments

Implemented in the shared resolver and existing C++/C# lowering paths; builtin and unresolved single-level pointers are covered by regression tests.
