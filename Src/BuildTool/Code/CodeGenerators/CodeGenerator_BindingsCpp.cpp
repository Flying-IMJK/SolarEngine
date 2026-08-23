
#include "CodeGenerator_BindingsCpp.h"
#include "CodeGenerator_BindingsTypeMap.h"
#include "CodeGenerator_Utils.h"

#include "Core/Utils.h"

namespace SE::BuildTool
{
    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    static std::string GetPropertyName(TypeInfoFunc const& fn)
    {
        if ((Utils::String::StartsWith(fn.name, "Get") || Utils::String::StartsWith(fn.name, "Set"))
            && fn.name.length() > 3)
        {
            return fn.name.substr(3);
        }
        return fn.name;
    }

    static std::string GetCppNativeSimpleName(const TypeInfoStruct& cls)
    {
        return cls.APIName.empty() ? cls.name : cls.APIName;
    }

    static std::string GetCppNativeTypeName(const TypeInfoStruct& cls)
    {
        return CodeGeneratorUtils::GetNativeName(cls.namespaceScopeList, cls.structScopeList, GetCppNativeSimpleName(cls));
    }

    static std::string GetCppNativeInvokeTypeName(const TypeInfoStruct& cls)
    {
        std::string nativeName = cls.APIIsNativeInvokeUseName ? cls.name : GetCppNativeSimpleName(cls);
        return CodeGeneratorUtils::GetNativeName(cls.namespaceScopeList, cls.structScopeList, nativeName);
    }

    static std::string GetCollectionBaseType(const std::string& cppType)
    {
        CppTypeInfo type;
        type.Parse(cppType);
        std::string base = type.baseType;
        const size_t separator = base.rfind("::");
        return separator == std::string::npos ? base : base.substr(separator + 2);
    }

    static std::string GetCollectionCountExpression(const std::string& cppType, const std::string& expression)
    {
        return GetCollectionBaseType(cppType) == "Span"
            ? expression + ".Length()"
            : expression + ".Count()";
    }

    static std::string GetCollectionDataExpression(const std::string& expression)
    {
        return expression + ".Get()";
    }

    static std::string GetCppSimpleTypeName(const std::string& cppType)
    {
        const std::string stripped = StripTypeQualifiers(cppType);
        const size_t namespaceSeparator = stripped.rfind("::");
        return namespaceSeparator == std::string::npos ? stripped : stripped.substr(namespaceSeparator + 2);
    }

    static std::string GetInteropValueType(const std::string& cppType)
    {
        std::string stripped = StripTypeQualifiers(cppType);
        if (IsStringType(cppType))
            return "CLRString*";

        std::string interopStruct = GetApiInteropStructCppType(cppType);
        if (!interopStruct.empty())
            return interopStruct;

        if (IsScriptingObjectPointer(cppType) || CodeGeneratorUtils::IsNativePointer(cppType))
            return "void*";

        if (stripped == "Variant")
            return "CLRObject*";
        if (stripped == "VariantType" || stripped == "ScriptingTypeHandle")
            return "CLRTypeObject*";

        return CodeGeneratorUtils::QualifyCppType(stripped);
    }

    std::string BindingsCppGenerator::GetInteropReturnType(const TypeInfoFunc& fn) const
    {
        std::string const returnType = fn.returnType.ToString();
        if (returnType == "void")
            return "void";
        if (GetCollectionAbiInfo(returnType, fn.returnArraySize).IsCollection())
            return "CLRArray*";
        return GetInteropValueType(returnType);
    }

    std::string BindingsCppGenerator::GetInteropParamType(const TypeInfoParam& param) const
    {
        std::string const cppType = param.type.ToString();
        if (GetCollectionAbiInfo(cppType, param.arraySize).IsCollection())
            return "CLRArray*";
        return GetInteropValueType(cppType);
    }

    TypeInfoBase const* BindingsCppGenerator::GetRegisteredType(TypeID typeID) const
    {
        TypeInfoBase const* registeredType = m_Database.GetType(typeID);
        if (registeredType != nullptr)
            return registeredType;

        std::string const stripped = StripTypeQualifiers(typeID.ToString());
        if (stripped.empty() || stripped == typeID.ToString())
            return nullptr;

        return m_Database.GetType(TypeID(stripped));
    }

    bool BindingsCppGenerator::CanGenerateVariantFieldAccess(TypeID typeID) const
    {
        std::string const cppType = typeID.ToString();
        std::string const stripped = StripTypeQualifiers(cppType);

        if (IsCollectionType(cppType) || IsApiInteropStructType(cppType))
            return false;

        TypeInfoBase const* registeredType = GetRegisteredType(typeID);
        if (registeredType != nullptr)
        {
            if (registeredType->IsFlag(TypeInfoBase::Flag::IsEnum))
                return true;
            if (registeredType->IsFlag(TypeInfoBase::Flag::IsStruct))
            {
                TypeInfoStruct const* structType = static_cast<TypeInfoStruct const*>(registeredType);
                return structType->isPod || structType->isScriptingObject;
            }
        }

        return FindTypeMapping(stripped.c_str()) != nullptr
            || FindTypeMapping(GetCppSimpleTypeName(stripped).c_str()) != nullptr
            || IsStringType(cppType)
            || IsScriptingObjectPointer(cppType)
            || IsPodType(stripped)
            || stripped == "Variant"
            || stripped == "VariantType"
            || stripped == "ScriptingTypeHandle";
    }

    std::string BindingsCppGenerator::GetNativeToManagedConvert(TypeID typeID, const std::string& expr) const
    {
        TypeInfoBase const* registeredType = GetRegisteredType(typeID);
        std::string const cppType = typeID.ToString();
        std::string stripped = StripTypeQualifiers(cppType);
        if (registeredType != nullptr && registeredType->IsFlag(TypeInfoBase::Flag::IsEnum))
            return expr;
        if (IsStringType(cppType))
        {
            return Utils::String::Format("CLRUtils::ToString({0})", expr);
        }
        if (IsApiInteropStructType(cppType))
            return Utils::String::Format("BindingsInterop::ToManaged({0})", expr);
        if (stripped == "Variant")
            return Utils::String::Format("CLRUtils::BoxVariant({0})", expr);
        if (stripped == "VariantType")
            return Utils::String::Format("CLRUtils::BoxVariantType({0})", expr);
        if (stripped == "ScriptingTypeHandle")
            return Utils::String::Format("CLRUtils::BoxScriptingTypeHandle({0})", expr);
        if (IsScriptingObjectPointer(cppType))
        {
            // API headers frequently forward-declare a scripting object return
            // type. Use an explicit base-pointer reinterpret cast so the stub
            // does not require the concrete type definition merely to emit the
            // managed handle conversion.
            return Utils::String::Format("ScriptingObject::ToManaged(reinterpret_cast<ScriptingObject*>({0}))", expr);
        }
        return expr;
    }

    std::string BindingsCppGenerator::GetManagedToNativeConvert(TypeID typeID, const std::string& expr) const
    {
        TypeInfoBase const* registeredType = GetRegisteredType(typeID);
        std::string const cppType = typeID.ToString();
        std::string stripped = StripTypeQualifiers(cppType);
        if (registeredType != nullptr && registeredType->IsFlag(TypeInfoBase::Flag::IsEnum))
            return expr;
        if (IsStringType(cppType))
        {
            const std::string simpleType = GetCppSimpleTypeName(cppType);
            if (simpleType == "String" || simpleType == "StringView")
                return Utils::String::Format("CLRUtils::ToString((CLRString*){0})", expr);
            return Utils::String::Format("CLRUtils::ToStringAnsi((CLRString*){0})", expr);
        }
        if (IsApiInteropStructType(cppType))
            return Utils::String::Format("BindingsInterop::ToNative({0})", expr);
        if (stripped == "Variant")
            return Utils::String::Format("CLRUtils::UnboxVariant((CLRObject*){0})", expr);
        if (stripped == "VariantType")
            return Utils::String::Format("CLRUtils::UnboxVariantType((CLRTypeObject*){0})", expr);
        if (stripped == "ScriptingTypeHandle")
            return Utils::String::Format("CLRUtils::UnboxScriptingTypeHandle((CLRTypeObject*){0})", expr);
        if (IsScriptingObjectPointer(cppType))
            return Utils::String::Format("({0}*){1}", CodeGeneratorUtils::QualifyCppType(stripped), expr);
        if (CodeGeneratorUtils::IsNativePointer(cppType))
            return Utils::String::Format("({0}*){1}", CodeGeneratorUtils::QualifyCppType(stripped), expr);
        return expr;
    }

