# 04: Validate Mixed Struct Binding Contract

**What to build:**

Validate a reflected structure containing a character value, an internal opaque pointer, and an object pointer as one binding contract, including field layout and generated getter/setter ABI plans.

**Blocked by:** 02: Implement Opaque Single-Level Pointer Resolution; 03: Preserve Object Pointer and Pointer Depth Policies

**Status:** resolved

- [x] A mixed reflected structure passes binding preflight when all fields use supported policies.
- [x] Character, opaque-pointer, and object-pointer fields each retain their distinct semantic kinds.
- [x] Field ABI value kinds and pointer pass modes are deterministic and match native layout expectations.
- [x] Generated read and write plans use direct `IntPtr` value copying for opaque fields.
- [x] Object fields continue using their existing managed-handle conversion and cleanup rules.
- [x] The ABI fingerprint records the opaque-pointer ABI kind and changes when the field contract changes.

## Comments

Implemented with a mixed-field validation fixture in `SEBuilderBindingsTests`.
