#pragma once

// BindingsTypeMap.h
// Type mapping table: C++ type -> C# type + marshalling strategy.
// Extended with collection types, Variant, object references, and pass-by-reference rules.

#include "CodeGenerator_BindingsDataTypes.h"
#include <string>

namespace SE::BuildTool
{
    // A contiguous collection as represented by the managed/native ABI. Both
    // native containers (List/Span/etc.) and C-style fixed arrays are exposed
    // as managed T[]. The only ABI distinction is where the length comes from.
    enum class CollectionAbiKind
    {
        None,
        Variable,
        Fixed,
    };

    struct CollectionAbiInfo
    {
        CollectionAbiKind kind = CollectionAbiKind::None;
        std::string       elementCppType;
        int               fixedElementCount = 0;

        bool IsCollection() const { return kind != CollectionAbiKind::None; }
        bool HasRuntimeCount() const { return kind == CollectionAbiKind::Variable; }
    };

    // -------------------------------------------------------------------------
    // Core lookup functions
    // -------------------------------------------------------------------------

    /// Returns the TypeMapping for a known C++ type, or nullptr.
    const TypeMapping* FindTypeMapping(const char* cppType);

    /// True when a C++ type is one of the supported string types. Unlike a
    /// direct mapping lookup this also accepts qualified names (SE::String).
    bool IsStringType(const std::string& cppType);

    /// True if the C++ type is a pointer to a registered ScriptingObject-derived API type.
    bool IsScriptingObjectPointer(const std::string& cppType);

    /// Strips trailing '*', '&', 'const ' from a C++ type name.
    std::string StripTypeQualifiers(const std::string& cppType);

    /// Checks whether a property's getter return type and setter value type can
    /// share one managed property. Exact type matches are accepted, together
    /// with the explicit String/StringView and Array<T>/Span<T> bridges.
    /// Top-level const and reference qualifiers are intentionally ignored.
    bool ArePropertyAccessorTypesCompatible(const std::string& getterType,
                                            const std::string& setterType);

    /// Clears generation-time API type aliases.
    void ClearApiTypeNameAliases();

    /// Registers a C++ type name to C# public API name mapping.
    void RegisterApiTypeNameAlias(const std::string& nativeName,
                                  const std::string& nativeFullName,
                                  const std::string& publicName,
                                  const std::string& publicFullName);

    /// Registers a native API type that derives from ScriptingObject.
    void RegisterApiScriptingObjectType(const std::string& nativeName,
                                        const std::string& nativeFullName);

    /// Registers a non-ScriptingObject API class that is exposed as an owning native handle wrapper.
    void RegisterApiNativeObjectType(const std::string& nativeName,
                                     const std::string& nativeFullName);

    /// Registers a non-blittable API struct that needs an explicit generated ABI
    /// representation (for example, a struct that contains String fields).
    void RegisterApiInteropStructType(const std::string& nativeName,
                                      const std::string& nativeFullName,
                                      const std::string& publicName,
                                      const std::string& publicFullName);

    /// True if the type is represented by a generated BindingsInterop struct on
    /// the native boundary instead of its native C++ layout.
    bool IsApiInteropStructType(const std::string& cppType);

    /// Returns the fully qualified C++ BindingsInterop type name for a registered
    /// API struct. Returns an empty string when no special ABI type is required.
    std::string GetApiInteropStructCppType(const std::string& cppType);

    /// Returns the fully qualified C# marshaller type for a registered API struct.
    /// Returns an empty string when no special marshaller is required.
    std::string GetApiInteropStructMarshallerType(const std::string& cppType);

    // -------------------------------------------------------------------------
    // C# type resolution
    // -------------------------------------------------------------------------

    /// Returns the C# interop type for a P/Invoke parameter.
    /// For strings: "string"; for ScriptingObject*: "IntPtr"; otherwise the mapped C# type.
    std::string GetCSharpInteropType(const std::string& cppType);
    std::string GetCSharpInteropType(const std::string& cppType, int fixedArraySize);

    /// Returns the C# public-facing type (what the user sees in the API).
    std::string GetCSharpPublicType(const std::string& cppType);
    std::string GetCSharpPublicType(const std::string& cppType, int fixedArraySize);

    /// Returns the C# expression to convert from interop type to public type.
    /// e.g. for string: "{0}" (StringMarshalling handles it); for scripting object: "({Type})SE.Interop.ManagedHandleMarshaller.NativeToManaged.ConvertToManaged({0})"
    std::string GetCSharpFromInterop(const std::string& cppType, const std::string& varName);

    /// Returns the C# expression to convert from public type to interop type.
    /// e.g. for string: "{0}"; for ScriptingObject: "Object.GetUnmanagedPtr({0})"
    std::string GetCSharpToInterop(const std::string& cppType, const std::string& varName);

    // -------------------------------------------------------------------------
    // Pass-by-reference and type classification
    // -------------------------------------------------------------------------

    /// True if the C# side should pass this type by reference (struct types, math types, etc.).
    bool UsePassByReference(const std::string& cppType);

    /// True if the C++ type is a blittable POD type (can be copied directly across the interop boundary).
    bool IsPodType(const std::string& cppType);

    /// True if the C++ type is a known ScriptingObject-derived type (not just any pointer).
    bool IsScriptingObjectType(const std::string& cppType);

    /// True if the type is an object reference type (ScriptingObjectReference, AssetReference, etc.).
    bool IsObjectTypeRef(const std::string& cppType);

    /// True if the type is a collection type (Array, Span, List, Dictionary, HashSet, etc.).
    bool IsCollectionType(const std::string& cppType);

    /// Returns the ABI description for contiguous collection types. Pass a
    /// positive fixedArraySize for a C-style field whose extent is stored out
    /// of band in ApiField.
    CollectionAbiInfo GetCollectionAbiInfo(const std::string& cppType, int fixedArraySize = 0);

    /// Returns a fully-qualified managed type name suitable for native runtime
    /// lookup (for example, SE.Sprite or System.Int32).
    std::string GetCSharpFullTypeName(const std::string& cppType);

    // -------------------------------------------------------------------------
    // Marshal attribute generation
    // -------------------------------------------------------------------------

    /// Returns the C# marshal attribute for a P/Invoke parameter.
    /// e.g. "[MarshalAs(UnmanagedType.U1)]" for bool, "[MarshalUsing(typeof(ArrayMarshaller<,>))]" for arrays.
    std::string GetCSharpParamMarshalAttribute(const std::string& cppType, const std::string& paramName,
                                               int fixedArraySize = 0);

    /// Returns the C# marshal attribute for a P/Invoke return value.
    /// e.g. "[return: MarshalAs(UnmanagedType.U1)]" for bool.
    std::string GetCSharpReturnMarshalAttribute(const std::string& cppType, int fixedArraySize = 0);

    // -------------------------------------------------------------------------
    // CppTypeInfo - parsed C++ type for precise analysis
    // -------------------------------------------------------------------------

    /// Parsed representation of a C++ type with all qualifiers and generic arguments.
    struct CppTypeInfo
    {
        std::string baseType;       // stripped base type name
        bool       isConst;
        bool       isRef;
        bool       isMoveRef;
        bool       isPointer;
        bool       isArray;
        int         arraySize;
        std::vector<std::string> genericArgs; // template arguments

        CppTypeInfo()
            : isConst(false), isRef(false), isMoveRef(false)
            , isPointer(false), isArray(false), arraySize(0) {}

        /// Parse a C++ type string into this structure.
        void Parse(const std::string& cppType);

        /// Reconstruct the full C++ type string.
        std::string ToString() const;
    };

} // namespace SE::BuildTool
