// BindingsTypeMap.cpp
// Type mapping table implementation.
// Extended with collection types, Variant, object references, and pass-by-reference rules.

#include "CodeGenerator_BindingsTypeMap.h"
#include <cstring>
#include <cstdlib>

namespace SE::ReflectTool
{
    // -------------------------------------------------------------------------
    // Static type mapping table
    // -------------------------------------------------------------------------

    static const TypeMapping s_typeMappings[] =
    {
        // cppType          csType          csInterop       blittable  isString  isObject
        { "bool",           "bool",         "bool",         true,      false,    false },
        { "int8",           "sbyte",        "sbyte",        true,      false,    false },
        { "uint8",          "byte",         "byte",         true,      false,    false },
        { "int16",          "short",        "short",        true,      false,    false },
        { "uint16",         "ushort",       "ushort",       true,      false,    false },
        { "int32",          "int",          "int",          true,      false,    false },
        { "uint32",         "uint",         "uint",         true,      false,    false },
        { "int64",          "long",         "long",         true,      false,    false },
        { "uint64",         "ulong",        "ulong",        true,      false,    false },
        { "float",          "float",        "float",        true,      false,    false },
        { "double",         "double",       "double",       true,      false,    false },
        { "int",            "int",          "int",          true,      false,    false },
        // C++ char types
        { "char",           "sbyte",        "sbyte",        true,      false,    false },
        { "Char",           "char",         "char",         true,      false,    false },
        // String types (StringMarshalling handles strings automatically)
        { "StringView",     "string",       "string",       false,     true,     false },
        { "String",         "string",       "string",       false,     true,     false },
        { "StringAnsi",     "string",       "string",       false,     true,     false },
        { "StringAnsiView", "string",       "string",       false,     true,     false },
        // Guid (blittable 16-byte struct)
        { "SGUID",          "System.Guid",  "System.Guid",  true,      false,    false },
        { "Guid",           "System.Guid",  "System.Guid",  true,      false,    false },
        { "UID",            "System.Guid",  "System.Guid",  true,      false,    false },
        // Math types (blittable structs)
        { "Vector2",        "Vector2",      "Vector2",      true,      false,    false },
        { "Vector3",        "Vector3",      "Vector3",      true,      false,    false },
        { "Vector4",        "Vector4",      "Vector4",      true,      false,    false },
        { "Quaternion",     "Quaternion",   "Quaternion",   true,      false,    false },
        { "Matrix",         "Matrix",       "Matrix",       true,      false,    false },
        // Scripting types
        { "Variant",        "object",       "System.IntPtr",  false,   false,    false },
        { "VariantType",    "System.Type",  "System.IntPtr",  false,   false,    false },
        { "ScriptingTypeHandle", "System.Type", "System.IntPtr", false,  false,  false },
        { "CLRObject",      "object",       "System.IntPtr",  false,   false,    false },
        { "CLRClass",       "System.Type",  "System.IntPtr",  false,   false,    false },
        // Version
        { "Version",        "System.Version", "System.IntPtr", false,  false,   false },
        // Void
        { "void",           "void",         "void",         true,      false,    false },
        // Sentinel
        { nullptr, nullptr, nullptr, false, false, false }
    };

    // -------------------------------------------------------------------------
    // Types that should always be passed by reference in C#
    // -------------------------------------------------------------------------

    static const char* s_passByRefTypes[] =
    {
        "Vector2", "Vector3", "Vector4", "Quaternion", "Matrix",
        "Color", "Color32", "Float2", "Float3", "Float4",
        "Double2", "Double3", "Double4",
        "Int2", "Int3", "Int4",
        "BoundingBox", "BoundingSphere", "BoundingFrustum",
        "Ray", "Transform", "Transform3D",
        "Rectangle", "Margin",
        nullptr
    };

    // -------------------------------------------------------------------------
    // Object reference types
    // -------------------------------------------------------------------------

    static const char* s_objectRefTypes[] =
    {
        "ScriptingObjectReference",
        "AssetReference",
        "WeakAssetReference",
        "SoftAssetReference",
        "SoftObjectReference",
        nullptr
    };

    // -------------------------------------------------------------------------
    // Core lookup functions
    // -------------------------------------------------------------------------

    const TypeMapping* FindTypeMapping(const char* cppType)
    {
        if (!cppType)
            return nullptr;
        for (int i = 0; s_typeMappings[i].cppType != nullptr; ++i)
        {
            if (strcmp(s_typeMappings[i].cppType, cppType) == 0)
                return &s_typeMappings[i];
        }
        return nullptr;
    }

