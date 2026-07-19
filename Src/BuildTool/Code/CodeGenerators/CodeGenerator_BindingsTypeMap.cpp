// BindingsTypeMap.cpp
// Type mapping table implementation.
// Extended with collection types, Variant, object references, and pass-by-reference rules.

#include "CodeGenerator_BindingsTypeMap.h"
#include <cstring>
#include <cstdlib>
#include <vector>

namespace SE::BuildTool
{
    struct ApiTypeNameAlias
    {
        std::string nativeName;
        std::string nativeFullName;
        std::string publicName;
        std::string publicFullName;
    };

    static std::vector<ApiTypeNameAlias> s_apiTypeNameAliases;
    static std::vector<std::string> s_scriptingObjectTypeNames;
    static std::vector<std::string> s_nativeObjectTypeNames;

    static std::string StripCppKeywordPrefixes(const std::string& cppType)
    {
        std::string result = cppType;
        Utils::String::TrimStart(result);
        Utils::String::TrimEnd(result);

        while (Utils::String::StartsWith(result, "::"))
            result = result.substr(2);
        if (Utils::String::StartsWith(result, "class "))
            result = result.substr(6);
        else if (Utils::String::StartsWith(result, "struct "))
            result = result.substr(7);
        else if (Utils::String::StartsWith(result, "enum "))
            result = result.substr(5);
        Utils::String::TrimStart(result);
        Utils::String::TrimEnd(result);
        return result;
    }

    static std::string GetUnqualifiedTypeName(const std::string& cppType)
    {
        std::string result = StripCppKeywordPrefixes(cppType);
        int pos = INVALID_INDEX;
        int searchStart = 0;
        while (true)
        {
            std::string tail = result.substr(searchStart);
            int found = Utils::String::Find(tail, "::");
            if (found == INVALID_INDEX)
                break;
            pos = searchStart + found;
            searchStart = pos + 2;
        }
        if (pos != INVALID_INDEX)
            result = result.substr(pos + 2);
        Utils::String::TrimStart(result);
        Utils::String::TrimEnd(result);
        return result;
    }

    static std::string NormalizeCppNameForAlias(const std::string& cppType)
    {
        std::string result = StripCppKeywordPrefixes(cppType);
        Utils::String::ReplaceAll(result, " :: ", "::");
        Utils::String::ReplaceAll(result, ":: ", "::");
        Utils::String::ReplaceAll(result, " ::", "::");
        while (Utils::String::StartsWith(result, "::"))
            result = result.substr(2);
        Utils::String::TrimStart(result);
        Utils::String::TrimEnd(result);
        return result;
    }

    static std::string ToCSharpQualifiedName(const std::string& cppType)
    {
        std::string result = NormalizeCppNameForAlias(cppType);
        int pos;
        while ((pos = Utils::String::Find(result, "::")) != INVALID_INDEX)
        {
            result = result.substr(0, pos) + "." + result.substr(pos + 2);
        }
        return result;
    }

    static std::string GetCSharpSimpleName(const std::string& csType)
    {
        int separator = Utils::String::FindLast(csType, '.');
        return separator == INVALID_INDEX ? csType : csType.substr(separator + 1);
    }

    static std::string ResolveCSharpTypeNameAlias(const std::string& cppType)
    {
        std::string stripped = NormalizeCppNameForAlias(cppType);
        std::string unqualified = GetUnqualifiedTypeName(stripped);
        for (auto const& alias : s_apiTypeNameAliases)
        {
            if (!alias.nativeFullName.empty() && stripped == alias.nativeFullName)
                return alias.publicFullName;
        }
        for (auto const& alias : s_apiTypeNameAliases)
        {
            if (stripped == alias.nativeName || unqualified == alias.nativeName)
                return alias.publicName;
        }
        return ToCSharpQualifiedName(stripped);
    }

    static const TypeMapping* FindTypeMappingNormalized(const std::string& cppType)
    {
        std::string stripped = StripTypeQualifiers(cppType);
        stripped = StripCppKeywordPrefixes(stripped);

        const TypeMapping* mapping = FindTypeMapping(stripped.c_str());
        if (mapping)
            return mapping;

        std::string unqualified = GetUnqualifiedTypeName(stripped);
        return FindTypeMapping(unqualified.c_str());
    }

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

    void ClearApiTypeNameAliases()
    {
        s_apiTypeNameAliases.clear();
        s_scriptingObjectTypeNames.clear();
        s_nativeObjectTypeNames.clear();
    }