    bool BindingsCppGenerator::ShouldUseOutResult(const std::string& cppType) const
    {
        std::string stripped = StripTypeQualifiers(cppType);
        if (stripped == "void" || stripped.empty())
            return false;
        if (IsStringType(cppType))
            return false;
        if (GetCollectionAbiInfo(cppType).IsCollection())
            return false;
        if (IsScriptingObjectPointer(cppType) || CodeGeneratorUtils::IsNativePointer(cppType))
            return false;
        if (IsApiInteropStructType(cppType))
            return true;
        return UsePassByReference(cppType);
    }

    bool BindingsCppGenerator::NeedsInteropPointer(const TypeInfoParam& param) const
    {
        std::string const cppType = param.type.ToString();
        if (GetCollectionAbiInfo(cppType, param.arraySize).IsCollection())
            return false;
        return param.isOut || param.isRef || UsePassByReference(cppType);
    }

    std::string BindingsCppGenerator::GetNativeToVariantConvert(TypeID typeID, const std::string& expr) const
    {
        TypeInfoBase const* registeredType = GetRegisteredType(typeID);
        std::string const cppType = typeID.ToString();
        std::string stripped = StripTypeQualifiers(cppType);
        if (registeredType != nullptr && registeredType->IsFlag(TypeInfoBase::Flag::IsEnum))
            return Utils::String::Format("Variant((uint64){0})", expr);
        if (stripped == "bool" || stripped == "int32" || stripped == "uint32"
            || stripped == "int64" || stripped == "uint64" || stripped == "float"
            || stripped == "double")
            return Utils::String::Format("Variant({0})", expr);
        if (IsScriptingObjectPointer(cppType))
            return Utils::String::Format("Variant((ScriptingObject*){0})", expr);
        return Utils::String::Format("Variant({0})", expr);
    }

    std::string BindingsCppGenerator::GetVariantToNativeConvert(TypeID typeID, const std::string& expr) const
    {
        TypeInfoBase const* registeredType = GetRegisteredType(typeID);
        std::string const cppType = typeID.ToString();
        std::string stripped = StripTypeQualifiers(cppType);
        if (registeredType != nullptr && registeredType->IsFlag(TypeInfoBase::Flag::IsEnum))
            return Utils::String::Format("({0})(uint64){1}", CodeGeneratorUtils::QualifyCppType(stripped), expr);
        if (stripped == "Variant" || stripped == "VariantType")
            return expr;
        const std::string simpleType = GetCppSimpleTypeName(cppType);
        if (simpleType == "String")
            return Utils::String::Format("(StringView){0}", expr);
        if (simpleType == "StringAnsi")
            return Utils::String::Format("(StringAnsiView){0}", expr);
        if (IsStringType(cppType))
            return Utils::String::Format("(StringView){0}", expr);
        if (IsScriptingObjectPointer(cppType))
        {
            return Utils::String::Format("({0}*)ScriptingObject::Cast((ScriptingObject*){1})", CodeGeneratorUtils::QualifyCppType(stripped), expr);
        }
        // Enum types
        if (FindTypeMapping(stripped.c_str()) == nullptr && !IsPodType(stripped))
        {
            return Utils::String::Format("({0})(uint64){1}", CodeGeneratorUtils::QualifyCppType(stripped), expr);
        }
        return Utils::String::Format("({0}){1}", CodeGeneratorUtils::QualifyCppType(stripped), expr);
    }

    std::string BindingsCppGenerator::BuildWrapperParams(const TypeInfoStruct& cls, const TypeInfoFunc& fn, bool forExport) const
    {
        std::string params;
        if (!fn.isStatic)
        {
            std::string nativeTypeName = GetCppNativeTypeName(cls);
            if (forExport)
            {
                params += Utils::String::Format("::{0}* __obj", nativeTypeName);
            }
            else
            {
                params += Utils::String::Format("::{0}* __obj", nativeTypeName);
            }
        }
        for (int i = 0; i < fn.params.size(); ++i)
        {
            if (params.length() > 0)
            {
                params += ", ";
            }
            std::string const cppType = fn.params[i].type.ToString();
            const CollectionAbiInfo collection = GetCollectionAbiInfo(cppType, fn.params[i].arraySize);
            if (collection.IsCollection())
            {
                params += Utils::String::Format("CLRArray* {0}", fn.params[i].name);
                if (collection.HasRuntimeCount())
                    params += Utils::String::Format(", int32 __{0}Count", fn.params[i].name);
            }
            else
            {
                std::string interopType = GetInteropParamType(fn.params[i]);
                params += Utils::String::Format("{0}{1} {2}", interopType,
                    NeedsInteropPointer(fn.params[i]) ? "*" : "", fn.params[i].name);
            }
        }
        return params;
    }

    std::string BindingsCppGenerator::BuildForwardArgs(const TypeInfoFunc& fn) const
    {
        std::string args;
        for (int i = 0; i < fn.params.size(); ++i)
        {
            if (i > 0)
                args += ", ";
            args += fn.params[i].name;
            const CollectionAbiInfo collection = GetCollectionAbiInfo(fn.params[i].type.ToString(), fn.params[i].arraySize);
            if (collection.HasRuntimeCount())
                args += Utils::String::Format(", __{0}Count", fn.params[i].name);
        }
        return args;
    }

    std::string BindingsCppGenerator::BuildCallArgs(const TypeInfoStruct& cls, const TypeInfoFunc& fn, std::string& setupOut) const
    {
        std::string args;
        for (int i = 0; i < fn.params.size(); ++i)
        {
            if (i > 0)
                args += ", ";
            std::string converted;
            const TypeInfoParam& param = fn.params[i];
            std::string const paramCppType = param.type.ToString();
            const CollectionAbiInfo collection = GetCollectionAbiInfo(paramCppType, param.arraySize);
            if (collection.IsCollection())
            {
                const std::string baseType = GetCollectionBaseType(paramCppType);
                const std::string nativeElementType = CodeGeneratorUtils::QualifyCppType(collection.elementCppType);
                const std::string localName = Utils::String::Format("__{0}Native", param.name);
                const std::string countName = Utils::String::Format("__{0}NativeCount", param.name);

                if (baseType == "BytesContainer")
                {
                    setupOut += Utils::String::Format("        auto {0} = CLRUtils::LinkArray({1});\n", localName, param.name);
                    converted = localName;
                }
                else if (baseType == "DataContainer")
                {
                    setupOut += Utils::String::Format("        ::SE::DataContainer<{0}> {1};\n", nativeElementType, localName);
                    setupOut += Utils::String::Format("        CLRUtils::ToArray({0}, {1});\n", param.name, localName);
                    converted = localName;
                }
                else
                {
                    // Use a List as the conversion buffer for both owning
                    // List parameters and non-owning Span parameters. Element
                    // conversion deliberately goes through the same scalar ABI
                    // converter as regular parameters, so non-blittable API
                    // structs (for example Sprite) remain safe.
                    setupOut += Utils::String::Format("        ::SE::List<{0}> {1};\n", nativeElementType, localName);
                    setupOut += Utils::String::Format("        const int32 {0} = {1} ? ({2} < CLRCore::Array::GetLength({1}) ? {2} : CLRCore::Array::GetLength({1})) : 0;\n",
                        countName, param.name, collection.HasRuntimeCount() ? Utils::String::Format("__{0}Count", param.name) : Utils::String::Format("CLRCore::Array::GetLength({0})", param.name));
                    setupOut += Utils::String::Format("        {0}.Resize({1});\n", localName, countName);
                    setupOut += Utils::String::Format("        if ({1} > 0)\n        {{\n", localName, countName);
                    setupOut += Utils::String::Format("            auto* __{0}Items = CLRCore::Array::GetAddress<{1}>({2});\n", param.name,
                        GetInteropValueType(collection.elementCppType), param.name);
                    setupOut += Utils::String::Format("            for (int32 i = 0; i < {0}; ++i) {1}[i] = {2};\n        }}\n",
                        countName, localName, GetManagedToNativeConvert(TypeID(collection.elementCppType),
                            Utils::String::Format("__{0}Items[i]", param.name)));
                    if (baseType == "Span")
                        converted = Utils::String::Format("::SE::Span<{0}>({1}.Get(), {1}.Count())", nativeElementType, localName);
                    else
                        converted = localName;
                }
            }
            else
            {
                std::string value = NeedsInteropPointer(param)
                    ? Utils::String::Format("*{0}", param.name)
                    : param.name;
                converted = GetManagedToNativeConvert(TypeID(paramCppType), value);
            }
            args += converted;
        }
        return args;
    }

