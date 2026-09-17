# 01: Add Character Alias Compatibility

**What to build:**

Make both qualified and unqualified engine character aliases resolve to the existing C# `char` binding contract, so reflected character fields pass binding preflight and preserve the current character ABI semantics.

**Blocked by:** None (can start immediately)

**Status:** resolved

- [x] Qualified `SE::Char` resolves as a supported character value with managed type `char`.
- [x] Unqualified `Char` continues to resolve as the same supported character value.
- [x] Character values remain blittable and use the existing ABI conversion strategy.
- [x] Regression coverage distinguishes qualified and unqualified spellings.

## Comments

Implemented in the shared binding type map and covered by `SEBuilderBindingsTests`.
