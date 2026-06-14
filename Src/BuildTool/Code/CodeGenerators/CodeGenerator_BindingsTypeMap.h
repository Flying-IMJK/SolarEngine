#pragma once

// BindingsTypeMap.h
// Type mapping table: C++ type -> C# type + marshalling strategy.
// Extended with collection types, Variant, object references, and pass-by-reference rules.

#include "CodeGenerator_BindingsDataTypes.h"
#include <string>

namespace SE::ReflectTool
{
    // -------------------------------------------------------------------------
    // Core lookup functions
    // -------------------------------------------------------------------------

    /// Returns the TypeMapping for a known C++ type, or nullptr.
    const TypeMapping* FindTypeMapping(const char* cppType);

    /// True if the C++ type is a ScriptingObject-derived pointer
    /// (ends with '*' and is not a known primitive).
    bool IsScriptingObjectPointer(const StringAnsi& cppType);

    /// Strips trailing '*', '&', 'const ' from a C++ type name.
    StringAnsi StripTypeQualifiers(const StringAnsi& cppType);

    // -------------------------------------------------------------------------
    // C# type resolution
    // -------------------------------------------------------------------------

    /// Returns the C# interop type for a P/Invoke parameter.
    /// For strings: "string"; for ScriptingObject*: "IntPtr"; otherwise the mapped C# type.
    StringAnsi GetCSharpInteropType(const StringAnsi& cppType);

    /// Returns the C# public-facing type (what the user sees in the API).
    StringAnsi GetCSharpPublicType(const StringAnsi& cppType);

    /// Returns the C# expression to convert from interop type to public type.
    /// e.g. for string: "{0}" (StringMarshalling handles it); for object: "({Type})ScriptingObject.ToNative({0})"
    StringAnsi GetCSharpFromInterop(const StringAnsi& cppType, const StringAnsi& varName);

    /// Returns the C# expression to convert from public type to interop type.
    /// e.g. for string: "{0}"; for object: "{0} != null ? {0}.__unmanagedPtr : IntPtr.Zero"
    StringAnsi GetCSharpToInterop(const StringAnsi& cppType, const StringAnsi& varName);

    // -------------------------------------------------------------------------
    // Pass-by-reference and type classification
    // -------------------------------------------------------------------------

    /// True if the C# side should pass this type by reference (struct types, math types, etc.).
    bool UsePassByReference(const StringAnsi& cppType);

    /// True if the C++ type is a blittable POD type (can be copied directly across the interop boundary).
    bool IsPodType(const StringAnsi& cppType);

    /// True if the C++ type is a known ScriptingObject-derived type (not just any pointer).
    bool IsScriptingObjectType(const StringAnsi& cppType);

    /// True if the type is an object reference type (ScriptingObjectReference, AssetReference, etc.).
    bool IsObjectTypeRef(const StringAnsi& cppType);

    /// True if the type is a collection type (Array, Span, List, Dictionary, HashSet, etc.).
    bool IsCollectionType(const StringAnsi& cppType);

    // -------------------------------------------------------------------------
    // Marshal attribute generation
    // -------------------------------------------------------------------------

    /// Returns the C# marshal attribute for a P/Invoke parameter.
    /// e.g. "[MarshalAs(UnmanagedType.U1)]" for bool, "[MarshalUsing(typeof(ArrayMarshaller<,>))]" for arrays.
    StringAnsi GetCSharpParamMarshalAttribute(const StringAnsi& cppType, const StringAnsi& paramName);

    /// Returns the C# marshal attribute for a P/Invoke return value.
    /// e.g. "[return: MarshalAs(UnmanagedType.U1)]" for bool.
    StringAnsi GetCSharpReturnMarshalAttribute(const StringAnsi& cppType);

    // -------------------------------------------------------------------------
    // CppTypeInfo — parsed C++ type for precise analysis
    // -------------------------------------------------------------------------

    /// Parsed representation of a C++ type with all qualifiers and generic arguments.
    struct CppTypeInfo
    {
        StringAnsi baseType;       // stripped base type name
        bool       isConst;
        bool       isRef;
        bool       isMoveRef;
        bool       isPointer;
        bool       isArray;
        int         arraySize;
        List<StringAnsi> genericArgs; // template arguments

        CppTypeInfo()
            : isConst(false), isRef(false), isMoveRef(false)
            , isPointer(false), isArray(false), arraySize(0) {}

        /// Parse a C++ type string into this structure.
        void Parse(const StringAnsi& cppType);

        /// Reconstruct the full C++ type string.
        StringAnsi ToString() const;
    };

} // namespace SE::ReflectTool