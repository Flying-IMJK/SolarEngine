# 03: Preserve Object Pointer and Pointer Depth Policies

**What to build:**

Ensure the new opaque-pointer fallback is narrowly scoped: existing object-pointer and reference-wrapper semantics remain authoritative, while unsupported pointer depth and unresolved non-pointer values continue to produce their existing diagnostics.

**Blocked by:** 02: Implement Opaque Single-Level Pointer Resolution

**Status:** resolved

- [x] Scripting-object pointers retain their existing object classification, managed type, and handle conversion.
- [x] Strong, weak, soft, and explicitly marshalled reference wrappers retain their dedicated policies.
- [x] Multi-level pointers remain unsupported with the existing pointer-depth diagnostic.
- [x] Function pointers and other unsupported pointer forms are not silently converted to opaque addresses.
- [x] Unresolved non-pointer values remain unsupported with the missing-declaration diagnostic.
- [x] Regression tests prove opaque fallback does not override any stronger pointer policy.

## Comments

Implemented and covered by focused resolver policy tests.