    void BindingsCppGenerator::GenerateCollectionReturn(const TypeInfoFunc& fn, const CollectionAbiInfo& collection,
                                                        const std::string& nativeExpression, std::string& output) const
    {
        const std::string nativeElementType = CodeGeneratorUtils::QualifyCppType(collection.elementCppType);
        const std::string interopElementType = GetInteropValueType(collection.elementCppType);
        const std::string managedElementType = GetCSharpFullTypeName(collection.elementCppType);
        const bool usesInteropStruct = IsApiInteropStructType(collection.elementCppType);

        output += Utils::String::Format("        const auto& __collectionValue = {0};\n", nativeExpression);
        if (collection.kind == CollectionAbiKind::Fixed)
            output += Utils::String::Format("        const int32 __collectionCount = {0};\n", collection.fixedElementCount);
        else
            output += Utils::String::Format("        const int32 __collectionCount = {0};\n",
                GetCollectionCountExpression(fn.returnType.ToString(), "__collectionValue"));
        if (collection.HasRuntimeCount())
            output += "        if (__returnCount != nullptr) *__returnCount = __collectionCount;\n";
        output += Utils::String::Format("        CLRClass* __elementClass = Scripting::FindClass(StringAnsiView(\"{0}\"));\n", managedElementType);
        output += "        if (__elementClass == nullptr) return nullptr;\n";

        if (!usesInteropStruct)
        {
            const std::string dataExpression = collection.kind == CollectionAbiKind::Fixed
                ? "__collectionValue" : GetCollectionDataExpression("__collectionValue");
            output += Utils::String::Format("        return CLRUtils::ToArray(::SE::Span<{0}>({1}, __collectionCount), __elementClass);\n",
                nativeElementType, dataExpression);
            return;
        }

        output += "        CLRArray* __result = CLRCore::Array::New(__elementClass, __collectionCount);\n";
        output += "        if (__result == nullptr || __collectionCount == 0) return __result;\n";
        output += Utils::String::Format("        auto* __resultItems = CLRCore::Array::GetAddress<{0}>(__result);\n", interopElementType);
        output += Utils::String::Format("        for (int32 i = 0; i < __collectionCount; ++i) __resultItems[i] = {0};\n",
            GetNativeToManagedConvert(TypeID(collection.elementCppType), "__collectionValue[i]"));
        output += "        return __result;\n";
    }

    bool BindingsCppGenerator::GenerateInteropHeader(const std::vector<BindingsHeaderInfo>& headers, std::string& output)
    {
        output.clear();
        output += "#pragma once\n";
        output += "//-------------------------------------------------------------------------\n";
        output += "// Auto-generated managed/native ABI bridge - do not edit manually.\n";
        output += "//-------------------------------------------------------------------------\n";

        std::vector<std::string> includes;
        std::vector<const TypeInfoStruct*> structs;
        std::vector<std::string> structNativeNames;
        for (auto const& header : headers)
        {
            bool hasGeneratedStruct = false;
            for (auto const& cls : header.classes)
            {
                if (cls->isStruct && cls->APIInBuildMapType.empty())
                {
                    const std::string nativeType = GetCppNativeTypeName(*cls);
                    if (Utils::Vector::Contains(structNativeNames, nativeType))
                    {
                        continue;
                    }

                    structs.push_back(cls);
                    structNativeNames.push_back(nativeType);
                    hasGeneratedStruct = true;
                }
            }
            if (hasGeneratedStruct && !Utils::Vector::Contains(includes, header.filePath))
                includes.push_back(header.filePath);
        }

        if (structs.empty())
            return true;

        for (auto const& include : includes)
            output += Utils::String::Format("#include \"{0}\"\n", include);
        output += "#include \"Runtime/Core/Scripting/ManagedCLR/CLRUtils.h\"\n\n";
        output += "namespace SE::BindingsInterop\n{\n";

        for (auto const* cls : structs)
        {
            if (cls->isPod)
            {
                continue;
            }

            const std::string nativeType = GetCppNativeTypeName(*cls);
            const std::string interopType = GetApiInteropStructCppType(nativeType);
            const int nameOffset = Utils::String::FindLast(interopType, ':');
            const std::string interopName = nameOffset == INVALID_INDEX ? interopType : interopType.substr(nameOffset + 1);

            output += Utils::String::Format("    struct {0}\n    {{\n", interopName);
            for (auto const& field : cls->fields)
            {
                if (field.isStatic)
                    continue;
                const std::string fieldCppType = field.type.ToString();
                const std::string fieldType = GetInteropValueType(fieldCppType);
                if (field.arraySize > 0)
                {
                    output += Utils::String::Format("        {0} {1}[{2}];\n", fieldType, field.name, field.arraySize);
                }
                else
                {
                    output += Utils::String::Format("        {0} {1};\n", fieldType, field.name);
                }
            }
            output += "    };\n\n";

            output += Utils::String::Format("    inline ::{0} ToNative(const {1}& value)\n    {{\n", nativeType, interopName);
            output += Utils::String::Format("        ::{0} result{{}};\n", nativeType);
            for (auto const& field : cls->fields)
            {
                if (field.isStatic) continue;
                if (field.arraySize > 0)
                {
                    output += Utils::String::Format("        for (int32 i = 0; i < {0}; ++i) result.{1}[i] = {2};\n",
                        field.arraySize, field.name, GetManagedToNativeConvert(TypeID(field.type.ToString()), Utils::String::Format("value.{0}[i]", field.name)));
                }
                else
                {
                    output += Utils::String::Format("        result.{0} = {1};\n", field.name,
                        GetManagedToNativeConvert(TypeID(field.type.ToString()), Utils::String::Format("value.{0}", field.name)));
                }
            }
            output += "        return result;\n    }\n\n";

            output += Utils::String::Format("    inline {0} ToManaged(const ::{1}& value)\n    {{\n", interopName, nativeType);
            output += Utils::String::Format("        {0} result{{}};\n", interopName);
            for (auto const& field : cls->fields)
            {
                if (field.isStatic)
                    continue;
                if (field.arraySize > 0)
                {
                    output += Utils::String::Format("        for (int32 i = 0; i < {0}; ++i) result.{1}[i] = {2};\n",
                        field.arraySize, field.name, GetNativeToManagedConvert(TypeID(field.type.ToString()), Utils::String::Format("value.{0}[i]", field.name)));
                }
                else
                {
                    output += Utils::String::Format("        result.{0} = {1};\n", field.name,
                        GetNativeToManagedConvert(TypeID(field.type.ToString()), Utils::String::Format("value.{0}", field.name)));
                }
            }
            output += "        return result;\n    }\n\n";
        }

        output += "}\n\n";

        output += "namespace SE\n{\n";
        for (auto const* cls : structs)
        {
            const std::string nativeType = GetCppNativeTypeName(*cls);
            output += "    template<>\n";
            output += Utils::String::Format("    struct CLRConverter<::{0}>\n    {{\n", nativeType);

            if (cls->isPod)
            {
                output += Utils::String::Format("        CLRObject* Box(const ::{0}& data, const CLRClass* klass)\n", nativeType);
                output += "        {\n";
                output += "            return CLRCore::Object::Box((void*)&data, klass);\n";
                output += "        }\n\n";
                output += Utils::String::Format("        void Unbox(::{0}& result, CLRObject* data)\n", nativeType);
                output += "        {\n";
                output += "            if (data)\n";
                output += Utils::String::Format("                Platform::MemoryCopy(&result, CLRCore::Object::Unbox(data), sizeof(::{0}));\n", nativeType);
                output += "        }\n\n";
                output += Utils::String::Format("        void ToManagedArray(CLRArray* result, const Span<::{0}>& data)\n", nativeType);
                output += "        {\n";
                output += Utils::String::Format("            Platform::MemoryCopy(CLRCore::Array::GetAddress(result), data.Get(), data.Length() * sizeof(::{0}));\n", nativeType);
                output += "        }\n\n";
                output += Utils::String::Format("        void ToNativeArray(Span<::{0}>& result, const CLRArray* data)\n", nativeType);
                output += "        {\n";
                output += Utils::String::Format("            Platform::MemoryCopy(result.Get(), CLRCore::Array::GetAddress(data), result.Length() * sizeof(::{0}));\n", nativeType);
                output += "        }\n";
            }
            else
            {
                const std::string interopType = GetApiInteropStructCppType(nativeType);
                const int nameOffset = Utils::String::FindLast(interopType, ':');
                const std::string interopName = nameOffset == INVALID_INDEX ? interopType : interopType.substr(nameOffset + 1);

                output += Utils::String::Format("        CLRObject* Box(const ::{0}& data, const CLRClass* klass)\n", nativeType);
                output += "        {\n";
                output += "            auto managed = BindingsInterop::ToManaged(data);\n";
                output += "            return CLRCore::Object::Box((void*)&managed, klass);\n";
                output += "        }\n\n";
                output += Utils::String::Format("        void Unbox(::{0}& result, CLRObject* data)\n", nativeType);
                output += "        {\n";
                output += "            if (data)\n";
                output += Utils::String::Format("                result = BindingsInterop::ToNative(*reinterpret_cast<BindingsInterop::{0}*>(CLRCore::Object::Unbox(data)));\n", interopName);
                output += "        }\n\n";
                output += Utils::String::Format("        void ToManagedArray(CLRArray* result, const Span<::{0}>& data)\n", nativeType);
                output += "        {\n";
                output += "            if (result == nullptr)\n";
                output += "                return;\n";
                output += Utils::String::Format("            auto* resultItems = CLRCore::Array::GetAddress<BindingsInterop::{0}>(result);\n", interopName);
                output += Utils::String::Format("            const CLRClass* elementClass = ::{0}::TypeInitializer.GetClass();\n", nativeType);
                output += "            for (int32 i = 0; i < data.Length(); ++i)\n";
                output += "            {\n";
                output += "                auto managed = BindingsInterop::ToManaged(data[i]);\n";
                output += "                CLRCore::GC::WriteValue(&resultItems[i], &managed, 1, elementClass);\n";
                output += "            }\n";
                output += "        }\n\n";
                output += Utils::String::Format("        void ToNativeArray(Span<::{0}>& result, const CLRArray* data)\n", nativeType);
                output += "        {\n";
                output += "            if (data == nullptr)\n";
                output += "                return;\n";
                output += Utils::String::Format("            auto* dataItems = CLRCore::Array::GetAddress<BindingsInterop::{0}>(data);\n", interopName);
                output += "            for (int32 i = 0; i < result.Length(); ++i)\n";
                output += "                result[i] = BindingsInterop::ToNative(dataItems[i]);\n";
                output += "        }\n";
            }

            output += "    };\n\n";
        }
        output += "}\n";
        return true;
    }