    void RegisterApiTypeNameAlias(const std::string& nativeName,
                                  const std::string& nativeFullName,
                                  const std::string& publicName,
                                  const std::string& publicFullName)
    {
        if (nativeName.empty() || publicName.empty())
        {
            return;
        }

        ApiTypeNameAlias alias;
        alias.nativeName = NormalizeCppNameForAlias(nativeName);
        alias.nativeFullName = NormalizeCppNameForAlias(nativeFullName.empty() ? nativeName : nativeFullName);
        alias.publicName = publicName;
        alias.publicFullName = publicFullName.empty() ? publicName : publicFullName;
        s_apiTypeNameAliases.push_back(alias);
    }

    void RegisterApiScriptingObjectType(const std::string& nativeName,
                                        const std::string& nativeFullName)
    {
        if (!nativeName.empty())
            s_scriptingObjectTypeNames.push_back(NormalizeCppNameForAlias(nativeName));
        if (!nativeFullName.empty())
            s_scriptingObjectTypeNames.push_back(NormalizeCppNameForAlias(nativeFullName));
    }

    void RegisterApiNativeObjectType(const std::string& nativeName,
                                     const std::string& nativeFullName)
    {
        if (!nativeName.empty())
            s_nativeObjectTypeNames.push_back(NormalizeCppNameForAlias(nativeName));
        if (!nativeFullName.empty())
            s_nativeObjectTypeNames.push_back(NormalizeCppNameForAlias(nativeFullName));
    }

    static bool IsNativePointer(const std::string& cppType)
    {
        std::string type = cppType;
        Utils::String::TrimStart(type);
        Utils::String::TrimEnd(type);
        return !type.empty() && type.back() == '*';
    }

    bool IsScriptingObjectPointer(const std::string& cppType)
    {
        if (!IsNativePointer(cppType))
            return false;

        std::string base = NormalizeCppNameForAlias(StripTypeQualifiers(cppType));
        std::string unqualified = GetUnqualifiedTypeName(base);
        static const char* scriptingBaseTypes[] = {
            "ScriptingObject", "ManagedScriptingObject", "PersistentScriptingObject", nullptr
        };
        for (int index = 0; scriptingBaseTypes[index] != nullptr; index++)
        {
            if (base == scriptingBaseTypes[index] || unqualified == scriptingBaseTypes[index])
                return true;
        }
        for (auto const& typeName : s_scriptingObjectTypeNames)
        {
            if (base == typeName || unqualified == GetUnqualifiedTypeName(typeName))
                return true;
        }
        return false;
    }

    static bool IsNativeApiObjectPointer(const std::string& cppType)
    {
        if (!IsNativePointer(cppType))
            return false;

        std::string base = NormalizeCppNameForAlias(StripTypeQualifiers(cppType));
        std::string unqualified = GetUnqualifiedTypeName(base);
        for (auto const& typeName : s_nativeObjectTypeNames)
        {
            if (base == typeName || unqualified == GetUnqualifiedTypeName(typeName))
                return true;
        }
        return false;
    }

    std::string StripTypeQualifiers(const std::string& cppType)
    {
        std::string result = cppType;
        if (Utils::String::StartsWith(result, "const "))
            result = result.substr(6);
        while (!result.empty())
        {
            char last = result.c_str()[result.length() - 1];
            if (last == '*' || last == '&' || last == ' ')
                result = result.substr(0, (int)result.length() - 1);
            else
                break;
        }
        return result;
    }

    // -------------------------------------------------------------------------
    // C# type resolution
    // -------------------------------------------------------------------------

    std::string GetCSharpInteropType(const std::string& cppType)
    {
        std::string stripped = StripTypeQualifiers(cppType);

        // Check for known type mappings first
        const TypeMapping* mapping = FindTypeMappingNormalized(stripped);
        if (mapping)
        {
            if (mapping->isString)
                return std::string("string");
            return std::string(mapping->csInterop);
        }

        // Object reference types → IntPtr
        if (IsObjectTypeRef(cppType))
            return std::string("IntPtr");

        // ScriptingObject-derived pointer
        if (IsScriptingObjectPointer(cppType))
            return std::string("IntPtr");

        // Native API object pointer (not a ScriptingObject) is represented by a generated wrapper.
        if (IsNativePointer(cppType))
            return std::string("IntPtr");

        // Collection types → use public type with special marshalling
        if (Utils::String::StartsWith(stripped, "Array<") || Utils::String::StartsWith(stripped, "Span<")
            || Utils::String::StartsWith(stripped, "List<") || Utils::String::StartsWith(stripped, "DataContainer<")
            || Utils::String::StartsWith(stripped, "BytesContainer"))
        {
            // Element type resolution happens in marshal attributes
            return std::string("IntPtr");
        }

        if (Utils::String::StartsWith(stripped, "Dictionary<") || Utils::String::StartsWith(stripped, "HashSet<"))
            return std::string("IntPtr");

        // BitArray → bool[]
        if (stripped == "BitArray")
            return std::string("IntPtr");

        // Unknown value/enum/struct type: keep a stable C# type name so public
        // wrappers and P/Invoke signatures agree. Pointer/object cases are
        // handled above and stay IntPtr.
        return ResolveCSharpTypeNameAlias(stripped);
    }