    bool IsScriptingObjectPointer(const StringAnsi& cppType)
    {
        if (cppType.IsEmpty())
            return false;
        const char* raw = cppType.Get();
        int len = (int)cppType.Length();
        if (raw[len - 1] != '*')
            return false;
        StringAnsi base = cppType.Substring(0, len - 1);
        base = StripTypeQualifiers(base);
        return FindTypeMapping(base.Get()) == nullptr;
    }

    StringAnsi StripTypeQualifiers(const StringAnsi& cppType)
    {
        StringAnsi result = cppType;
        if (result.StartsWith("const "))
            result = result.Substring(6);
        while (!result.IsEmpty())
        {
            char last = result.Get()[result.Length() - 1];
            if (last == '*' || last == '&' || last == ' ')
                result = result.Substring(0, (int)result.Length() - 1);
            else
                break;
        }
        return result;
    }

    // -------------------------------------------------------------------------
    // C# type resolution
    // -------------------------------------------------------------------------

    StringAnsi GetCSharpInteropType(const StringAnsi& cppType)
    {
        StringAnsi stripped = StripTypeQualifiers(cppType);

        // Check for known type mappings first
        const TypeMapping* mapping = FindTypeMapping(stripped.Get());
        if (mapping)
        {
            if (mapping->isString)
                return StringAnsi("string");
            return StringAnsi(mapping->csInterop);
        }

        // Object reference types → IntPtr
        if (IsObjectTypeRef(cppType))
            return StringAnsi("IntPtr");

        // ScriptingObject-derived pointer
        if (IsScriptingObjectPointer(cppType))
            return StringAnsi("IntPtr");

        // Collection types → use public type with special marshalling
        if (stripped.StartsWith("Array<") || stripped.StartsWith("Span<")
            || stripped.StartsWith("List<") || stripped.StartsWith("DataContainer<")
            || stripped.StartsWith("BytesContainer"))
        {
            // Element type resolution happens in marshal attributes
            return StringAnsi("IntPtr");
        }

        if (stripped.StartsWith("Dictionary<") || stripped.StartsWith("HashSet<"))
            return StringAnsi("IntPtr");

        // BitArray → bool[]
        if (stripped == "BitArray")
            return StringAnsi("IntPtr");

        // Unknown type — use IntPtr as fallback
        return StringAnsi("System.IntPtr");
    }

    StringAnsi GetCSharpPublicType(const StringAnsi& cppType)
    {
        StringAnsi stripped = StripTypeQualifiers(cppType);
        const TypeMapping* mapping = FindTypeMapping(stripped.Get());
        if (mapping)
            return StringAnsi(mapping->csType);

        // ScriptingObject-derived pointer: use the class name
        if (IsScriptingObjectPointer(cppType))
            return stripped;

        // Object reference types: resolve generic argument
        if (IsObjectTypeRef(cppType))
        {
            int ltPos = stripped.Find("<");
            int gtPos = stripped.Find(">");
            if (ltPos != INVALID_INDEX && gtPos != INVALID_INDEX && gtPos > ltPos)
            {
                StringAnsi innerType = stripped.Substring(ltPos + 1, gtPos - ltPos - 1);
                return StripTypeQualifiers(innerType);
            }
            return stripped;
        }

        // Collection types
        if (stripped.StartsWith("Array<") || stripped.StartsWith("Span<") || stripped.StartsWith("List<"))
        {
            int ltPos = stripped.Find("<");
            int gtPos = stripped.Find(">");
            if (ltPos != INVALID_INDEX && gtPos != INVALID_INDEX && gtPos > ltPos)
            {
                StringAnsi elementType = StripTypeQualifiers(stripped.Substring(ltPos + 1, gtPos - ltPos - 1));
                return GetCSharpPublicType(elementType) + "[]";
            }
            return stripped + "[]";
        }

        if (stripped.StartsWith("DataContainer<"))
        {
            int ltPos = stripped.Find("<");
            int gtPos = stripped.Find(">");
            if (ltPos != INVALID_INDEX && gtPos != INVALID_INDEX && gtPos > ltPos)
            {
                StringAnsi elementType = StripTypeQualifiers(stripped.Substring(ltPos + 1, gtPos - ltPos - 1));
                return GetCSharpPublicType(elementType) + "[]";
            }
            return stripped + "[]";
        }

        if (stripped == "BytesContainer")
            return StringAnsi("byte[]");

        if (stripped.StartsWith("Dictionary<"))
        {
            int ltPos = stripped.Find("<");
            int gtPos = stripped.Find(">");
            if (ltPos != INVALID_INDEX && gtPos != INVALID_INDEX && gtPos > ltPos)
            {
                StringAnsi inner = stripped.Substring(ltPos + 1, gtPos - ltPos - 1);
                // Split by comma for K,V
                int commaPos = inner.Find(",");
                if (commaPos != INVALID_INDEX)
                {
                    StringAnsi keyType = StripTypeQualifiers(inner.Substring(0, commaPos));
                    StringAnsi valType = StripTypeQualifiers(inner.Substring(commaPos + 1));
                    return StringAnsi("System.Collections.Generic.Dictionary<")
                         + GetCSharpPublicType(keyType) + ", " + GetCSharpPublicType(valType) + ">";
                }
            }
            return stripped;
        }

        if (stripped.StartsWith("HashSet<"))
        {
            int ltPos = stripped.Find("<");
            int gtPos = stripped.Find(">");
            if (ltPos != INVALID_INDEX && gtPos != INVALID_INDEX && gtPos > ltPos)
            {
                StringAnsi elementType = StripTypeQualifiers(stripped.Substring(ltPos + 1, gtPos - ltPos - 1));
                return StringAnsi("System.Collections.Generic.HashSet<") + GetCSharpPublicType(elementType) + ">";
            }
            return stripped;
        }

        if (stripped == "BitArray")
            return StringAnsi("bool[]");

        // Unknown — return as-is (replace :: with .)
        StringAnsi result = stripped;
        int pos;
        while ((pos = result.Find("::")) != INVALID_INDEX)
        {
            result = result.Substring(0, pos) + "." + result.Substring(pos + 2);
        }
        return result;
    }