    // -------------------------------------------------------------------------
    // Wrapper function generation
    // -------------------------------------------------------------------------

    void BindingsCppGenerator::GenerateCppWrapperFunction(const TypeInfoStruct& cls,
                                                          const TypeInfoFunc&   fn,
                                                           BindingInvocationKind invocation,
                                                           std::string& bodyOut, std::string& endOut)
    {
        std::string const returnCppType = fn.returnType.ToString();
        const CollectionAbiInfo returnCollection = GetCollectionAbiInfo(returnCppType, fn.returnArraySize);
        const CollectionAbiInfo valueCollection = fn.params.empty() ? CollectionAbiInfo()
            : GetCollectionAbiInfo(fn.params[0].type.ToString(), fn.params[0].arraySize);

        std::string retType = GetInteropReturnType(fn);
        std::string params = BuildWrapperParams(cls, fn, true);

        const bool retIsVoid = returnCppType == "void";
        const bool useOutResult = !retIsVoid && ShouldUseOutResult(returnCppType);
        if (useOutResult)
        {
            if (!params.empty())
            {
                params += ", ";
            }
            params += Utils::String::Format("{0}* __resultAsRef", retType);
        }
        else if (returnCollection.HasRuntimeCount())
        {
            if (!params.empty())
                params += ", ";
            params += "int32* __returnCount";
        }
        std::string collectionSetup;
        std::string callArgs = BuildCallArgs(cls, fn, collectionSetup);
        std::string callExpr;

        std::string target;
        if (fn.isStatic)
        {
            std::string nativeName = GetCppNativeInvokeTypeName(cls);
            target = Utils::String::Format("::{0}::{1}", nativeName, fn.name);
        }
        else
        {
            target = Utils::String::Format("__obj->{0}", fn.name);
        }

        switch (invocation)
        {
        case BindingInvocationKind::FieldGet:
            callExpr = target;
            break;
        case BindingInvocationKind::FieldSet:
            callExpr = Utils::String::Format("{0} = {1}", target, callArgs);
            break;
        default:
            callExpr = Utils::String::Format("{0}({1})", target, callArgs);
            break;
        }

        std::string retConvert = GetNativeToManagedConvert(TypeID(returnCppType), callExpr);

        // MSVC exports the C++ helper under the flat C# entry-point name through
        // a linker alias. Other toolchains use the plain-C forwarding wrapper
        // emitted below. Both paths share the same ABI-safe signature.
        bodyOut += "#if defined(_MSC_VER)\n";
        bodyOut += Utils::String::Format("    DLLEXPORT static {0} {1}({2})\n", useOutResult ? "void" : retType, fn.uniqueName, params);
        bodyOut += "#else\n";
        bodyOut += Utils::String::Format("    static {0} {1}({2})\n", useOutResult ? "void" : retType, fn.uniqueName, params);
        bodyOut += "#endif\n";
        bodyOut += "    {\n";
        bodyOut += "#if defined(_MSC_VER)\n";
        bodyOut += Utils::String::Format("        MSVC_FUNC_EXPORT({0})\n", fn.entryPoint);
        bodyOut += "#endif\n";

        if (!fn.isStatic)
        {
            bodyOut += "        if (__obj == nullptr)\n        {\n";
            if (returnCollection.HasRuntimeCount())
                bodyOut += "            if (__returnCount != nullptr) *__returnCount = 0;\n";
            if (useOutResult)
            {
                bodyOut += "            if (__resultAsRef != nullptr) *__resultAsRef = {};\n";
            }
            bodyOut += "            return";
            if (!retIsVoid && !useOutResult) bodyOut += " {}";
            bodyOut += ";\n        }\n";
        }

        if (returnCollection.IsCollection())
        {
            GenerateCollectionReturn(fn, returnCollection, callExpr, bodyOut);
        }
        else if (invocation == BindingInvocationKind::FieldSet && valueCollection.kind == CollectionAbiKind::Fixed)
        {
            const std::string& valueName = fn.params[0].name;
            const std::string elementInteropType = GetInteropValueType(valueCollection.elementCppType);
            bodyOut += Utils::String::Format("        if ({0} == nullptr || CLRCore::Array::GetLength({0}) != {1}) return;\n",
                valueName, valueCollection.fixedElementCount);
            bodyOut += Utils::String::Format("        auto* __valueItems = CLRCore::Array::GetAddress<{0}>({1});\n", elementInteropType, valueName);
            bodyOut += Utils::String::Format("        for (int32 i = 0; i < {0}; ++i) {1}[i] = {2};\n", valueCollection.fixedElementCount,
                target, GetManagedToNativeConvert(TypeID(valueCollection.elementCppType), "__valueItems[i]"));
        }
        else if (retIsVoid)
        {
            bodyOut += collectionSetup;
            bodyOut += Utils::String::Format("        {0};\n", retConvert);
        }
        else if (useOutResult)
        {
            bodyOut += collectionSetup;
            bodyOut += Utils::String::Format("        *__resultAsRef = {0};\n", retConvert);
        }
        else
        {
            bodyOut += collectionSetup;
            bodyOut += Utils::String::Format("        return {0};\n", retConvert);
        }
        bodyOut += "    }\n";

        std::string exportParams;
        if (!fn.isStatic)
        {
            exportParams += "void* __obj";
        }
        for (int i = 0; i < fn.params.size(); ++i)
        {
            if (exportParams.length() > 0) exportParams += ", ";
            const CollectionAbiInfo collection = GetCollectionAbiInfo(fn.params[i].type.ToString(), fn.params[i].arraySize);
            if (collection.IsCollection())
            {
                exportParams += Utils::String::Format("CLRArray* {0}", fn.params[i].name);
                if (collection.HasRuntimeCount())
                    exportParams += Utils::String::Format(", int32 __{0}Count", fn.params[i].name);
            }
            else
            {
                exportParams += Utils::String::Format("{0}{1} {2}", GetInteropParamType(fn.params[i]),
                    NeedsInteropPointer(fn.params[i]) ? "*" : "", fn.params[i].name);
            }
        }
        if (useOutResult)
        {
            if (!exportParams.empty()) exportParams += ", ";
            exportParams += Utils::String::Format("{0}* __resultAsRef", retType);
        }
        else if (returnCollection.HasRuntimeCount())
        {
            if (!exportParams.empty()) exportParams += ", ";
            exportParams += "int32* __returnCount";
        }

        std::string forwardArgs = BuildForwardArgs(fn);
        if (useOutResult)
        {
            if (!forwardArgs.empty()) forwardArgs += ", ";
            forwardArgs += "__resultAsRef";
        }
        else if (returnCollection.HasRuntimeCount())
        {
            if (!forwardArgs.empty()) forwardArgs += ", ";
            forwardArgs += "__returnCount";
        }

        endOut += "#if !defined(_MSC_VER)\n";
        endOut += Utils::String::Format("DEFINE_INTERNAL_CALL({0}) {1}_{2}({3})\n", useOutResult ? "void" : retType, cls.name, fn.uniqueName, exportParams);
        endOut += "{\n";
        if (!fn.isStatic)
        {
            std::string nativeTypeName = GetCppNativeTypeName(cls);
            std::string internalName = CodeGeneratorUtils::GetInternalClassName(cls.name);
            std::string castExpr = Utils::String::Format("return {0}::{1}((::{2}*)__obj{3}{4})",
                internalName, fn.uniqueName, nativeTypeName,
                !forwardArgs.empty() ? ", " : "", forwardArgs);
            endOut += Utils::String::Format("    {0};\n", (retIsVoid || useOutResult) ?
                Utils::String::Format("{0}::{1}((::{2}*)__obj{3}{4})",
                    internalName, fn.uniqueName, nativeTypeName,
                    !forwardArgs.empty() ? ", " : "",
                    forwardArgs) :
                castExpr);
        }
        else
        {
            endOut += Utils::String::Format("    {0}{1}Internal::{2}({3});\n", (retIsVoid || useOutResult) ? "" : "return ",
                cls.name, fn.uniqueName, forwardArgs);
        }
        endOut += "}\n";
        endOut += "#endif\n";
    }