    std::string GetCSharpPublicType(const std::string& cppType)
    {
        std::string stripped = StripTypeQualifiers(cppType);
        const TypeMapping* mapping = FindTypeMappingNormalized(stripped);
        if (mapping)
            return std::string(mapping->csType);

        if (stripped == "ScriptingObject" || stripped == "SE::ScriptingObject"
            || stripped == "ManagedScriptingObject" || stripped == "SE::ManagedScriptingObject"
            || stripped == "PersistentScriptingObject" || stripped == "SE::PersistentScriptingObject")
        {
            return std::string("SE.Scripting.ScriptingObject");
        }

        // ScriptingObject-derived pointer: use the class name
        if (IsScriptingObjectPointer(cppType))
            return ResolveCSharpTypeNameAlias(stripped);

        if (IsNativeApiObjectPointer(cppType))
            return ResolveCSharpTypeNameAlias(stripped);

        // A pointer to a native type that is not itself API is represented by a
        // generated, strongly typed opaque handle. This preserves overloads
        // between unrelated native pointer types.
        if (IsNativePointer(cppType))
            return ResolveCSharpTypeNameAlias(stripped);

        // Object reference types: resolve generic argument
        if (IsObjectTypeRef(cppType))
        {
            int ltPos = Utils::String::Find(stripped, "<");
            int gtPos = Utils::String::Find(stripped, ">");
            if (ltPos != INVALID_INDEX && gtPos != INVALID_INDEX && gtPos > ltPos)
            {
                std::string innerType = stripped.substr(ltPos + 1, gtPos - ltPos - 1);
                return ResolveCSharpTypeNameAlias(StripTypeQualifiers(innerType));
            }
            return ResolveCSharpTypeNameAlias(stripped);
        }

        // Collection types
        if (Utils::String::StartsWith(stripped, "Array<") || Utils::String::StartsWith(stripped, "Span<") || Utils::String::StartsWith(stripped, "List<"))
        {
            int ltPos = Utils::String::Find(stripped, "<");
            int gtPos = Utils::String::Find(stripped, ">");
            if (ltPos != INVALID_INDEX && gtPos != INVALID_INDEX && gtPos > ltPos)
            {
                std::string elementType = StripTypeQualifiers(stripped.substr(ltPos + 1, gtPos - ltPos - 1));
                return GetCSharpPublicType(elementType) + "[]";
            }
            return ResolveCSharpTypeNameAlias(stripped) + "[]";
        }

        if (Utils::String::StartsWith(stripped, "DataContainer<"))
        {
            int ltPos = Utils::String::Find(stripped, "<");
            int gtPos = Utils::String::Find(stripped, ">");
            if (ltPos != INVALID_INDEX && gtPos != INVALID_INDEX && gtPos > ltPos)
            {
                std::string elementType = StripTypeQualifiers(stripped.substr(ltPos + 1, gtPos - ltPos - 1));
                return GetCSharpPublicType(elementType) + "[]";
            }
            return ResolveCSharpTypeNameAlias(stripped) + "[]";
        }

        if (stripped == "BytesContainer")
            return std::string("byte[]");

        if (Utils::String::StartsWith(stripped, "Dictionary<"))
        {
            int ltPos = Utils::String::Find(stripped, "<");
            int gtPos = Utils::String::Find(stripped, ">");
            if (ltPos != INVALID_INDEX && gtPos != INVALID_INDEX && gtPos > ltPos)
            {
                std::string inner = stripped.substr(ltPos + 1, gtPos - ltPos - 1);
                // Split by comma for K,V
                int commaPos = Utils::String::Find(inner, ",");
                if (commaPos != INVALID_INDEX)
                {
                    std::string keyType = StripTypeQualifiers(inner.substr(0, commaPos));
                    std::string valType = StripTypeQualifiers(inner.substr(commaPos + 1));
                    return std::string("System.Collections.Generic.Dictionary<")
                         + GetCSharpPublicType(keyType) + ", " + GetCSharpPublicType(valType) + ">";
                }
            }
            return ResolveCSharpTypeNameAlias(stripped);
        }

        if (Utils::String::StartsWith(stripped, "HashSet<"))
        {
            int ltPos = Utils::String::Find(stripped, "<");
            int gtPos = Utils::String::Find(stripped, ">");
            if (ltPos != INVALID_INDEX && gtPos != INVALID_INDEX && gtPos > ltPos)
            {
                std::string elementType = StripTypeQualifiers(stripped.substr(ltPos + 1, gtPos - ltPos - 1));
                return std::string("System.Collections.Generic.HashSet<") + GetCSharpPublicType(elementType) + ">";
            }
            return ResolveCSharpTypeNameAlias(stripped);
        }

        if (stripped == "BitArray")
            return std::string("bool[]");

        return ResolveCSharpTypeNameAlias(stripped);
    }