    StringAnsi GetCSharpFromInterop(const StringAnsi& cppType, const StringAnsi& varName)
    {
        StringAnsi stripped = StripTypeQualifiers(cppType);
        const TypeMapping* mapping = FindTypeMapping(stripped.Get());
        if (mapping && mapping->isString)
            return varName; // StringMarshalling handles strings automatically

        if (IsScriptingObjectPointer(cppType))
        {
            StringAnsi className = stripped;
            return StringAnsi::Format("({0})ScriptingObject.ToNative({1})", className.Get(), varName.Get());
        }

        if (IsObjectTypeRef(cppType))
            return varName; // handled by marshal attributes

        return varName; // blittable or handled by marshalling
    }

    StringAnsi GetCSharpToInterop(const StringAnsi& cppType, const StringAnsi& varName)
    {
        StringAnsi stripped = StripTypeQualifiers(cppType);
        const TypeMapping* mapping = FindTypeMapping(stripped.Get());
        if (mapping && mapping->isString)
            return varName; // LibraryImport StringMarshalling handles strings

        if (IsScriptingObjectPointer(cppType))
            return StringAnsi::Format("{0} != null ? {0}.__unmanagedPtr : IntPtr.Zero", varName.Get());

        if (IsObjectTypeRef(cppType))
            return StringAnsi::Format("{0} != null ? {0}.__unmanagedPtr : IntPtr.Zero", varName.Get());

        return varName; // blittable
    }

    // -------------------------------------------------------------------------
    // Pass-by-reference and type classification
    // -------------------------------------------------------------------------

    bool UsePassByReference(const StringAnsi& cppType)
    {
        StringAnsi stripped = StripTypeQualifiers(cppType);

        // Pointers and objects: no
        if (stripped.IsEmpty())
            return false;
        if (IsScriptingObjectPointer(cppType))
            return false;
        if (IsObjectTypeRef(cppType))
            return false;

        // Strings and collections: no
        const TypeMapping* mapping = FindTypeMapping(stripped.Get());
        if (mapping && mapping->isString)
            return false;
        if (IsCollectionType(cppType))
            return false;

        // Variant/Type: no
        if (stripped == "Variant" || stripped == "VariantType" || stripped == "ScriptingTypeHandle")
            return false;

        // Explicit ref: yes
        if (cppType.Get()[cppType.Length() - 1] == '&')
            return true;

        // Known pass-by-ref types (math structs)
        for (int i = 0; s_passByRefTypes[i] != nullptr; ++i)
        {
            if (stripped == s_passByRefTypes[i])
                return true;
        }

        // Struct types that we can't identify as blittable: pass by ref
        // (This is a heuristic; the Reflector will need more complete type info
        //  to make this determination accurately. For now, we rely on the explicit list.)
        return false;
    }

