#pragma once

// BindingsTypeMap.h
// Type mapping table: C++ type -> C# type + marshalling strategy.
// Extended with collection types, Variant, object references, and pass-by-reference rules.

#include "CodeGenerator_BindingsDataTypes.h"
#include <string>

namespace SE::BuildTool
{
    // -------------------------------------------------------------------------
    // Core lookup functions
    // -------------------------------------------------------------------------

    /// Returns the TypeMapping for a known C++ type, or nullptr.
    const TypeMapping* FindTypeMapping(const char* cppType);

    /// True if the C++ type is a pointer to a registered ScriptingObject-derived API type.
    bool IsScriptingObjectPointer(const std::string& cppType);

    /// Strips trailing '*', '&', 'const ' from a C++ type name.
    std::string StripTypeQualifiers(const std::string& cppType);

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

    // -------------------------------------------------------------------------
    // C# type resolution
    // -------------------------------------------------------------------------

    /// Returns the C# interop type for a P/Invoke parameter.
    /// For strings: "string"; for ScriptingObject*: "IntPtr"; otherwise the mapped C# type.
    std::string GetCSharpInteropType(const std::string& cppType);

    /// Returns the C# public-facing type (what the user sees in the API).
    std::string GetCSharpPublicType(const std::string& cppType);

    /// Returns the C# expression to convert from interop type to public type.
    /// e.g. for string: "{0}" (StringMarshalling handles it); for scripting object: "({Type})SE.Interop.ManagedHandleMarshaller.NativeToManaged.ConvertToManaged({0})"
    std::string GetCSharpFromInterop(const std::string& cppType, const std::string& varName);

    /// Returns the C# expression to convert from public type to interop type.
    /// e.g. for string: "{0}"; for object: "{0} != null ? {0}.__unmanagedPtr : IntPtr.Zero"
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

    // -------------------------------------------------------------------------
    // Marshal attribute generation
    // -------------------------------------------------------------------------

    /// Returns the C# marshal attribute for a P/Invoke parameter.
    /// e.g. "[MarshalAs(UnmanagedType.U1)]" for bool, "[MarshalUsing(typeof(ArrayMarshaller<,>))]" for arrays.
    std::string GetCSharpParamMarshalAttribute(const std::string& cppType, const std::string& paramName);

    /// Returns the C# marshal attribute for a P/Invoke return value.
    /// e.g. "[return: MarshalAs(UnmanagedType.U1)]" for bool.
    std::string GetCSharpReturnMarshalAttribute(const std::string& cppType);

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