    std::string GetCSharpFromInterop(const std::string& cppType, const std::string& varName)
    {
        std::string stripped = StripTypeQualifiers(cppType);
        const TypeMapping* mapping = FindTypeMappingNormalized(stripped);
        if (mapping && mapping->isString)
            return varName; // StringMarshalling handles strings automatically

        if (IsScriptingObjectPointer(cppType))
        {
            std::string className = GetCSharpPublicType(cppType);
            return Utils::String::Format("({0})SE.Interop.ManagedHandleMarshaller.NativeToManaged.ConvertToManaged({1})", className, varName);
        }

        if (IsNativeApiObjectPointer(cppType))
        {
            std::string className = GetCSharpPublicType(cppType);
            return Utils::String::Format("{0}.{1}Marshaller.ConvertToManaged({2})", className, GetCSharpSimpleName(className), varName);
        }

        if (IsNativePointer(cppType))
        {
            std::string className = GetCSharpPublicType(cppType);
            return Utils::String::Format("{0}.FromUnmanaged({1})", className, varName);
        }

        if (IsObjectTypeRef(cppType))
            return varName; // handled by marshal attributes

        return varName; // blittable or handled by marshalling
    }

    std::string GetCSharpToInterop(const std::string& cppType, const std::string& varName)
    {
        std::string stripped = StripTypeQualifiers(cppType);
        const TypeMapping* mapping = FindTypeMappingNormalized(stripped);
        if (mapping && mapping->isString)
            return varName; // LibraryImport StringMarshalling handles strings

        if (IsScriptingObjectPointer(cppType))
            return Utils::String::Format("{0} != null ? {0}.__unmanagedPtr : IntPtr.Zero", varName);

        if (IsNativeApiObjectPointer(cppType))
            return Utils::String::Format("{0} != null ? {0}.__unmanagedPtr : IntPtr.Zero", varName);

        if (IsNativePointer(cppType))
            return Utils::String::Format("{0} != null ? {0}.__unmanagedPtr : IntPtr.Zero", varName);

        if (IsObjectTypeRef(cppType))
            return Utils::String::Format("{0} != null ? {0}.__unmanagedPtr : IntPtr.Zero", varName);

        if (IsCollectionType(cppType))
            return std::string("IntPtr.Zero");

        return varName; // blittable
    }

    // -------------------------------------------------------------------------
    // Pass-by-reference and type classification
    // -------------------------------------------------------------------------