    void BindingsCppGenerator::GenerateCppPropertyAccessors(const TypeInfoStruct& cls,
                                                            const TypeInfoFunc&    prop,
                                                            std::vector<bool>&     consumedFunctions,
                                                            int                    functionIndex,
                                                            std::string&           bodyOut,
                                                            std::string&           endOut)
    {
        BindingCallable getter;
        BindingCallable setter;
        const BindingCallable* getterPtr = nullptr;
        const BindingCallable* setterPtr = nullptr;

        for (int i = functionIndex; i < cls.functions.size(); ++i)
        {
            TypeInfoFunc const& fn = cls.functions[i];
            if (consumedFunctions[i] || !fn.APIIsPropertie || GetPropertyName(fn) != GetPropertyName(prop))
            {
                continue;
            }

            consumedFunctions[i] = true;
            if (fn.returnType == TypeID("void") && fn.params.size() == 1)
            {
                setter = MakeBindingPropertySetter(cls, fn);
                setterPtr = &setter;
            }
            else if (fn.returnType != TypeID("void"))
            {
                getter = MakeBindingPropertyGetter(cls, fn);
                getterPtr = &getter;
            }
        }

        if (getterPtr != nullptr)
        {
            GenerateCppWrapperFunction(cls, getterPtr->function, getterPtr->invocation, bodyOut, endOut);
        }

        if (setterPtr != nullptr)
        {
            GenerateCppWrapperFunction(cls, setterPtr->function, setterPtr->invocation, bodyOut, endOut);
        }
    }

    void BindingsCppGenerator::GenerateCppEventWrappers(const TypeInfoStruct& cls, const TypeInfoEvent& evt,
                                                         const std::string& assemblyType, std::string& bodyOut, std::string& endOut)
    {
        std::string fullType = GetCppNativeTypeName(cls);
        std::string internalName = CodeGeneratorUtils::GetInternalClassName(cls.name);

        // Build parameter type list for the event callback
        std::string paramTypes;
        for (int i = 0; i < evt.params.size(); ++i)
        {
            if (i > 0) paramTypes += ", ";

            std::string cppType = evt.params[i].type.ToString();
            size_t      index   = cppType.find_first_of("SE");
            if (index != std::string::npos)
            {
                cppType.insert(index, "::");
            }

            paramTypes += Utils::String::Format("{0} {1}", cppType, evt.params[i].name);
        }

        // Managed wrapper - C++ calls C# delegate
        bodyOut += Utils::String::Format("    {0}void {1}_ManagedWrapper({2})\n", evt.isStatic ? "static " : "", evt.name, paramTypes);
        bodyOut += "    {\n";
        bodyOut += "        static CLRMethod* method = nullptr;\n";
        if (evt.isStatic)
        {
            const std::string managedType = CodeGeneratorUtils::GetFullCSTypeName(cls.namespaceScopeList, cls.name);
            bodyOut += Utils::String::Format("        if (!method)\n        {{\n            CLRClass* managedClass = ((ManagedBinaryModule*)GetBinaryModule{0}())->Assembly->GetClass(\"{1}\");\n", assemblyType, managedType);
            bodyOut += Utils::String::Format("            method = managedClass ? managedClass->GetMethod(\"Internal_{0}_Invoke\", {1}) : nullptr; ASSERT(method); \n", evt.name, evt.params.size());
            bodyOut += "        }";
        }
        else
        {
            bodyOut += Utils::String::Format("        if (!method)\n        {{\n            method = ::{0}::TypeInitializer->GetType().ManagedClass->GetMethod(\"Internal_{1}_Invoke\", {2}); ASSERT(method); }}\n",
                fullType, evt.name, evt.params.size());
        }
        bodyOut += "\n        CLRObject* exception = nullptr;\n";
        if (evt.params.size() > 0)
        {
            bodyOut += Utils::String::Format("        void* params[{0}];\n", evt.params.size());
            for (int i = 0; i < evt.params.size(); ++i)
            {
                // CLR invocation expects an address to each interop argument.
                // Event callback parameters are native values/references, so
                // taking their address also preserves pointer and Guid values.
                bodyOut += Utils::String::Format("        params[{0}] = (void*)&{1};\n", i, evt.params[i].name);
            }
        }
        if (evt.isStatic)
        {
            bodyOut += Utils::String::Format("        method->Invoke(nullptr, {0}, &exception);\n", evt.params.empty() ? "nullptr" : "params");
        }
        else
        {
            bodyOut += Utils::String::Format("        CLRObject* instance = ((::{0}*)this)->GetManagedInstance();\n", fullType);
            bodyOut += Utils::String::Format("        method->Invoke(instance, {0}, &exception);\n", evt.params.empty() ? "nullptr" : "params");
        }
        // bodyOut += "        if (exception) DebugLog::LogException(exception);\n";
        bodyOut += "    }\n\n";

        // Managed bind/unbind
        std::string bindTarget = Utils::String::Format("&{0}::{1}_ManagedWrapper", internalName, evt.name);
        if (evt.isStatic)
            bodyOut += Utils::String::Format("    static void {0}_ManagedBind(bool bind)\n", evt.name);
        else
            bodyOut += Utils::String::Format("    static void {0}_ManagedBind(::{1}* __obj, bool bind)\n", evt.name, fullType);
        bodyOut += "    {\n";
        if (!evt.isStatic)
            bodyOut += "        if (__obj == nullptr) return;\n";
        bodyOut += Utils::String::Format("        Function<void({0})> f;\n", paramTypes);
        if (evt.isStatic)
        {
            bodyOut += Utils::String::Format("        f.Bind<{0}>();\n", bindTarget);
            bodyOut += Utils::String::Format("        if (bind) ::{0}::{1}.Bind(f);\n", fullType, evt.name);
            bodyOut += Utils::String::Format("        else ::{0}::{1}.Unbind(f);\n", fullType, evt.name);
        }
        else
        {
            bodyOut += Utils::String::Format("        f.Bind<{0}, {1}>(({0}*)__obj);\n", internalName, bindTarget);
            bodyOut += Utils::String::Format("        if (bind) __obj->{0}.Bind(f);\n", evt.name);
            bodyOut += Utils::String::Format("        else __obj->{0}.Unbind(f);\n", evt.name);
        }
        bodyOut += "    }\n\n";

        if (evt.isStatic)
        {
            endOut += Utils::String::Format("DEFINE_INTERNAL_CALL(void) {0}_{1}_ManagedBind(bool bind)\n", cls.name, evt.name);
            endOut += "{\n";
            endOut += Utils::String::Format("    {0}::{1}_ManagedBind(bind);\n", internalName, evt.name);
            endOut += "}\n";
            return;
        }

        endOut += Utils::String::Format("DEFINE_INTERNAL_CALL(void) {0}_{1}_ManagedBind(void* __obj, bool bind)\n", cls.name, evt.name);
        endOut += "{\n";
        endOut += Utils::String::Format("    {0}::{1}_ManagedBind((::{2}*)__obj, bind);\n", internalName, evt.name, fullType);
        endOut += "}\n";

        // Generic scripting event wrapper (Variant-based)
        bodyOut += Utils::String::Format("    void {0}_Wrapper({1})\n", evt.name, paramTypes);
        bodyOut += "    {\n";
        if (evt.params.size() > 0)
        {
            bodyOut += Utils::String::Format("        Variant parameters[{0}];\n", evt.params.size());
            for (int i = 0; i < evt.params.size(); ++i)
            {
                std::string convertExpr = GetNativeToVariantConvert(TypeID(evt.params[i].type.ToString()), evt.params[i].name);
                bodyOut += Utils::String::Format("        parameters[{0}] = {1};\n", i, convertExpr);
            }
            bodyOut += Utils::String::Format("        ScriptingEvents::Event((ScriptingObject*)this, Span<Variant>(parameters, {0}), ::{1}::TypeInitializer, StringView(SE_TEXT(\"{2}\")));\n",
                evt.params.size(), fullType, evt.name);
        }
        else
        {
            bodyOut += Utils::String::Format("        ScriptingEvents::Event((ScriptingObject*)this, Span<Variant>(), ::{0}::TypeInitializer, StringView(SE_TEXT(\"{1}\")));\n",
                fullType, evt.name);
        }
        bodyOut += "    }\n\n";

        // Generic scripting bind/unbind
        bodyOut += Utils::String::Format("    static void {0}_Bind(::{1}* __obj, void* instance, bool bind)\n", evt.name, fullType);
        bodyOut += "    {\n";
        bodyOut += Utils::String::Format("        Function<void({0})> f;\n", paramTypes);
        bodyOut += Utils::String::Format("        f.Bind<{0}, &{0}::{1}_Wrapper>(({0}*)instance);\n", internalName, evt.name);
        bodyOut += Utils::String::Format("        if (bind) __obj->{0}.Bind(f);\n", evt.name);
        bodyOut += Utils::String::Format("        else __obj->{0}.Unbind(f);\n", evt.name);
        bodyOut += "    }\n";
    }

