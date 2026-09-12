Status: ready-for-agent

# SEBuilder Opaque Pointer Bindings

## Problem Statement

When SEBuilder reflects `FontCharacterEntry`, it rejects `SE::Char` and `const FontTextureAtlasSlot*` during binding preflight. Flax.Build accepts the same native shape: `Char` is emitted as C# `char`, while a raw pointer to an internal, non-reflected type is emitted as C# `IntPtr` and copied without ownership management.

The current SEBuilder model already contains an `OpaquePointer` lowering, but its resolver does not select that lowering for unresolved single-level pointers or reflected struct pointers. As a result, valid internal pointer fields cannot reach the existing C++ and C# lowering paths, and generation stops before emitting bindings.

## Solution

Extend the shared SEBuilder binding type resolver so that a single-level raw pointer with no managed ownership contract is classified as an opaque pointer. The native side keeps the original pointer value and type for wrapper expressions, the ABI side uses a pointer-sized opaque slot, and the managed side uses `IntPtr`.

Keep object pointers and explicit object-reference wrappers ahead of the opaque fallback so they retain their existing lifetime and handle semantics. Continue rejecting multi-level pointers and unsupported reference forms. Normalize both qualified and unqualified `Char` spellings to the managed C# `char` ABI type.

The change must be validated through one shared binding-map regression seam that exercises the resolver and both language conversion descriptions for representative fields. The seam must prove that the generated contract is equivalent to Flax.Build for this case: value character, opaque pointer field, managed object pointer, and rejected multi-level pointer.

## User Stories

1. As a SEBuilder user, I want `SE::Char` fields to resolve as C# `char`, so that reflected UTF-16 character values do not fail preflight.
2. As a SEBuilder user, I want unqualified `Char` to remain supported, so that existing headers using the legacy spelling do not regress.
3. As a SEBuilder user, I want a pointer to an unreflected internal type to bind as `IntPtr`, so that implementation details can cross the ABI without requiring public type metadata.
4. As a SEBuilder user, I want a pointer to an unreflected struct in a reflected struct field to preserve its address value, so that native caches and handles can be passed through safely as opaque data.
5. As a SEBuilder user, I want opaque pointer fields to have pointer-sized ABI storage, so that generated C# sequential layouts match native layouts on supported platforms.
6. As a SEBuilder user, I want opaque pointer fields to have no implicit ownership, so that the generator does not invent destruction, pinning, or managed-handle behavior for internal addresses.
7. As a SEBuilder user, I want opaque pointer fields to be directly readable and writable as `IntPtr`, so that generated marshalling is a value copy with no hidden conversion.
8. As a SEBuilder user, I want native wrappers to retain the original pointee type when invoking native code, so that generated C++ remains type-correct without requiring a reflected declaration for the pointee.
9. As a SEBuilder user, I want scripting-object pointers to keep their managed object conversion, so that opaque fallback does not break object lifetime or identity semantics.
10. As a SEBuilder user, I want strong object-reference wrappers to keep their existing CLR handle conversion, so that ownership and reference policy remain explicit.
11. As a SEBuilder user, I want explicitly marshalled pointer types to use their declared marshal policy, so that opaque fallback does not override an intentional ABI contract.
12. As a SEBuilder user, I want multi-level pointers to remain rejected unless a dedicated policy exists, so that unsupported pointer-to-pointer ABI shapes are not silently emitted.
13. As a SEBuilder user, I want invalid non-pointer value types to continue failing declaration resolution, so that opaque fallback does not hide missing reflection metadata.
14. As a SEBuilder user, I want a reflected struct containing `Char`, an internal pointer, and an object pointer to pass preflight together, so that mixed layouts are validated as one contract.
15. As a SEBuilder user, I want generated C# interop layouts to use `IntPtr` for opaque pointer members, so that the result is usable without unsafe typed-pointer syntax.
16. As a SEBuilder user, I want generated C++ interop layouts to use pointer-compatible storage while preserving native conversion expressions, so that native and managed representations agree.
17. As a SEBuilder maintainer, I want the resolver to expose opaque-pointer semantics through the existing shared model, so that C++ and C# generators do not each implement separate pointer exceptions.
18. As a SEBuilder maintainer, I want ABI fingerprints to include the resulting opaque pointer ABI kind, so that changing a field from rejected to opaque is observable in generated-contract compatibility checks.
19. As a SEBuilder maintainer, I want diagnostics for unsupported pointer depth to remain stable, so that callers and tests can distinguish policy failures from missing declarations.
20. As a SEBuilder maintainer, I want the regression seam to cover both qualified and unqualified character aliases, so that future Clang spelling changes are detected early.
21. As a SEBuilder maintainer, I want the regression seam to verify C++ export type, managed public type, managed import type, and conversion strategy, so that a resolver-only green test cannot mask emitter incompatibility.
22. As a SEBuilder maintainer, I want the implementation to reuse the existing `OpaquePointer` conversion paths, so that the change remains localized to type classification and focused tests.
23. As a SEBuilder maintainer, I want no generated-file churn to be required for the resolver change itself, so that source-of-truth behavior is tested before regeneration.
24. As a SEBuilder user, I want `FontCharacterEntry` to generate successfully after the policy change, so that the existing font cache API can be reflected without editing the internal atlas slot type.