    bool IsPodType(const StringAnsi& cppType)
    {
        StringAnsi stripped = StripTypeQualifiers(cppType);
        const TypeMapping* mapping = FindTypeMapping(stripped.Get());
        if (mapping)
            return mapping->isBlittable;
        // Non-mapped types default to non-POD
        return false;
    }

    bool IsScriptingObjectType(const StringAnsi& cppType)
    {
        StringAnsi stripped = StripTypeQualifiers(cppType);
        // Known scripting object base types
        if (stripped == "ScriptingObject" || stripped == "PersistentScriptingObject"
            || stripped == "ManagedObject" || stripped == "BinaryAsset"
            || stripped == "Asset" || stripped == "Actor" || stripped == "Component")
            return true;
        return IsScriptingObjectPointer(cppType);
    }

    bool IsObjectTypeRef(const StringAnsi& cppType)
    {
        StringAnsi stripped = StripTypeQualifiers(cppType);
        for (int i = 0; s_objectRefTypes[i] != nullptr; ++i)
        {
            if (stripped.StartsWith(s_objectRefTypes[i]))
                return true;
        }
        return false;
    }

    bool IsCollectionType(const StringAnsi& cppType)
    {
        StringAnsi stripped = StripTypeQualifiers(cppType);
        return stripped.StartsWith("Array<")
            || stripped.StartsWith("Span<")
            || stripped.StartsWith("List<")
            || stripped.StartsWith("Dictionary<")
            || stripped.StartsWith("HashSet<")
            || stripped.StartsWith("DataContainer<")
            || stripped == "BytesContainer"
            || stripped == "BitArray";
    }

    // -------------------------------------------------------------------------
    // Marshal attribute generation
    // -------------------------------------------------------------------------

    StringAnsi GetCSharpParamMarshalAttribute(const StringAnsi& cppType, const StringAnsi& paramName)
    {
        StringAnsi stripped = StripTypeQualifiers(cppType);

        // bool → U1
        if (stripped == "bool")
            return StringAnsi("[MarshalAs(UnmanagedType.U1)]");

        // char → I2
        if (stripped == "Char")
            return StringAnsi("[MarshalAs(UnmanagedType.I2)]");

        // Variant/object → ManagedHandleMarshaller
        if (stripped == "Variant" || stripped == "object")
            return StringAnsi("[MarshalUsing(typeof(ManagedHandleMarshaller))]");

        // System.Type
        if (stripped == "VariantType" || stripped == "ScriptingTypeHandle")
            return StringAnsi("[MarshalUsing(typeof(SystemTypeMarshaller))]");

        // Array/Span/List/DataContainer
        if (stripped.StartsWith("Array<") || stripped.StartsWith("Span<")
            || stripped.StartsWith("List<") || stripped.StartsWith("DataContainer<"))
        {
            StringAnsi countName = StringAnsi::Format("__{0}Count", paramName.Get());
            return StringAnsi::Format("[MarshalUsing(typeof(ArrayMarshaller<,>), CountElementName = \"{0}\")]", countName.Get());
        }

        // BytesContainer
        if (stripped == "BytesContainer")
            return StringAnsi("[MarshalUsing(typeof(ArrayMarshaller<,>), CountElementName = \"__count\"])]");

        // Dictionary
        if (stripped.StartsWith("Dictionary<"))
            return StringAnsi("[MarshalUsing(typeof(DictionaryMarshaller<,>), ConstantElementCount = 0)]");

        // HashSet
        if (stripped.StartsWith("HashSet<"))
            return StringAnsi("[MarshalUsing(typeof(HashSetMarshaller<,>), ConstantElementCount = 0)]");

        // Object reference types
        if (IsObjectTypeRef(cppType))
            return StringAnsi("[MarshalUsing(typeof(ManagedHandleMarshaller))]");

        // ScriptingObject pointer
        if (IsScriptingObjectPointer(cppType))
            return StringAnsi(); // IntPtr, no attribute needed

        return StringAnsi(); // no attribute for blittable types
    }

