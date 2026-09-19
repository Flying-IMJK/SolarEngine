#include "CodeGenerator_BindingsTypeMap.h"

#include "CodeGenerator_Utils.h"
#include "Database/TypeDatabase.h"

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <sstream>

namespace SE::BuildTool
{
    struct BuiltinMapping
    {
        const char* nativeType;
        const char* managedType;
        AbiValueKind abiKind;
        bool isBlittable;
        bool isString;
        bool isStringView;
    };

    static const BuiltinMapping s_builtinMappings[] =
    {
        { "bool", "bool", AbiValueKind::Integer, true, false, false },
        { "int8", "sbyte", AbiValueKind::Integer, true, false, false },
        { "uint8", "byte", AbiValueKind::Integer, true, false, false },
        { "int16", "short", AbiValueKind::Integer, true, false, false },
        { "uint16", "ushort", AbiValueKind::Integer, true, false, false },
        { "int32", "int", AbiValueKind::Integer, true, false, false },
        { "uint32", "uint", AbiValueKind::Integer, true, false, false },
        { "int64", "long", AbiValueKind::Integer, true, false, false },
        { "uint64", "ulong", AbiValueKind::Integer, true, false, false },
        { "float", "float", AbiValueKind::Float, true, false, false },
        { "double", "double", AbiValueKind::Float, true, false, false },
        { "int", "int", AbiValueKind::Integer, true, false, false },
        { "char", "sbyte", AbiValueKind::Integer, true, false, false },
        { "Char", "char", AbiValueKind::Integer, true, false, false },
        { "SE::Char", "char", AbiValueKind::Integer, true, false, false },
        { "void", "void", AbiValueKind::Void, true, false, false },
        { "Guid", "System.Guid", AbiValueKind::BlittableStruct, true, false, false },
        { "UID", "System.Guid", AbiValueKind::BlittableStruct, true, false, false },
        { "Vector2", "Vector2", AbiValueKind::BlittableStruct, true, false, false },
        { "Vector3", "Vector3", AbiValueKind::BlittableStruct, true, false, false },
        { "Vector4", "Vector4", AbiValueKind::BlittableStruct, true, false, false },
        { "Quaternion", "Quaternion", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::Matrix", "Matrix", AbiValueKind::BlittableStruct, true, false, false },
        { "Matrix", "Matrix", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::Matrix3x3", "Matrix3x3", AbiValueKind::BlittableStruct, true, false, false },
        { "Matrix3x3", "Matrix3x3", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::Viewport", "Viewport", AbiValueKind::BlittableStruct, true, false, false },
        { "Viewport", "Viewport", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::TypeID", "uint", AbiValueKind::Integer, true, false, false },
        { "TypeID", "uint", AbiValueKind::Integer, true, false, false },
        { "SE::Float2", "Float2", AbiValueKind::BlittableStruct, true, false, false },
        { "Float2", "Float2", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::Float3", "Float3", AbiValueKind::BlittableStruct, true, false, false },
        { "Float3", "Float3", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::Float4", "Float4", AbiValueKind::BlittableStruct, true, false, false },
        { "Float4", "Float4", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::Double2", "Double2", AbiValueKind::BlittableStruct, true, false, false },
        { "Double2", "Double2", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::Double3", "Double3", AbiValueKind::BlittableStruct, true, false, false },
        { "Double3", "Double3", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::Double4", "Double4", AbiValueKind::BlittableStruct, true, false, false },
        { "Double4", "Double4", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::Int2", "Int2", AbiValueKind::BlittableStruct, true, false, false },
        { "Int2", "Int2", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::Int3", "Int3", AbiValueKind::BlittableStruct, true, false, false },
        { "Int3", "Int3", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::Int4", "Int4", AbiValueKind::BlittableStruct, true, false, false },
        { "Int4", "Int4", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::Vector2Base<float>", "Float2", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::Vector2Base<double>", "Double2", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::Vector2Base<int32>", "Int2", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::Vector3Base<float>", "Float3", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::Vector3Base<double>", "Double3", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::Vector3Base<int32>", "Int3", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::Vector4Base<float>", "Float4", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::Vector4Base<double>", "Double4", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::Vector4Base<int32>", "Int4", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::Rectangle", "Rectangle", AbiValueKind::BlittableStruct, true, false, false },
        { "Rectangle", "Rectangle", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::ScreenOrientationType", "ScreenOrientationType", AbiValueKind::Enum, true, false, false },
        { "ScreenOrientationType", "ScreenOrientationType", AbiValueKind::Enum, true, false, false },
        { "SE::Color32", "Color32", AbiValueKind::BlittableStruct, true, false, false },
        { "Color32", "Color32", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::Color", "Color", AbiValueKind::BlittableStruct, true, false, false },
        { "Color", "Color", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::Transform", "Transform", AbiValueKind::BlittableStruct, true, false, false },
        { "Transform", "Transform", AbiValueKind::BlittableStruct, true, false, false },
        { "SE::String", "string", AbiValueKind::ClrString, false, true, false },
        { "String", "string", AbiValueKind::ClrString, false, true, false },
        { "SE::StringView", "string", AbiValueKind::ClrString, false, true, true },
        { "StringView", "string", AbiValueKind::ClrString, false, true, true },
        { nullptr, nullptr, AbiValueKind::Void, false, false, false },
    };

    static const BuiltinMapping* FindBuiltin(std::string const& nativeName)
    {
        for (int i = 0; s_builtinMappings[i].nativeType; ++i)
            if (nativeName == s_builtinMappings[i].nativeType)
                return &s_builtinMappings[i];
        return nullptr;
    }

    static bool IsOneOf(std::string const& name, std::initializer_list<const char*> values)
    {
        for (const char* value : values)
            if (name == value)
                return true;
        return false;
    }

    static bool IsStrongObjectReference(TypeInfo const& type)
    {
        return type.genericityArgs.size() == 1 && IsOneOf(type.typeID.ToString(),
            { "SE::AssetRef", "AssetRef", "SE::AssetReference", "AssetReference",
              "SE::ScriptingObjectReference", "ScriptingObjectReference" });
    }

    static bool IsWeakOrSoftReference(TypeInfo const& type)
    {
        return type.genericityArgs.size() == 1 && IsOneOf(type.typeID.ToString(),
            { "SE::WeakAssetRef", "WeakAssetRef", "SE::SoftAssetRef", "SoftAssetRef",
              "SE::WeakAssetReference", "WeakAssetReference", "SE::SoftAssetReference", "SoftAssetReference" });
    }

    static bool IsExplicitlyUnsupportedFamily(TypeInfo const& type)
    {
        return IsOneOf(type.typeID.ToString(),
            { "SE::Dictionary", "Dictionary", "SE::HashSet", "HashSet", "SE::Function", "Function",
              "SE::BitArray", "BitArray", "SE::StringAnsi", "StringAnsi", "SE::StringAnsiView", "StringAnsiView" });
    }

    CollectionInfo GetCollectionInfo(TypeInfo const& type)
    {
        CollectionInfo result;
        if (type.arraySize > 0)
        {
            result.kind = CollectionKind::Fixed;
            result.fixedElementCount = type.arraySize;
            result.elementType = type;
            result.elementType.arraySize = 0;
            return result;
        }

        const std::string& name = type.typeID.ToString();
        if (IsOneOf(name, { "SE::BytesContainer", "BytesContainer" }))
        {
            result.kind = CollectionKind::Variable;
            result.elementType = TypeInfo(TypeID("uint8"));
            return result;
        }

        const bool isCollection = IsOneOf(name,
            { "SE::Array", "Array", "SE::Span", "Span", "SE::List", "List", "SE::DataContainer", "DataContainer" });
        if (isCollection && (type.genericityArgs.size() == 1 ||
            (IsOneOf(name, { "SE::Array", "Array", "SE::List", "List" }) && type.genericityArgs.size() == 2)))
        {
            result.kind = CollectionKind::Variable;
            result.elementType = type.genericityArgs[0];
        }
        return result;
    }

    std::string GetManagedTypeName(TypeInfoBase const& declaration)
    {
        auto const* structType = declaration.IsFlag(TypeInfoBase::Flag::IsClassStruct)
            ? static_cast<TypeInfoStruct const*>(&declaration) : nullptr;
        if (structType && !structType->APIInBuildMapType.empty())
            return structType->APIInBuildMapType;

        const std::string simpleName = structType && !structType->APIName.empty() ? structType->APIName : declaration.name;
        std::string result = CodeGeneratorUtils::GetFullCSNameSpaceName(declaration.namespaceScopeList);
        if (!result.empty()) result += ".";
        for (std::string const& scope : declaration.structScopeList)
            result += scope + ".";
        return result + simpleName;
    }

    static BindingTypeSemantics Unsupported(TypeInfo const& type, const char* code, std::string message)
    {
        BindingTypeSemantics result;
        result.sourceType = type;
        result.canonicalType = type.typeID;
        result.reference.pointerDepth = type.pointerDepth > 0 ? type.pointerDepth : (type.isPointer ? 1 : 0);
        result.reference.isConst = type.isConst;
        result.reference.isLValueReference = type.isRef && !type.isMoveRef;
        result.reference.isRValueReference = type.isMoveRef;
        result.diagnosticCode = code;
        result.diagnostic = std::move(message);
        return result;
    }

    static BindingTypeSemantics ResolveSemanticsImpl(TypeDatabase const& database, TypeInfo const& type,
                                                      std::string_view marshalAs, std::vector<std::string>& marshalStack)
    {
        if (!marshalAs.empty())
        {
            const std::string replacement(marshalAs);
            if (std::find(marshalStack.begin(), marshalStack.end(), replacement) != marshalStack.end())
            {
                std::string chain;
                for (auto const& item : marshalStack)
                {
                    chain += (chain.empty() ? "" : " -> ") + item;
                }
                chain += (chain.empty() ? "" : " -> ") + replacement;
                return Unsupported(type, "SEBIND003", "MarshalAs cycle: " + chain);
            }
            marshalStack.push_back(replacement);
            BindingTypeSemantics result = ResolveSemanticsImpl(database, TypeInfo(TypeID(replacement)), {}, marshalStack);
            result.sourceType = type;
            marshalStack.pop_back();
            return result;
        }

        if (type.isMoveRef)
        {
            return Unsupported(type, "SEBIND010", "rvalue references are not supported by the P0 bindings ABI");
        }
        const int pointerDepth = type.pointerDepth > 0 ? type.pointerDepth : (type.isPointer ? 1 : 0);
        if (pointerDepth > 1)
        {
            return Unsupported(type, "SEBIND004", "multi-level pointers require an explicit pointer policy");
        }
        if (IsWeakOrSoftReference(type))
        {
            return Unsupported(type, "SEBIND001", "weak and soft object references require a dedicated lifetime policy");
        }
        if (IsExplicitlyUnsupportedFamily(type))
        {
            return Unsupported(type, "SEBIND001", "type family is outside the P0 capability matrix");
        }

        BindingTypeSemantics result;
        result.sourceType = type;
        result.canonicalType = type.typeID;
        result.reference.pointerDepth = pointerDepth;
        result.reference.isConst = type.isConst;
        result.reference.isLValueReference = type.isRef && !type.isMoveRef;
        result.reference.isRValueReference = type.isMoveRef;

        if (CollectionInfo collection = GetCollectionInfo(type); collection.IsCollection())
        {
            BindingTypeSemantics element = ResolveSemanticsImpl(database, collection.elementType, {}, marshalStack);
            if (!element.IsSupported())
            {
                return Unsupported(type, element.diagnosticCode.c_str(), "unsupported collection element: " + element.diagnostic);
            }
            if (collection.kind == CollectionKind::Fixed && element.kind != BindingTypeKind::Blittable)
            {
                return Unsupported(type, "SEBIND006", "fixed arrays require a blittable P0 element type");
            }
            result.kind = BindingTypeKind::Collection;
            result.collection = std::move(collection);
            return result;
        }

        if (IsStrongObjectReference(type))
        {
            TypeInfoBase const* target = database.ResolveTypeDeclaration(type.genericityArgs[0]);
            if (!target || !target->IsFlag(TypeInfoBase::Flag::IsClassStruct))
            {
                return Unsupported(type, "SEBIND005", "object reference target declaration could not be resolved");
            }
            auto const* targetStruct = static_cast<TypeInfoStruct const*>(target);
            if (targetStruct->APIIsInterface)
            {
                return Unsupported(type, "SEBIND001", "interface object references are not supported in P0");
            }
            result.kind = BindingTypeKind::ObjectRef;
            result.declaration = target;
            result.canonicalType = target->typeID;
            return result;
        }

        const std::string& name = type.typeID.ToString();
        const std::string canonicalName = type.ToNativeType();
        if (pointerDepth == 0)
        {
            if (const BuiltinMapping* builtin = FindBuiltin(canonicalName))
            {
                result.canonicalType = TypeID(canonicalName);
                result.isEnum = builtin->abiKind == AbiValueKind::Enum;
                result.kind = builtin->isString
                    ? (builtin->isStringView ? BindingTypeKind::StringView : BindingTypeKind::String)
                    : BindingTypeKind::Blittable;
                return result;
            }
            if (const BuiltinMapping* builtin = FindBuiltin(name))
            {
                result.isEnum = builtin->abiKind == AbiValueKind::Enum;
                result.kind = builtin->isString
                    ? (builtin->isStringView ? BindingTypeKind::StringView : BindingTypeKind::String)
                    : BindingTypeKind::Blittable;
                return result;
            }
        }
        if (IsOneOf(name, { "SE::Variant", "Variant", "SE::CLRObject", "CLRObject" }))
        {
            result.kind = BindingTypeKind::VariantFamily;
            return result;
        }
        if (IsOneOf(name, { "SE::VariantType", "VariantType", "SE::ScriptingTypeHandle", "ScriptingTypeHandle", "SE::CLRClass", "CLRClass" }))
        {
            result.kind = BindingTypeKind::TypeHandle;
            return result;
        }

        TypeInfoBase const* declaration = database.ResolveTypeDeclaration(type);
        result.declaration = declaration;
        if (!declaration)
        {
            if (pointerDepth > 0)
            {
                result.kind = BindingTypeKind::OpaquePointer;
                return result;
            }
            return Unsupported(type, "SEBIND005", "type declaration could not be resolved");
        }
        result.canonicalType = declaration->typeID;
        if (declaration->IsFlag(TypeInfoBase::Flag::IsEnum))
        {
            result.isEnum = true;
            result.kind = BindingTypeKind::Blittable;
            return result;
        }
        if (!declaration->IsFlag(TypeInfoBase::Flag::IsClassStruct))
        {
            return Unsupported(type, "SEBIND001", "declaration is not an enum, class, or struct");
        }

        auto const* structType = static_cast<TypeInfoStruct const*>(declaration);
        if (!structType->APIMarshalAs.empty())
        {
            return ResolveSemanticsImpl(database, type, structType->APIMarshalAs, marshalStack);
        }
        if (structType->APIIsInterface)
        {
            return Unsupported(type, "SEBIND001", "interfaces require a dedicated interop contract");
        }
        if (!structType->isStruct)
        {
            if (pointerDepth == 0)
            {
                return Unsupported(type, "SEBIND004", "native class values require an explicit pointer semantic");
            }
            result.kind = structType->isScriptingObject ? BindingTypeKind::ScriptingObject : BindingTypeKind::NativeObject;
            return result;
        }
        if (pointerDepth > 0)
        {
            result.kind = BindingTypeKind::OpaquePointer;
            return result;
        }

        result.kind = structType->isPod ? BindingTypeKind::Blittable : BindingTypeKind::InteropStruct;
        return result;
    }

    BindingTypeSemantics ResolveBindingTypeSemantics(TypeDatabase const& database, TypeInfo const& type, std::string_view marshalAs)
    {
        std::vector<std::string> marshalStack;
        return ResolveSemanticsImpl(database, type, marshalAs, marshalStack);
    }

    static std::string GetNativeDeclarationName(TypeInfoBase const& declaration)
    {
        return CodeGeneratorUtils::GetFullNativeName(declaration.namespaceScopeList, declaration.structScopeList, declaration.name, true);
    }

    static std::string GetInteropStructName(TypeInfoBase const& declaration)
    {
        std::string result = CodeGeneratorUtils::GetFullNativeName(declaration.namespaceScopeList, declaration.structScopeList, declaration.name, false);
        Utils::String::ReplaceAll(result, "::", "_");
        return "::SE::BindingsInterop::" + result;
    }

    CppTypeConversion ResolveCppTypeConversion(TypeDatabase const&, BindingTypeSemantics const& semantics,
                                               BindingUseSite, BindingDirection)
    {
        CppTypeConversion result;
        result.kind = semantics.kind;
        if (!semantics.IsSupported())
        {
            result.diagnostic = semantics.diagnostic;
            return result;
        }

        result.strategy = InteropStrategy::Direct;
        switch (semantics.kind)
        {
        case BindingTypeKind::Blittable:
            if (semantics.sourceType.typeID == TypeID("TypeID") || semantics.sourceType.typeID == TypeID("SE::TypeID"))
            {
                result.exportType = "uint32";
                result.strategy = InteropStrategy::ManualWrapper;
            }
            else
            {
                TypeInfo exportType = semantics.sourceType;
                exportType.isConst = false;
                exportType.isRef = false;
                exportType.isMoveRef = false;
                result.exportType = semantics.declaration ? GetNativeDeclarationName(*semantics.declaration)
                                                          : exportType.ToString(false, true);
            }
            result.nativeValueType = semantics.sourceType.ToString(false, true);
            break;
        case BindingTypeKind::String:
        case BindingTypeKind::StringView:
            result.exportType = "CLRString*";
            result.nativeValueType = semantics.sourceType.ToString(false, true);
            result.strategy = InteropStrategy::CustomMarshaller;
            break;
        case BindingTypeKind::Collection:
            result.exportType = semantics.collection.kind == CollectionKind::Fixed
                ? semantics.sourceType.ToString(false, true) : "CLRArray*";
            result.nativeValueType = semantics.sourceType.ToString(false, true);
            result.strategy = InteropStrategy::CustomMarshaller;
            break;
        case BindingTypeKind::InteropStruct:
            result.exportType = GetInteropStructName(*semantics.declaration);
            result.nativeValueType = GetNativeDeclarationName(*semantics.declaration);
            result.strategy = InteropStrategy::ManualWrapper;
            break;
        case BindingTypeKind::ScriptingObject:
        case BindingTypeKind::NativeObject:
            result.exportType = "void*";
            result.nativeValueType = GetNativeDeclarationName(*semantics.declaration) + "*";
            result.strategy = InteropStrategy::ManualWrapper;
            break;
        case BindingTypeKind::ObjectRef:
        case BindingTypeKind::VariantFamily:
            result.exportType = "CLRObject*";
            result.nativeValueType = semantics.sourceType.ToString(false, true);
            result.strategy = InteropStrategy::ManualWrapper;
            break;
        case BindingTypeKind::TypeHandle:
            result.exportType = "CLRTypeObject*";
            result.nativeValueType = semantics.sourceType.ToString(false, true);
            result.strategy = InteropStrategy::ManualWrapper;
            break;
        case BindingTypeKind::OpaquePointer:
            result.exportType = "void*";
            result.nativeValueType = semantics.sourceType.ToString(false, true);
            break;
        default:
            result.strategy = InteropStrategy::Unsupported;
            result.diagnostic = "unsupported C++ lowering";
            break;
        }
        return result;
    }

    static std::string GetManagedNameForSemantics(BindingTypeSemantics const& semantics)
    {
        if (semantics.declaration)
            return GetManagedTypeName(*semantics.declaration);
        if (const BuiltinMapping* builtin = FindBuiltin(semantics.sourceType.ToNativeType()))
            return builtin->managedType;
        if (const BuiltinMapping* builtin = FindBuiltin(semantics.sourceType.typeID.ToString()))
            return builtin->managedType;
        return {};
    }

    CSharpTypeConversion ResolveCSharpTypeConversion(TypeDatabase const& database, BindingTypeSemantics const& semantics,
                                                     BindingUseSite useSite, BindingDirection direction)
    {
        CSharpTypeConversion result;
        result.kind = semantics.kind;
        if (!semantics.IsSupported())
        {
            result.diagnostic = semantics.diagnostic;
            return result;
        }

        switch (semantics.kind)
        {
        case BindingTypeKind::Blittable:
            result.publicType = GetManagedNameForSemantics(semantics);
            result.libraryImportManagedType = result.publicType;
            result.strategy = InteropStrategy::Direct;
            break;
        case BindingTypeKind::String:
        case BindingTypeKind::StringView:
            if (semantics.kind == BindingTypeKind::StringView && direction != BindingDirection::In)
            {
                result.strategy = InteropStrategy::Unsupported;
                result.diagnostic = "StringView is call-scope input-only in P0";
                break;
            }
            result.publicType = "string";
            result.libraryImportManagedType = "string";
            result.marshaller = "SE.Interop.StringMarshaller";
            result.strategy = InteropStrategy::CustomMarshaller;
            break;
        case BindingTypeKind::Collection:
        {
            BindingTypeSemantics elementSemantics = ResolveBindingTypeSemantics(database, semantics.collection.elementType);
            CSharpTypeConversion element = ResolveCSharpTypeConversion(database, elementSemantics, BindingUseSite::ArrayElement, BindingDirection::In);
            if (element.strategy == InteropStrategy::Unsupported)
            {
                result.diagnostic = "unsupported collection element C# lowering: " + element.diagnostic;
                break;
            }
            result.publicType = element.publicType + "[]";
            result.libraryImportManagedType = result.publicType;
            result.marshaller = "SE.Interop.ArrayMarshaller<,>";
            result.strategy = InteropStrategy::CustomMarshaller;
            break;
        }
        case BindingTypeKind::InteropStruct:
        {
            result.publicType = GetManagedTypeName(*semantics.declaration);
            const int separator = Utils::String::FindLast(result.publicType, '.');
            const std::string simpleName = separator == INVALID_INDEX ? result.publicType : result.publicType.substr(separator + 1);
            result.marshaller = result.publicType + "Marshaller";
            result.libraryImportManagedType = result.marshaller + "." + simpleName + "Internal";
            result.strategy = InteropStrategy::ManualWrapper;
            break;
        }
        case BindingTypeKind::ScriptingObject:
        case BindingTypeKind::NativeObject:
        case BindingTypeKind::ObjectRef:
            result.publicType = GetManagedTypeName(*semantics.declaration);
            result.libraryImportManagedType = "IntPtr";
            result.strategy = InteropStrategy::ManualWrapper;
            break;
        case BindingTypeKind::VariantFamily:
            result.publicType = "object";
            result.libraryImportManagedType = "IntPtr";
            result.strategy = InteropStrategy::ManualWrapper;
            break;
        case BindingTypeKind::TypeHandle:
            result.publicType = "System.Type";
            result.libraryImportManagedType = "IntPtr";
            result.strategy = InteropStrategy::ManualWrapper;
            break;
        case BindingTypeKind::OpaquePointer:
            result.publicType = "IntPtr";
            result.libraryImportManagedType = "IntPtr";
            result.strategy = InteropStrategy::Direct;
            break;
        default:
            result.strategy = InteropStrategy::Unsupported;
            result.diagnostic = "unsupported C# lowering";
            break;
        }
        return result;
    }

    BindingDirection GetBindingDirection(TypeInfoParam const& parameter)
    {
        switch (parameter.direction)
        {
        case ApiParameterDirection::Out: return BindingDirection::Out;
        case ApiParameterDirection::Ref: return BindingDirection::Ref;
        default: return BindingDirection::In;
        }
    }

    static AbiValueKind GetAbiValueKind(BindingTypeSemantics const& semantics)
    {
        if (semantics.sourceType.typeID == TypeInfo::Void.typeID) return AbiValueKind::Void;
        switch (semantics.kind)
        {
        case BindingTypeKind::String:
        case BindingTypeKind::StringView: return AbiValueKind::ClrString;
        case BindingTypeKind::Collection: return AbiValueKind::ClrArray;
        case BindingTypeKind::InteropStruct: return AbiValueKind::InteropStruct;
        case BindingTypeKind::ScriptingObject:
        case BindingTypeKind::NativeObject:
        case BindingTypeKind::OpaquePointer: return AbiValueKind::OpaquePointer;
        case BindingTypeKind::ObjectRef:
        case BindingTypeKind::VariantFamily: return AbiValueKind::ClrObject;
        case BindingTypeKind::TypeHandle: return AbiValueKind::ClrTypeObject;
        case BindingTypeKind::Blittable:
            if (semantics.declaration && semantics.declaration->IsFlag(TypeInfoBase::Flag::IsEnum)) return AbiValueKind::Enum;
            if (semantics.declaration) return AbiValueKind::BlittableStruct;
            if (const BuiltinMapping* builtin = FindBuiltin(semantics.sourceType.typeID.ToString())) return builtin->abiKind;
            return AbiValueKind::BlittableStruct;
        default: return AbiValueKind::OpaquePointer;
        }
    }

    static AbiType MakeAbiType(BindingTypeSemantics const& semantics, AbiPassMode passMode = AbiPassMode::Value)
    {
        return { GetAbiValueKind(semantics), semantics.canonicalType, passMode, semantics.kind };
    }

    static void HashText(uint64_t& value, std::string_view text)
    {
        for (char c : text)
        {
            value ^= static_cast<uint8_t>(c);
            value *= 1099511628211ull;
        }
        value ^= 0xff;
        value *= 1099511628211ull;
    }

    static std::string BuildFingerprint(FunctionAbiPlan const& plan)
    {
        uint64_t hash = 14695981039346656037ull;
        HashText(hash, plan.entryPoint);
        HashText(hash, plan.usesHiddenResult ? "hidden-result" : "direct-result");
        HashText(hash, std::to_string(static_cast<int>(plan.returnType.kind)));
        HashText(hash, std::to_string(static_cast<int>(plan.returnType.passMode)));
        for (auto const& parameter : plan.parameters)
        {
            HashText(hash, std::to_string(static_cast<int>(parameter.role)));
            HashText(hash, std::to_string(static_cast<int>(parameter.type.kind)));
            HashText(hash, std::to_string(static_cast<int>(parameter.type.passMode)));
            HashText(hash, parameter.type.canonicalType.ToString());
        }
        std::ostringstream stream;
        stream << std::hex << std::setfill('0') << std::setw(16) << hash;
        return stream.str();
    }

    FunctionAbiPlan BuildFunctionAbiPlan(TypeDatabase const& database, TypeInfoStruct const& owner, TypeInfoFunc const& function)
    {
        FunctionAbiPlan plan;
        plan.entryPoint = function.entryPoint;

        BindingTypeSemantics returnSemantics = ResolveBindingTypeSemantics(database, function.returnType, function.marshalAs);
        CSharpTypeConversion returnCSharp = ResolveCSharpTypeConversion(database, returnSemantics, BindingUseSite::Return, BindingDirection::Out);
        if (!returnSemantics.IsSupported() || returnCSharp.strategy == InteropStrategy::Unsupported)
            plan.diagnostics.push_back("SEBIND001 " + owner.name + "::" + function.name + " return '" +
                function.returnType.ToString() + "': " +
                (!returnSemantics.IsSupported() ? returnSemantics.diagnostic : returnCSharp.diagnostic));

        const AbiValueKind returnAbiKind = GetAbiValueKind(returnSemantics);

        plan.usesHiddenResult = returnSemantics.kind == BindingTypeKind::InteropStruct ||
            returnAbiKind == AbiValueKind::BlittableStruct ||
            (function.returnType.isRef && returnSemantics.kind == BindingTypeKind::Blittable);

        if (function.isVirtual && function.returnType.isRef)
            plan.diagnostics.push_back("SEBIND008 " + owner.name + "::" + function.name +
                " virtual reference returns are not supported in P0");
        plan.returnType = plan.usesHiddenResult
            ? AbiType{ AbiValueKind::Void, TypeInfo::Void.typeID, AbiPassMode::Value, BindingTypeKind::Blittable }
            : MakeAbiType(returnSemantics);

        if (!function.isStatic)
        {
            AbiParameterPlan thisParameter;
            thisParameter.role = AbiParameterRole::This;
            thisParameter.type = { AbiValueKind::OpaquePointer, owner.typeID, AbiPassMode::Value, BindingTypeKind::OpaquePointer };
            plan.parameters.push_back(thisParameter);
        }

        for (int i = 0; i < function.params.size(); ++i)
        {
            TypeInfoParam const& parameter = function.params[i];
            const BindingDirection direction = GetBindingDirection(parameter);
            BindingTypeSemantics semantics = ResolveBindingTypeSemantics(database, parameter.type, parameter.marshalAs);
            CSharpTypeConversion csharp = ResolveCSharpTypeConversion(database, semantics, BindingUseSite::Parameter, direction);
            if (!semantics.IsSupported() || csharp.strategy == InteropStrategy::Unsupported)
                plan.diagnostics.push_back("SEBIND001 " + owner.name + "::" + function.name + " parameter '" + parameter.name +
                    "' type '" + parameter.type.ToString() + "': " +
                    (!semantics.IsSupported() ? semantics.diagnostic : csharp.diagnostic));
            if (semantics.kind == BindingTypeKind::Collection && direction != BindingDirection::In &&
                IsOneOf(parameter.type.typeID.ToString(), { "SE::Span", "Span" }))
                plan.diagnostics.push_back("SEBIND001 " + owner.name + "::" + function.name + " parameter '" + parameter.name +
                    "' type '" + parameter.type.ToString() + "': mutable Span replacement is outside the P0 collection policy");

            PublicToAbiMapping mapping;
            mapping.publicParameterIndex = i;
            AbiParameterPlan abiParameter;
            abiParameter.role = AbiParameterRole::PublicParameter;
            abiParameter.publicParameterIndex = i;
            AbiPassMode passMode = AbiPassMode::Value;
            if (direction == BindingDirection::Out) passMode = AbiPassMode::OutPointer;
            else if (direction == BindingDirection::Ref) passMode = AbiPassMode::Pointer;
            abiParameter.type = MakeAbiType(semantics, passMode);
            mapping.abiParameterIndices.push_back(static_cast<int>(plan.parameters.size()));
            plan.parameters.push_back(abiParameter);

            if (semantics.collection.HasRuntimeCount())
            {
                AbiParameterPlan countParameter;
                countParameter.role = AbiParameterRole::HiddenCount;
                countParameter.publicParameterIndex = i;
                AbiPassMode countPassMode = AbiPassMode::Value;
                if (direction == BindingDirection::Ref) countPassMode = AbiPassMode::Pointer;
                else if (direction == BindingDirection::Out) countPassMode = AbiPassMode::OutPointer;
                countParameter.type = { AbiValueKind::Integer, TypeID("int32"), countPassMode, BindingTypeKind::Blittable };
                mapping.abiParameterIndices.push_back(static_cast<int>(plan.parameters.size()));
                plan.parameters.push_back(countParameter);
            }
            plan.publicMappings.push_back(std::move(mapping));
        }

        if (returnSemantics.collection.HasRuntimeCount())
        {
            AbiParameterPlan countParameter;
            countParameter.role = AbiParameterRole::HiddenCount;
            countParameter.publicParameterIndex = -1;
            countParameter.type = { AbiValueKind::Integer, TypeID("int32"), AbiPassMode::OutPointer, BindingTypeKind::Blittable };
            plan.parameters.push_back(countParameter);
        }
        if (plan.usesHiddenResult)
        {
            AbiParameterPlan resultParameter;
            resultParameter.role = AbiParameterRole::HiddenResult;
            resultParameter.type = MakeAbiType(returnSemantics, AbiPassMode::OutPointer);
            plan.parameters.push_back(resultParameter);
        }

        plan.fingerprint = BuildFingerprint(plan);
        return plan;
    }

    static void ValidateUseSite(TypeDatabase const& database, TypeInfoStruct const& owner, std::string const& memberName,
                                TypeInfo const& type, std::string_view marshalAs, BindingUseSite useSite,
                                BindingDirection direction, std::vector<std::string>& diagnostics)
    {
        BindingTypeSemantics semantics = ResolveBindingTypeSemantics(database, type, marshalAs);
        CppTypeConversion cpp = ResolveCppTypeConversion(database, semantics, useSite, direction);
        CSharpTypeConversion csharp = ResolveCSharpTypeConversion(database, semantics, useSite, direction);
        if (!semantics.IsSupported() || cpp.strategy == InteropStrategy::Unsupported || csharp.strategy == InteropStrategy::Unsupported)
        {
            const std::string detail = !semantics.IsSupported() ? semantics.diagnostic
                : (!cpp.diagnostic.empty() ? cpp.diagnostic : csharp.diagnostic);
            const std::string code = semantics.diagnosticCode.empty() ? "SEBIND001" : semantics.diagnosticCode;
            diagnostics.push_back(code + " " + owner.name + "::" + memberName + ": " + detail);
        }
    }

    static void ValidateOwner(TypeDatabase const& database, TypeInfoStruct const& owner,
                              std::vector<std::string>& diagnostics, std::vector<std::string>* fingerprints)
    {
        for (auto const& function : owner.functions)
        {
            if (!function.isAPI) continue;
            FunctionAbiPlan plan = BuildFunctionAbiPlan(database, owner, function);
            diagnostics.insert(diagnostics.end(), plan.diagnostics.begin(), plan.diagnostics.end());
            if (fingerprints)
                fingerprints->push_back(plan.entryPoint + ":" + plan.fingerprint);
        }

        for (auto const& field : owner.fields)
        {
            if (owner.isStruct && !field.isStatic)
                ValidateUseSite(database, owner, field.name, field.type, field.marshalAs,
                    BindingUseSite::Field, BindingDirection::In, diagnostics);
            if (!field.isAPI) continue;

            TypeInfoFunc getter;
            getter.name = field.name;
            getter.uniqueName = field.name + "_Get";
            getter.entryPoint = Utils::String::Format("{0}_{1}_Get", owner.name, field.name);
            getter.returnType = field.type;
            getter.isStatic = field.isStatic;
            getter.isAPI = true;
            getter.marshalAs = field.marshalAs;
            FunctionAbiPlan getterPlan = BuildFunctionAbiPlan(database, owner, getter);
            diagnostics.insert(diagnostics.end(), getterPlan.diagnostics.begin(), getterPlan.diagnostics.end());
            if (fingerprints) fingerprints->push_back(getterPlan.entryPoint + ":" + getterPlan.fingerprint);

            if (!field.APIIsReadOnly)
            {
                TypeInfoFunc setter;
                setter.name = field.name;
                setter.uniqueName = field.name + "_Set";
                setter.entryPoint = Utils::String::Format("{0}_{1}_Set", owner.name, field.name);
                setter.returnType = TypeInfo::Void;
                setter.isStatic = field.isStatic;
                setter.isAPI = true;
                TypeInfoParam parameter;
                parameter.name = "value";
                parameter.type = field.type;
                parameter.marshalAs = field.marshalAs;
                setter.params.push_back(std::move(parameter));
                FunctionAbiPlan setterPlan = BuildFunctionAbiPlan(database, owner, setter);
                diagnostics.insert(diagnostics.end(), setterPlan.diagnostics.begin(), setterPlan.diagnostics.end());
                if (fingerprints) fingerprints->push_back(setterPlan.entryPoint + ":" + setterPlan.fingerprint);
            }
        }

        for (auto const& eventInfo : owner.events)
        {
            if (!eventInfo.isAPI) continue;
            for (auto const& parameter : eventInfo.params)
                ValidateUseSite(database, owner, eventInfo.name + "." + parameter.name, parameter.type, parameter.marshalAs,
                    BindingUseSite::Parameter, GetBindingDirection(parameter), diagnostics);
        }
    }

    bool ValidateBindingsHeader(TypeDatabase const& database, BindingsHeaderInfo const& header,
                                std::vector<std::string>& diagnostics, std::vector<std::string>* fingerprints)
    {
        const size_t initialCount = diagnostics.size();
        for (auto const* type : header.classes)
            if (type && type->APIInBuildMapType.empty()) ValidateOwner(database, *type, diagnostics, fingerprints);
        for (auto const* type : header.interfaces)
            if (type && type->APIInBuildMapType.empty()) ValidateOwner(database, *type, diagnostics, fingerprints);
        return diagnostics.size() == initialCount;
    }

    bool IsKnownBlittableBuiltin(TypeInfo const& type)
    {
        if (type.isPointer || type.pointerDepth > 0 || type.isRef || type.isMoveRef || type.arraySize > 0 || !type.genericityArgs.empty())
            return false;
        const BuiltinMapping* mapping = FindBuiltin(type.typeID.ToString());
        return mapping && mapping->isBlittable;
    }
} // namespace SE::BuildTool
