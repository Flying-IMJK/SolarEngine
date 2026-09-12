# 05: Regenerate and Verify Render2D Font Bindings

**What to build:**

Regenerate the Render2D font bindings using the completed resolver policy so `FontCharacterEntry` is emitted successfully, with a managed character value and an opaque atlas-slot address, while preserving the existing native and managed representation of the surrounding Font API.

**Blocked by:** 04: Validate Mixed Struct Binding Contract

**Status:** claimed

- [ ] The exposed `FontCharacterEntry` marker names the correct reflected type before generation.
- [ ] Font binding generation completes without the unresolved character or raw-pointer diagnostics.
- [ ] The generated managed `Character` field is `char`.
- [ ] The generated managed `Slot` field is `IntPtr`.
- [ ] The native generated contract preserves the atlas-slot pointer value without introducing ownership behavior.
- [ ] The existing managed representation for `Font` and related fields is unchanged except for the required pointer/character fixes.
- [ ] Focused SEBuilder tests and the Render2D generation verification both pass.

## Comments

The focused resolver and SEBuilder builds pass. Full Render2D generation remains blocked by pre-existing missing Editor generated source and path-specific Builder filtering in the dirty worktree; no generated Font binding was rewritten by hand.