    StringAnsi GetCSharpReturnMarshalAttribute(const StringAnsi& cppType)
    {
        StringAnsi stripped = StripTypeQualifiers(cppType);

        // bool → U1
        if (stripped == "bool")
            return StringAnsi("[return: MarshalAs(UnmanagedType.U1)]");

        // Variant/object
        if (stripped == "Variant" || stripped == "object")
            return StringAnsi("[return: MarshalUsing(typeof(ManagedHandleMarshaller))]");

        // System.Type
        if (stripped == "VariantType" || stripped == "ScriptingTypeHandle")
            return StringAnsi("[return: MarshalUsing(typeof(SystemTypeMarshaller))]");

        // Array return
        if (stripped.StartsWith("Array<") || stripped.StartsWith("Span<")
            || stripped.StartsWith("List<") || stripped.StartsWith("DataContainer<")
            || stripped == "BytesContainer")
        {
            return StringAnsi("[return: MarshalUsing(typeof(ArrayMarshaller<,>), CountElementName = \"__returnCount\")]");
        }

        // Dictionary return
        if (stripped.StartsWith("Dictionary<"))
            return StringAnsi("[return: MarshalUsing(typeof(DictionaryMarshaller<,>), ConstantElementCount = 0)]");

        // Object reference types
        if (IsObjectTypeRef(cppType))
            return StringAnsi("[return: MarshalUsing(typeof(ManagedHandleMarshaller))]");

        return StringAnsi(); // no attribute for blittable types
    }

    // -------------------------------------------------------------------------
    // CppTypeInfo implementation
    // -------------------------------------------------------------------------

    void CppTypeInfo::Parse(const StringAnsi& cppType)
    {
        *this = CppTypeInfo(); // reset

        if (cppType.IsEmpty())
            return;

        StringAnsi remaining = cppType;

        // Strip leading "const "
        if (remaining.StartsWith("const "))
        {
            isConst = true;
            remaining = remaining.Substring(6);
        }

        // Strip trailing '&&' (move ref)
        if (remaining.Length() >= 2
            && remaining.Get()[remaining.Length() - 1] == '&'
            && remaining.Get()[remaining.Length() - 2] == '&')
        {
            isMoveRef = true;
            remaining = remaining.Substring(0, (int)remaining.Length() - 2);
        }
        // Strip trailing '&' (ref)
        else if (remaining.Length() >= 1 && remaining.Get()[remaining.Length() - 1] == '&')
        {
            isRef = true;
            remaining = remaining.Substring(0, (int)remaining.Length() - 1);
        }

        // Strip trailing '*' (pointer)
        while (!remaining.IsEmpty() && remaining.Get()[remaining.Length() - 1] == '*')
        {
            isPointer = true;
            remaining = remaining.Substring(0, (int)remaining.Length() - 1);
            remaining = StripTypeQualifiers(remaining); // strip spaces
        }

        // Check for fixed array [N]
        int lbPos = remaining.Find("[");
        int rbPos = remaining.Find("]");
        if (lbPos != INVALID_INDEX && rbPos != INVALID_INDEX && rbPos > lbPos)
        {
            isArray = true;
            StringAnsi sizeStr = remaining.Substring(lbPos + 1, rbPos - lbPos - 1);
            arraySize = atoi(sizeStr.Get());
            remaining = remaining.Substring(0, lbPos);
        }

        // Extract base type and generic arguments
        int ltPos = remaining.Find("<");
        if (ltPos != INVALID_INDEX)
        {
            baseType = StripTypeQualifiers(remaining.Substring(0, ltPos));
            // Extract generic arguments (simple, non-nested)
            int depth = 0;
            int start = ltPos + 1;
            const char* s = remaining.Get();
            int len = (int)remaining.Length();
            for (int i = ltPos + 1; i < len; ++i)
            {
                if (s[i] == '<') ++depth;
                else if (s[i] == '>') --depth;
                else if (s[i] == ',' && depth == 0)
                {
                    StringAnsi arg = StripTypeQualifiers(remaining.Substring(start, i - start));
                    genericArgs.Add(arg);
                    start = i + 1;
                }
                if (depth < 0) break;
            }
            // Last argument
            if (start < len - 1)
            {
                StringAnsi lastArg = StripTypeQualifiers(remaining.Substring(start, len - 1 - start));
                if (!lastArg.IsEmpty())
                    genericArgs.Add(lastArg);
            }
        }
        else
        {
            baseType = StripTypeQualifiers(remaining);
        }
    }

    StringAnsi CppTypeInfo::ToString() const
    {
        StringAnsi result;
        if (isConst)
            result += "const ";
        result += baseType;
        if (genericArgs.Count() > 0)
        {
            result += "<";
            for (int i = 0; i < genericArgs.Count(); ++i)
            {
                if (i > 0) result += ", ";
                result += genericArgs[i];
            }
            result += ">";
        }
        if (isPointer)
            result += "*";
        if (isRef)
            result += "&";
        if (isMoveRef)
            result += "&&";
        if (isArray)
            result += StringAnsi::Format("[{0}]", arraySize);
        return result;
    }

} // namespace SE::ReflectTool