    bool UsePassByReference(const std::string& cppType)
    {
        std::string stripped = StripTypeQualifiers(cppType);

        // Pointers and objects: no
        if (stripped.empty())
            return false;
        if (IsScriptingObjectPointer(cppType))
            return false;
        if (IsNativePointer(cppType))
            return false;
        if (IsObjectTypeRef(cppType))
            return false;

        // Strings and collections: no
        const TypeMapping* mapping = FindTypeMappingNormalized(stripped);
        if (mapping && mapping->isString)
            return false;
        if (IsCollectionType(cppType))
            return false;

        // Variant/Type: no
        if (stripped == "Variant" || stripped == "VariantType" || stripped == "ScriptingTypeHandle")
            return false;

        // Explicit ref: yes
        if (cppType.c_str()[cppType.length() - 1] == '&')
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

    bool IsPodType(const std::string& cppType)
    {
        std::string stripped = StripTypeQualifiers(cppType);
        const TypeMapping* mapping = FindTypeMappingNormalized(stripped);
        if (mapping)
        {
            return mapping->isBlittable;
        }
        // Non-mapped types default to non-POD
        return false;
    }

    bool IsScriptingObjectType(const std::string& cppType)
    {
        std::string stripped = StripTypeQualifiers(cppType);
        // Known scripting object base types
        if (stripped == "ScriptingObject" || stripped == "PersistentScriptingObject"
            || stripped == "ManagedObject" || stripped == "BinaryAsset"
            || stripped == "Asset" || stripped == "Actor" || stripped == "Component")
            return true;
        return IsScriptingObjectPointer(cppType);
    }

    bool IsObjectTypeRef(const std::string& cppType)
    {
        std::string stripped = StripTypeQualifiers(cppType);
        for (int i = 0; s_objectRefTypes[i] != nullptr; ++i)
        {
            if (Utils::String::StartsWith(stripped, s_objectRefTypes[i]) || Utils::String::StartsWith(GetUnqualifiedTypeName(stripped), s_objectRefTypes[i]))
                return true;
        }
        return false;
    }

    bool IsCollectionType(const std::string& cppType)
    {
        std::string stripped = StripTypeQualifiers(cppType);
        return Utils::String::StartsWith(stripped, "Array<")
            || Utils::String::StartsWith(stripped, "Span<")
            || Utils::String::StartsWith(stripped, "List<")
            || Utils::String::StartsWith(stripped, "Dictionary<")
            || Utils::String::StartsWith(stripped, "HashSet<")
            || Utils::String::StartsWith(stripped, "DataContainer<")
            || stripped == "BytesContainer"
            || stripped == "BitArray";
    }

    // -------------------------------------------------------------------------
    // Marshal attribute generation
    // -------------------------------------------------------------------------

    std::string GetCSharpParamMarshalAttribute(const std::string& cppType, const std::string& paramName)
    {
        std::string stripped = StripTypeQualifiers(cppType);

        // bool → U1
        if (stripped == "bool")
            return std::string("[MarshalAs(UnmanagedType.U1)]");

        // char → I2
        if (stripped == "Char")
            return std::string("[MarshalAs(UnmanagedType.I2)]");

        // Variant/object → ManagedHandleMarshaller
        if (stripped == "Variant" || stripped == "object")
            return std::string("[MarshalUsing(typeof(ManagedHandleMarshaller))]");

        // System.Type
        if (stripped == "VariantType" || stripped == "ScriptingTypeHandle")
            return std::string("[MarshalUsing(typeof(SystemTypeMarshaller))]");

        // Array/Span/List/DataContainer
        if (Utils::String::StartsWith(stripped, "Array<") || Utils::String::StartsWith(stripped, "Span<")
            || Utils::String::StartsWith(stripped, "List<") || Utils::String::StartsWith(stripped, "DataContainer<"))
        {
            std::string countName = Utils::String::Format("__{0}Count", paramName);
            return Utils::String::Format("[MarshalUsing(typeof(ArrayMarshaller<,>), CountElementName = \"{0}\")]", countName);
        }

        // BytesContainer
        if (stripped == "BytesContainer")
            return std::string("[MarshalUsing(typeof(ArrayMarshaller<,>), CountElementName = \"__count\")]");

        // Dictionary
        if (Utils::String::StartsWith(stripped, "Dictionary<"))
            return std::string("[MarshalUsing(typeof(DictionaryMarshaller<,>), ConstantElementCount = 0)]");

        // HashSet
        if (Utils::String::StartsWith(stripped, "HashSet<"))
            return std::string("[MarshalUsing(typeof(HashSetMarshaller<,>), ConstantElementCount = 0)]");

        // Object reference types
        if (IsObjectTypeRef(cppType))
            return std::string("[MarshalUsing(typeof(ManagedHandleMarshaller))]");

        // ScriptingObject pointer
        if (IsScriptingObjectPointer(cppType))
            return std::string(); // IntPtr, no attribute needed

        return std::string(); // no attribute for blittable types
    }

    std::string GetCSharpReturnMarshalAttribute(const std::string& cppType)
    {
        std::string stripped = StripTypeQualifiers(cppType);

        // bool → U1
        if (stripped == "bool")
            return std::string("[return: MarshalAs(UnmanagedType.U1)]");

        // Variant/object
        if (stripped == "Variant" || stripped == "object")
            return std::string("[return: MarshalUsing(typeof(ManagedHandleMarshaller))]");

        // System.Type
        if (stripped == "VariantType" || stripped == "ScriptingTypeHandle")
            return std::string("[return: MarshalUsing(typeof(SystemTypeMarshaller))]");

        // Array return
        if (Utils::String::StartsWith(stripped, "Array<") || Utils::String::StartsWith(stripped, "Span<")
            || Utils::String::StartsWith(stripped, "List<") || Utils::String::StartsWith(stripped, "DataContainer<")
            || stripped == "BytesContainer")
        {
            return std::string("[return: MarshalUsing(typeof(ArrayMarshaller<,>), CountElementName = \"__returnCount\")]");
        }

        // Dictionary return
        if (Utils::String::StartsWith(stripped, "Dictionary<"))
            return std::string("[return: MarshalUsing(typeof(DictionaryMarshaller<,>), ConstantElementCount = 0)]");

        // Object reference types
        if (IsObjectTypeRef(cppType))
            return std::string("[return: MarshalUsing(typeof(ManagedHandleMarshaller))]");

        return std::string(); // no attribute for blittable types
    }

    // -------------------------------------------------------------------------
    // CppTypeInfo implementation
    // -------------------------------------------------------------------------

    void CppTypeInfo::Parse(const std::string& cppType)
    {
        *this = CppTypeInfo(); // reset

        if (cppType.empty())
            return;

        std::string remaining = cppType;

        // Strip leading "const "
        if (Utils::String::StartsWith(remaining, "const "))
        {
            isConst = true;
            remaining = remaining.substr(6);
        }

        // Strip trailing '&&' (move ref)
        if (remaining.length() >= 2
            && remaining.c_str()[remaining.length() - 1] == '&'
            && remaining.c_str()[remaining.length() - 2] == '&')
        {
            isMoveRef = true;
            remaining = remaining.substr(0, (int)remaining.length() - 2);
        }
        // Strip trailing '&' (ref)
        else if (remaining.length() >= 1 && remaining.c_str()[remaining.length() - 1] == '&')
        {
            isRef = true;
            remaining = remaining.substr(0, (int)remaining.length() - 1);
        }

        // Strip trailing '*' (pointer)
        while (!remaining.empty() && remaining.c_str()[remaining.length() - 1] == '*')
        {
            isPointer = true;
            remaining = remaining.substr(0, (int)remaining.length() - 1);
            remaining = StripTypeQualifiers(remaining); // strip spaces
        }

        // Check for fixed array [N]
        int lbPos = Utils::String::Find(remaining, "[");
        int rbPos = Utils::String::Find(remaining, "]");
        if (lbPos != INVALID_INDEX && rbPos != INVALID_INDEX && rbPos > lbPos)
        {
            isArray = true;
            std::string sizeStr = remaining.substr(lbPos + 1, rbPos - lbPos - 1);
            arraySize = atoi(sizeStr.c_str());
            remaining = remaining.substr(0, lbPos);
        }

        // Extract base type and generic arguments
        int ltPos = Utils::String::Find(remaining, "<");
        if (ltPos != INVALID_INDEX)
        {
            baseType = StripTypeQualifiers(remaining.substr(0, ltPos));
            // Extract generic arguments (simple, non-nested)
            int depth = 0;
            int start = ltPos + 1;
            const char* s = remaining.c_str();
            int len = (int)remaining.length();
            for (int i = ltPos + 1; i < len; ++i)
            {
                if (s[i] == '<') ++depth;
                else if (s[i] == '>') --depth;
                else if (s[i] == ',' && depth == 0)
                {
                    std::string arg = StripTypeQualifiers(remaining.substr(start, i - start));
                    genericArgs.push_back(arg);
                    start = i + 1;
                }
                if (depth < 0) break;
            }
            // Last argument
            if (start < len - 1)
            {
                std::string lastArg = StripTypeQualifiers(remaining.substr(start, len - 1 - start));
                if (!lastArg.empty())
                    genericArgs.push_back(lastArg);
            }
        }
        else
        {
            baseType = StripTypeQualifiers(remaining);
        }
    }

    std::string CppTypeInfo::ToString() const
    {
        std::string result;
        if (isConst)
            result += "const ";
        result += baseType;
        if (genericArgs.size() > 0)
        {
            result += "<";
            for (int i = 0; i < genericArgs.size(); ++i)
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
            result += Utils::String::Format("[{0}]", arraySize);
        return result;
    }

} // namespace SE::BuildTool