    void BindingsCppGenerator::GenerateCppFieldAccessors(const TypeInfoStruct& cls, const TypeInfoField& field,
                                                           std::string& bodyOut, std::string& endOut)
    {
        BindingCallable getter = MakeBindingFieldGetter(cls, field);
        GenerateCppWrapperFunction(cls, getter.function, getter.invocation, bodyOut, endOut);
        if (!field.APIIsReadOnly)
        {
            BindingCallable setter = MakeBindingFieldSetter(cls, field);
            GenerateCppWrapperFunction(cls, setter.function, setter.invocation, bodyOut, endOut);
        }
    }

    void BindingsCppGenerator::GenerateCppInitRuntime(const TypeInfoStruct& cls, std::string& output)
    {
        output += "    static void InitRuntime()\n    {\n";

        // Register events in ScriptingEvents table
        for (auto& evt : cls.events)
        {
            std::string fullType = GetCppNativeTypeName(cls);
            output += Utils::String::Format(
                "        ScriptingEvents::EventsTable[Pair<ScriptingTypeHandle, StringView>({0}::TypeInitializer, StringView(SE_TEXT(\"{1}\")))] = (void(*)(ScriptingObject*, void*, bool)){2}Internal::{1}_ManagedWrapper;\n",
                CodeGeneratorUtils::RemovePreNameSpace(fullType), evt.name, cls.name);
        }

        output += "    }\n";
    }

    // -------------------------------------------------------------------------
    // Class generation
    // -------------------------------------------------------------------------

    void BindingsCppGenerator::GenerateCppClass(const TypeInfoStruct& cls,
                                                const std::string&    assemblyType,
                                                std::string&          output)
    {
        std::string fullNativeName = CodeGeneratorUtils::GetNativeName(cls.namespaceScopeList, cls.structScopeList, cls.name);
        std::string fullCSharpTypename = CodeGeneratorUtils::GetFullCSTypeName(cls.namespaceScopeList, cls.APIName.empty() ? cls.name : cls.APIName);
        std::string internalName = CodeGeneratorUtils::GetInternalClassName(cls.name);
        std::string bodyOut, endOut;
        bool useScripting = cls.isScriptingObject || cls.APIIsStatic;

        // Internal class header
        if (!cls.namespaceScopeList.empty())
        {
            output += Utils::String::Format("namespace {0}\n{{\n", CodeGeneratorUtils::GetFullCNameSpaceName(cls.namespaceScopeList));
        }

        output += Utils::String::Format("class {0}\n{{\npublic:\n", internalName);

        // Instance events also register with ScriptingEvents; static API events
        // still need the direct C++ -> C# callback bridge.
        if (useScripting || cls.APIIsStatic)
        {
            for (auto& evt : cls.events)
            {
                GenerateCppEventWrappers(cls, evt, assemblyType, bodyOut, endOut);
            }
        }

        // Function, property and field exports also apply to native-handle API classes.
        for (auto& field : cls.fields)
        {
            GenerateCppFieldAccessors(cls, field, bodyOut, endOut);
        }

        std::vector<bool> consumedFunctions(cls.functions.size(), false);
        for (int i = 0; i < cls.functions.size(); ++i)
        {
            TypeInfoFunc const& fn = cls.functions[i];
            if (fn.APIIsPropertie)
            {
                if (!consumedFunctions[i])
                {
                    GenerateCppPropertyAccessors(cls, fn, consumedFunctions, i, bodyOut, endOut);
                }
                continue;
            }

            if (fn.APINoProxy)
            {
                continue;
            }
            GenerateCppWrapperFunction(cls, fn, BindingInvocationKind::Method, bodyOut, endOut);
        }

        if (useScripting)
        {
            GenerateCppInitRuntime(cls, bodyOut);
        }

        output += bodyOut;
        output += "};\n\n";

        if (!useScripting)
        {
            if (!endOut.empty())
            {
                output += Utils::String::Format("\n// Plain-C exports\n{0}", endOut);
            }

            if (!cls.namespaceScopeList.empty())
            {
                output += "}\n";
            }

            output += "\n";
            return;
        }

        // Interface inheritance table
        if (!cls.interfaces.empty())
        {
            output += Utils::String::Format("static const ScriptingType::InterfaceImplementation {0}_Interfaces[] = {{\n", fullNativeName);
            for (auto& iface : cls.interfaces)
            {
                std::string ifaceNativeName = iface->APIName.empty() ? iface->name : iface->APIName;
                std::string ifaceFull = CodeGeneratorUtils::GetFullCTypeName(iface->namespaceScopeList, ifaceNativeName);
                output += Utils::String::Format("    {{ &{0}::TypeInitializer, (int16)VTABLE_OFFSET({1}, {0}), 0, true }},\n", ifaceFull, CodeGeneratorUtils::RemovePreNameSpace(fullNativeName));
            }
            output += "    { nullptr, 0 },\n};\n\n";
        }

        // ScriptingTypeInitializer
        if (cls.isTemplateInstantiation)
        {
            output += "template<>\n";
        }
        output += Utils::String::Format("ScriptingTypeInitializer {0}::TypeInitializer(\n", CodeGeneratorUtils::RemovePreNameSpace(fullNativeName));
        output += Utils::String::Format("    (BinaryModule*)GetBinaryModule{0}(),\n", assemblyType);
        output += Utils::String::Format("    StringAnsiView(\"{0}\", ARRAY_SIZE(\"{0}\") - 1),\n", fullCSharpTypename);
        output += Utils::String::Format("    sizeof(::{0}),\n", fullNativeName);
        output += Utils::String::Format("    &{0}::InitRuntime,\n", internalName);

        if (useScripting)
        {
            // ScriptingObject path: spawn, baseType, vtable, vtable, interfaces
            if (cls.APIIsStatic || cls.APINoSpawn)
            {
                output += "    &ScriptingType::DefaultSpawn, \n";
            }
            else
            {
                output += Utils::String::Format("    (ScriptingType::SpawnHandler)&::{0}::Spawn,\n", fullNativeName);
            }

            if (!cls.baseClassName.empty())
            {
                output += Utils::String::Format("    &::{0}::TypeInitializer,\n", cls.baseClassName);
            }
            else
            {
                output += "    nullptr,\n";
            }

            output += "    nullptr,\n    nullptr";
            if (!cls.interfaces.empty())
            {
                output += Utils::String::Format(",\n    ::{0}_Interfaces", fullNativeName);
            }
            output += "\n);\n";
        }
        else
        {
            // Non-scripting class path: ctor, dtor, baseType, interfaces
            if (!cls.APIIsAbstract)
            {
                output += Utils::String::Format("    &{0}::Ctor, &{0}::Dtor,\n", internalName);
            }
            else
            {
                output += "    nullptr, nullptr,\n";
            }

            if (!cls.baseClassName.empty())
            {
                output += Utils::String::Format("    &::{0}::TypeInitializer", cls.baseClassName);
            }
            else
            {
                output += "    nullptr";
            }

            if (!cls.interfaces.empty())
            {
                output += Utils::String::Format(",\n    ::{0}_Interfaces", fullNativeName);
            }
            output += "\n);\n";
        }

        // Plain-C exports
        if (!endOut.empty())
        {
            output += Utils::String::Format("\n// Plain-C exports\n{0}", endOut);
        }

        if (!cls.namespaceScopeList.empty())
        {
            output += "}\n";
        }

        output += "\n";
    }

    // -------------------------------------------------------------------------
    // Struct generation (independent, not delegating to GenerateCppClass)
    // -------------------------------------------------------------------------