## Implementation Decisions

- The shared binding type resolver is the source of truth for pointer classification. Language-specific generators consume its `BindingTypeKind` and conversion records.
- A single-level raw pointer whose pointee declaration cannot be resolved is classified as `OpaquePointer`, provided no explicit marshal policy, object-reference family, or unsupported family has already claimed the type.
- A single-level pointer to a reflected struct is also classified as `OpaquePointer` by default. It is not emitted as a C# unsafe typed pointer unless a future explicit typed-pointer policy is added.
- A single-level pointer to a reflected scripting object or native class retains the existing object classification and managed-handle behavior.
- Strong, weak, and soft reference wrappers retain their existing dedicated policies. The opaque fallback applies only to raw pointer syntax.
- Pointer depth greater than one continues to produce the existing unsupported-pointer diagnostic.
- Non-pointer types still require either a builtin mapping or a registered declaration; they do not fall back to opaque storage.
- `OpaquePointer` uses a pointer-sized native ABI slot and maps to C# `IntPtr`. It has direct conversion semantics and no ownership, lifetime, or cleanup operation.
- Native conversion metadata retains the source native pointer spelling, allowing generated C++ to cast the opaque ABI value back to the declared native pointer type at call and field boundaries.
- The C# struct marshaller treats opaque pointer fields exactly as direct `IntPtr` fields in both directions. It must not generate object handles or release logic for them.
- The builtin mapping recognizes both `Char` and `SE::Char` and maps them to C# `char` with the existing character ABI kind.
- The `FontCharacterEntry` reflection marker typo is corrected independently when that type remains exposed: its minimal scripting type declaration must name `FontCharacterEntry`, not `FontLineCache`.
- The implementation does not register `FontTextureAtlasSlot` as a public reflected type. Its internal pointer remains opaque.
- The change is intentionally generic and must not add a Font-specific name check.
- The single highest test seam is the binding type-map contract test. It constructs a minimal reflected owner with representative `TypeInfo` values, resolves shared semantics, then checks both C++ and C# conversion records and the function ABI plan. Generator snapshot tests are not required for the first implementation unless the existing test harness already supports them.

## Testing Decisions

- Tests assert externally visible binding contracts: semantic kind, diagnostic code, C++ export type, C# public type, C# library-import type, conversion strategy, ABI value kind, and pointer pass mode.
- Add a qualified-character case for `SE::Char` and an unqualified alias case for `Char`; both must resolve as blittable C# `char` values.
- Add an unresolved single-level pointer case such as `const InternalSlot*`; it must resolve as supported `OpaquePointer`, lower to native `void*` ABI storage, and lower to managed `IntPtr`.
- Add a reflected struct pointer case; it must use the same opaque contract and must not require the pointee to be public or POD.
- Add a reflected scripting-object pointer case; it must remain an object classification with its existing managed type and handle strategy.
- Add a strong object-reference wrapper case; it must remain a CLR object classification.
- Add a two-level pointer case; it must remain unsupported with the multi-level-pointer diagnostic.
- Add an unresolved non-pointer value case; it must remain unsupported with the missing-declaration diagnostic.
- Add a mixed reflected-struct owner case containing character, opaque pointer, and object pointer fields. Its field validation and generated getter/setter ABI plans must be supported where the existing object policy allows them.
- Verify the ABI fingerprint is deterministic and changes when a pointer changes from unsupported to opaque ABI classification.
- Run the existing SEBuilder binding-map test executable and the focused regression test. Re-run the actual SEBuilder generation for the Render2D module only after the source tests pass, then inspect the generated font binding for `char Character`, `IntPtr Slot`, and the existing managed representation for `Font`.

## Out of Scope

- Designing a general unsafe typed-pointer API for C#.
- Adding ownership, lifetime tracking, pinning, or validation for opaque pointers.
- Registering or exposing `FontTextureAtlasSlot` as a reflected public type.
- Converting opaque pointers into managed engine objects.
- Supporting pointer-to-pointer, function-pointer, or arbitrary multi-level pointer policies.
- Changing collection, string, variant, interface, or object-reference ABI policies.
- Rewriting generated files by hand.
- Removing reflection from `FontLineCache` or `FontCharacterEntry`; this specification assumes they remain intentionally exposed.

## Further Notes

Flax.Build resolves API metadata for known types but deliberately treats raw pointers as address values when no stronger semantic exists. Its generated `FontCharacterEntry` confirms that the intended public representation is `IntPtr Slot`, while native conversion simply copies the `FontTextureAtlasSlot*` value. SEBuilder already has the required opaque lowering stages; the missing behavior is the resolver fallback and its regression coverage.

The implementation should be reviewed for the interaction between struct field layout and generated getter/setter wrappers. A field-level opaque pointer is safe as a copied address, but it must not be treated as a managed reference or released by marshaller cleanup.