    void BindingsCppGenerator::GenerateCppStruct(const TypeInfoStruct& cls,
                                                 const std::string& assemblyType,
                                                 std::string& output)
    {
        std::string fullNativeName = CodeGeneratorUtils::GetNativeName(cls.namespaceScopeList, cls.structScopeList, cls.name);
        std::string fullCSharpTypename = CodeGeneratorUtils::GetFullCSTypeName(cls.namespaceScopeList, cls.name);
        std::string internalName = CodeGeneratorUtils::GetInternalClassName(cls.name);
        std::string bodyOut, endOut;

        // Internal class header
        if (!cls.namespaceScopeList.empty())
        {
            output += Utils::String::Format("namespace {0}\n{{\n", CodeGeneratorUtils::GetFullCSNameSpaceName(cls.namespaceScopeList));
        }

        output += Utils::String::Format("class {0}\n{{\npublic:\n", internalName);

        // Struct instance fields are represented directly by the generated C#
        // layout. Only static fields need a native ABI wrapper.
        for (auto& field : cls.fields)
        {
            if (field.isStatic)
            {
                GenerateCppFieldAccessors(cls, field, bodyOut, endOut);
            }
        }

        // Value types register lifecycle callbacks just like Flax structs. The
        // generated CLRConverter specialization in BindingsInterop.h handles
        // POD raw-copy and non-POD wrapper-layout conversions.
        bodyOut += "    static void InitRuntime()\n    {\n    }\n\n";

        bodyOut += Utils::String::Format("    static void Ctor(void* ptr)\n    {{\n        new(ptr)::{0}();\n    }}\n\n", fullNativeName);
        bodyOut += Utils::String::Format("    static void Dtor(void* ptr)\n    {{\n        ((::{0}*)ptr)->~{1}();\n    }}\n\n", fullNativeName, cls.name);
        bodyOut += Utils::String::Format("    static void Copy(void* dst, void* src)\n    {{\n        *(::{0}*)dst = *(::{0}*)src;\n    }}\n\n", fullNativeName);
        bodyOut += Utils::String::Format("    static CLRObject* Box(void* ptr)\n    {{\n        return ::SE::CLRUtils::Box(*static_cast<::{0}*>(ptr), ::{0}::TypeInitializer.GetClass());\n    }}\n\n", fullNativeName);
        bodyOut += Utils::String::Format("    static void Unbox(void* ptr, CLRObject* managed)\n    {{\n        *static_cast<::{0}*>(ptr) = ::SE::CLRUtils::Unbox<::{0}>(managed);\n    }}\n\n", fullNativeName);

        bodyOut += "    static void GetField(void* ptr, const String& name, Variant& value)\n    {\n";
        for (int i = 0, count = 0; i < cls.fields.size(); ++i)
        {
            TypeInfoField const& field = cls.fields[i];
            if (field.isStatic || !CanGenerateVariantFieldAccess(field.type))
                continue;

            bodyOut += Utils::String::Format("        {0}if (name == SE_TEXT(\"{1}\"))\n", count == 0 ? "" : "else ", field.name);
            bodyOut += "        {\n";
            if (field.arraySize > 0)
            {
                bodyOut += "            List<Variant, HeapAllocation> __values;\n";
                bodyOut += Utils::String::Format("            __values.Resize({0});\n", field.arraySize);
                bodyOut += Utils::String::Format("            for (int32 i = 0; i < {0}; ++i)\n", field.arraySize);
                bodyOut += Utils::String::Format("                __values[i] = {0};\n",
                    GetNativeToVariantConvert(TypeID(field.type.ToString()), Utils::String::Format("((::{0}*)ptr)->{1}[i]", fullNativeName, field.name)));
                bodyOut += "            value = Variant(__values);\n";
            }
            else
            {
                bodyOut += Utils::String::Format("            value = {0};\n",
                    GetNativeToVariantConvert(TypeID(field.type.ToString()), Utils::String::Format("((::{0}*)ptr)->{1}", fullNativeName, field.name)));
            }
            bodyOut += "        }\n";
            ++count;
        }
        bodyOut += "    }\n\n";

        bodyOut += "    static void SetField(void* ptr, const String& name, const Variant& value)\n    {\n";
        for (int i = 0, count = 0; i < cls.fields.size(); ++i)
        {
            TypeInfoField const& field = cls.fields[i];
            if (field.isStatic || field.APIIsReadOnly || !CanGenerateVariantFieldAccess(field.type))
                continue;

            bodyOut += Utils::String::Format("        {0}if (name == SE_TEXT(\"{1}\"))\n", count == 0 ? "" : "else ", field.name);
            bodyOut += "        {\n";
            if (field.arraySize > 0)
            {
                bodyOut += "            if (value.Type != VariantTypes::Array)\n";
                bodyOut += "                return;\n";
                bodyOut += "            const auto& __values = value.AsArray();\n";
                bodyOut += Utils::String::Format("            const int32 __count = __values.Count() < {0} ? __values.Count() : {0};\n", field.arraySize);
                bodyOut += "            for (int32 i = 0; i < __count; ++i)\n";
                bodyOut += Utils::String::Format("                ((::{0}*)ptr)->{1}[i] = {2};\n",
                    fullNativeName, field.name, GetVariantToNativeConvert(TypeID(field.type.ToString()), "__values[i]"));
            }
            else
            {
                bodyOut += Utils::String::Format("            ((::{0}*)ptr)->{1} = {2};\n",
                    fullNativeName, field.name, GetVariantToNativeConvert(TypeID(field.type.ToString()), "value"));
            }
            bodyOut += "        }\n";
            ++count;
        }
        bodyOut += "    }\n";

        output += bodyOut;
        output += "};\n\n";

        if (cls.isTemplateInstantiation)
        {
            output += "template<>\n";
        }
        output += Utils::String::Format("ScriptingTypeInitializer {0}::TypeInitializer(\n", CodeGeneratorUtils::RemovePreNameSpace(fullNativeName));
        output += Utils::String::Format("    (BinaryModule*)GetBinaryModule{0}(),\n", assemblyType);
        output += Utils::String::Format("    StringAnsiView(\"{0}\", ARRAY_SIZE(\"{0}\") - 1),\n", fullCSharpTypename);
        output += Utils::String::Format("    sizeof(::{0}),\n", fullNativeName);
        output += Utils::String::Format("    &{0}::InitRuntime,\n", internalName);
        output += Utils::String::Format("    &{0}::Ctor, &{0}::Dtor, &{0}::Copy,\n", internalName);
        output += Utils::String::Format("    &{0}::Box, &{0}::Unbox, &{0}::GetField, &{0}::SetField", internalName);
        if (!cls.baseClassName.empty())
        {
            output += Utils::String::Format(",\n    &::{0}::TypeInitializer", cls.baseClassName);
        }
        else
        {
            output += ",\n    nullptr";
        }
        output += "\n);\n";

        // Plain-C exports
        if (!endOut.empty())
        {
            output += Utils::String::Format("\n// Plain-C exports\n{0}", endOut);
        }

        if (!cls.namespaceScopeList.empty())
        {
            output += "}\n";
        }

        output += "\n";
    }

    // -------------------------------------------------------------------------
    // Enum generation
    // -------------------------------------------------------------------------

    void BindingsCppGenerator::GenerateCppEnum(const TypeInfoEnum& en, const std::string& assemblyType, std::string& output)
    {
        std::string fullNameName = CodeGeneratorUtils::GetNativeName(en.namespaceScopeList, en.structScopeList , en.name);
        std::string namespaceName = CodeGeneratorUtils::GetFullCSNameSpaceName(en.namespaceScopeList);

        std::string InternalNativeName = en.name;
        if (!en.structScopeList.empty())
        {
            InternalNativeName = Utils::String::Format("{0}_{1}", Utils::CombineStringList(en.structScopeList, "_"), en.name);
        }


        if (!en.namespaceScopeList.empty())
        {
            output += Utils::String::Format("namespace {0}\n{{\n", namespaceName);
        }

        output += Utils::String::Format("class {0}Internal\n{{\npublic:\n", InternalNativeName);
        output += "    static ScriptingType::EnumItem Items[];\n";
        output += "};\n\n";

        // Items array
        output += Utils::String::Format("ScriptingType::EnumItem {0}Internal::Items[] = {{\n", InternalNativeName);
        for (int i = 0; i < en.enumConstants.size(); ++i)
        {
            const EnumDataConstant& enumData = en.enumConstants[i];
            output += Utils::String::Format("    {{ (uint64)::{0}::{1}, \"{2}\" }},\n", fullNameName, enumData.label, enumData.label);
        }
        output += "    { 0, nullptr }\n};\n\n";

        // ScriptingTypeInitializer
        output += Utils::String::Format("inline ScriptingTypeInitializer {0}_TypeInitializer(\n", en.name);
        output += Utils::String::Format("    (BinaryModule*)GetBinaryModule{0}(),\n", assemblyType);
        output += Utils::String::Format("    StringAnsiView(\"{0}\", ARRAY_SIZE(\"{0}\") - 1),\n", fullNameName);
        output += Utils::String::Format("    sizeof(::{0}),\n", fullNameName);
        output += Utils::String::Format("    StableID::Generate<::{0}>(),\n", fullNameName);
        output += Utils::String::Format("    {0}Internal::Items\n", InternalNativeName);
        output += ");\n\n";

        if (!namespaceName.empty())
        {
            output += "}\n";
        }
    }

    // -------------------------------------------------------------------------
    // Interface generation
    // -------------------------------------------------------------------------

    void BindingsCppGenerator::GenerateCppInterface(const TypeInfoStruct& iface,
                                                    const std::string&    assemblyType,
                                                     std::string& output)
    {
        std::string fullname = CodeGeneratorUtils::GetNativeName(
            iface.namespaceScopeList, {}, iface.APIName.empty() ? iface.name : iface.APIName);
        std::string fullTypename = CodeGeneratorUtils::GetFullCSTypeName(iface.namespaceScopeList, iface.name);
        std::string internalName = CodeGeneratorUtils::GetInternalClassName(iface.name);

/*        std::string nativeName = iface.nativeName.empty() ? iface.name : iface.nativeName;
        std::string fullType = CodeGeneratorUtils::GetFullCTypeName(iface.namespaceScopeList, nativeName);
        std::string fullTypename = CodeGeneratorUtils::GetFullCSTypeName(iface.namespaceScopeList, iface.name);*/

        if (!iface.namespaceScopeList.empty())
        {
            output += Utils::String::Format("namespace {0}\n{{\n",
                                            CodeGeneratorUtils::GetFullCSNameSpaceName(iface.namespaceScopeList));
        }

        // Wrapper class
        output += Utils::String::Format("class {0}Wrapper : public {1}\n{{\npublic:\n", iface.name, iface.name);
        output += "    ScriptingObject* Object;\n";

        for (auto& fn : iface.functions)
        {
            std::string paramTypes;
            for (int i = 0; i < fn.params.size(); ++i)
            {
                if (i > 0) paramTypes += ", ";
                paramTypes += fn.params[i].type.ToString();
            }

            std::string const returnType = fn.returnType.ToString();
            output += Utils::String::Format("    {0} {1}({2}) const override\n", returnType, fn.name, paramTypes);
            output += "    {\n";
            if (fn.params.size() > 0)
            {
                output += Utils::String::Format("        Variant parameters[{0}];\n", fn.params.size());
                for (int i = 0; i < fn.params.size(); ++i)
                {
                    std::string convertExpr =
                        GetNativeToVariantConvert(TypeID(fn.params[i].type.ToString()), fn.params[i].name);
                    output += Utils::String::Format("        parameters[{0}] = {1};\n", i, convertExpr);
                }
            }

            output +=                   "        auto typeHandle = Object->GetTypeHandle();\n";
            output +=                   "        while (typeHandle)\n        {\n";
            output += Utils::String::Format("            auto method = typeHandle.Module->FindMethod(typeHandle, StringAnsiView(\"{0}\", ARRAY_SIZE(\"{0}\") - 1), {1});\n",
                fn.name, fn.params.size());
            output +=                   "            if (method)\n            {\n";
            output +=                   "                Variant __result;\n";
            if (fn.params.size() > 0)
            {
                output += Utils::String::Format("                typeHandle.Module->InvokeMethod(method, Object, Span<Variant>(parameters, {0}), __result);\n", fn.params.size());
            }
            else
            {
                output +=               "                typeHandle.Module->InvokeMethod(method, Object, Span<Variant>(), __result);\n";
            }
            if (returnType != "void")
            {
                output += Utils::String::Format("                return ({0})__result;\n", returnType);
            }
            else
            {
                output += "                return;\n";
            }
            output += "            }\n";
            output += "            typeHandle = typeHandle.GetType().GetBaseType();\n";
            output += "        }\n";
            if (returnType != "void")
            {
                output += "        return {};\n"; // default return
            }
            output += "    }\n";
        }
        output += "};\n\n";

        // Internal class
        output += Utils::String::Format("class {0}\n{{\npublic:\n", internalName);
        output += Utils::String::Format("    static void InitRuntime() {{ }}\n");
        output += Utils::String::Format("    static void* GetInterfaceWrapper(ScriptingObject* __obj)\n    {{\n");
        output += Utils::String::Format("        auto wrapper = New<{0}Wrapper>();\n", iface.name);
        output += "        wrapper->Object = __obj;\n";
        output += "        return wrapper;\n";
        output += "    }\n";
        output += "};\n\n";

        // ScriptingTypeInitializer
        if (iface.isTemplateInstantiation)
        {
            output += "template<>\n";
        }
        output += Utils::String::Format("ScriptingTypeInitializer {0}::TypeInitializer(\n",
                                        CodeGeneratorUtils::RemovePreNameSpace(fullname));
        output += Utils::String::Format("    (BinaryModule*)GetBinaryModule{0}(),\n", assemblyType);
        output += Utils::String::Format("    StringAnsiView(\"{0}\", ARRAY_SIZE(\"{0}\") - 1),\n", fullTypename);
        output += Utils::String::Format("    &{0}::InitRuntime,\n", internalName);
        output += "    nullptr,\n    nullptr,\n";
        output += Utils::String::Format("    &{0}::GetInterfaceWrapper\n", internalName);
        output += ");\n";

        if (!iface.namespaceScopeList.empty())
        {
            output += "}\n";
        }
    }

    // -------------------------------------------------------------------------
    // Generate - entry point for a single header
    // -------------------------------------------------------------------------

    bool BindingsCppGenerator::GenerateSource(const BindingsHeaderInfo& headerInfo, std::string& output)
    {
        output.clear();
        m_errorMessage.clear();

        if (headerInfo.classes.empty() && headerInfo.enums.empty() && headerInfo.interfaces.empty() && headerInfo.events.empty())
        {
            return true;
        }

        bool hasCppInjectedCode = false;
        for (auto const& code : headerInfo.injectedCode)
        {
            if (code->lang == InjectEnum::CPP)
            {
                hasCppInjectedCode = true;
                break;
            }
        }

        // Derive assemblyType
        std::string assemblyType = CodeGeneratorUtils::DeriveAssemblyCSharpType(headerInfo.assemblyName);

        output += "//-------------------------------------------------------------------------\n";
        output += "// Auto-generated by BindingsGenerator - do not edit manually.\n";
        output += Utils::String::Format("// Source: {0}\n", headerInfo.filePath);
        output += "//-------------------------------------------------------------------------\n";
        output += Utils::String::Format("#include \"{0}\"\n", headerInfo.filePath);
        if (!headerInfo.assemblyDir.empty())
        {
            std::string interopHeader = Utils::String::Format("{0}/{1}/BindingsInterop.h", headerInfo.assemblyDir,
                Settings::g_autogeneratedDirectory);
            FileSystem::NormalizePath(interopHeader);
            output += Utils::String::Format("#include \"{0}\"\n", interopHeader);
        }
        output += "#include \"Runtime/Core/Scripting/ManagedCLR/CLRUtils.h\"\n";
        output += "#include \"Runtime/Core/Scripting/Scripting.h\"\n";
        output += "#include \"Runtime/Core/Scripting/Binary/ManagedBinaryModule.h\"\n";
        output += "#include \"Runtime/Core/Scripting/ScriptingObject.h\"\n";
        output += "#include \"Runtime/Core/Scripting/Internal/InternalCalls.h\"\n";
        output += "#include \"Runtime/Core/Scripting/ScriptingType.h\"\n";

        // Conditional includes for events
        bool hasEvents = false;
        for (auto cls : headerInfo.classes)
        {
            if (!cls->APIInBuildMapType.empty())
            {
                continue;
            }
            if (!cls->events.empty())
            {
                hasEvents = true;
                break;
            }
        }

        if (hasEvents)
        {
            output += "#include \"Runtime/Core/Scripting/Events.h\"\n";
            output += "#include \"Runtime/Core/Scripting/ManagedCLR/CLRMethod.h\"\n";
        }

        // Interfaces
        if (!headerInfo.interfaces.empty() || hasEvents)
        {
            output += "#include \"Runtime/Core/Scripting/ManagedCLR/CLRClass.h\"\n";
        }

        for (auto const code : headerInfo.injectedCode)
        {
            if (code->lang == InjectEnum::CPP)
            {
                output += code->code;
                if (!Utils::String::EndsWith(output, '\n'))
                {
                    output += "\n";
                }
            }
        }

        output += "\n";

        // Generate enums first
        for (auto en : headerInfo.enums)
        {
            GenerateCppEnum(*en, assemblyType, output);
        }

        // Generate interfaces
        for (auto iface : headerInfo.interfaces)
        {
            if (!iface->APIInBuildMapType.empty())
            {
                continue;
            }
            GenerateCppInterface(*iface, assemblyType, output);
        }

        // Generate classes/structs
        for (auto cls : headerInfo.classes)
        {
            if (!cls->APIInBuildMapType.empty())
            {
                continue;
            }
            if (cls->isStruct)
            {
                GenerateCppStruct(*cls, assemblyType, output);
            }
            else
            {
                GenerateCppClass(*cls, assemblyType, output);
            }
        }

        return true;
    }

} // namespace SE::BuildTool
