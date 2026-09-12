
#include "CodeGenerator_BindingsCpp.h"
#include "CodeGenerator_BindingsTypeMap.h"
#include "CodeGenerator_Utils.h"

#include "Core/Utils.h"

namespace SE::BuildTool
{
    using CodeGeneratorUtils::GetPropertyName;
    using CodeGeneratorUtils::WithoutArray;

    static bool IsTypeName(const std::string& typeName, std::string_view unqualifiedName)
    {
        return typeName == unqualifiedName || typeName == "SE::" + std::string(unqualifiedName);
    }

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    static std::string GetCppNativeSimpleName(const TypeInfoStruct& cls)
    {
        return cls.APIName.empty() ? cls.name : cls.APIName;
    }


    static std::string GetCppNativeInvokeTypeName(const TypeInfoStruct& cls)
    {
        std::string nativeName = cls.APIIsNativeInvokeUseName ? cls.name : GetCppNativeSimpleName(cls);
        return CodeGeneratorUtils::GetFullNativeName(cls.namespaceScopeList, cls.structScopeList, nativeName);
    }

    static std::string GetCollectionDataExpression(const std::string& expression)
    {
        return expression + ".Get()";
    }

    CppTypeConversion BindingsCppGenerator::ResolveConversion(TypeInfo const& type, std::string_view marshalAs,
                                                               BindingUseSite useSite, BindingDirection direction) const
    {
        const BindingTypeSemantics semantics = ResolveBindingTypeSemantics(m_Database, type, marshalAs);
        return ResolveCppTypeConversion(m_Database, semantics, useSite, direction);
    }

    std::string BindingsCppGenerator::GetInteropValueType(TypeInfo const& cppType, std::string_view marshalAs) const
    {
        const CppTypeConversion conversion = ResolveConversion(cppType, marshalAs);
        switch (conversion.kind)
        {
            case BindingTypeKind::String:
            case BindingTypeKind::StringView:
            case BindingTypeKind::InteropStruct:
            case BindingTypeKind::ObjectRef:
            case BindingTypeKind::Collection:
            case BindingTypeKind::Blittable:
            case BindingTypeKind::ScriptingObject:
            case BindingTypeKind::NativeObject:
            case BindingTypeKind::OpaquePointer:
            case BindingTypeKind::VariantFamily:
            case BindingTypeKind::TypeHandle: return conversion.exportType;
            default: break;
        }
        return cppType.ToString(true, true);
    }

    std::string BindingsCppGenerator::GetReturnTypeConver(const TypeInfoFunc& fn) const
    {
        TypeInfo returnType = fn.returnType;
        if (returnType.typeID == TypeInfo::Void.typeID)
        {
            return "void";
        }
        if (GetCollectionInfo(returnType).IsCollection())
        {
            return "CLRArray*";
        }
        return GetInteropValueType(returnType, fn.marshalAs);
    }

    TypeInfoBase const* BindingsCppGenerator::GetRegisteredType(TypeID typeID) const
    {
        return m_Database.ResolveTypeDeclaration(TypeInfo(typeID));
    }

    bool BindingsCppGenerator::CanGenerateVariantFieldAccess(TypeInfo const& type) const
    {
        const BindingTypeKind kind = ResolveBindingTypeSemantics(m_Database, type).kind;
        return kind == BindingTypeKind::Blittable || kind == BindingTypeKind::String ||
               kind == BindingTypeKind::StringView || kind == BindingTypeKind::ScriptingObject ||
               kind == BindingTypeKind::NativeObject || kind == BindingTypeKind::ObjectRef ||
               kind == BindingTypeKind::VariantFamily || kind == BindingTypeKind::TypeHandle ||
               kind == BindingTypeKind::OpaquePointer;
    }

    bool BindingsCppGenerator::GetNativeToManagedConvert(TypeInfo const& type, std::string& expr, std::string_view marshalAs) const
    {
        const std::string baseType = type.typeID.ToString();
        const CppTypeConversion conversion = ResolveConversion(type, marshalAs, BindingUseSite::Return, BindingDirection::Out);
        if (baseType == "TypeID" || baseType == "SE::TypeID")
        {
            expr = Utils::String::Format("(uint32){0}", expr);
            return true;
        }
        if (conversion.kind == BindingTypeKind::Blittable)
            return true;
        if (conversion.kind == BindingTypeKind::String || conversion.kind == BindingTypeKind::StringView)
        {
            expr = Utils::String::Format("CLRUtils::ToString({0})", expr);
            return true;
        }
        if (conversion.kind == BindingTypeKind::InteropStruct)
        {
            expr = Utils::String::Format("BindingsInterop::ToManaged({0})", expr);
            return true;
        }
        if (conversion.kind == BindingTypeKind::ObjectRef)
        {
            expr = Utils::String::Format("ScriptingObject::ToManaged(reinterpret_cast<ScriptingObject*>({0}.Get()))", expr);
            return true;
        }
        if (IsTypeName(baseType, "Variant"))
        {
            expr = Utils::String::Format("CLRUtils::BoxVariant({0})", expr);
            return true;
        }
        if (IsTypeName(baseType, "VariantType"))
        {
            expr = Utils::String::Format("CLRUtils::BoxVariantType({0})", expr);
            return true;
        }
        if (IsTypeName(baseType, "ScriptingTypeHandle"))
        {
            expr = Utils::String::Format("CLRUtils::BoxScriptingTypeHandle({0})", expr);
            return true;
        }
        if (conversion.kind == BindingTypeKind::ScriptingObject)
        {
            // API headers frequently forward-declare a scripting object return
            // type. Use an explicit base-pointer reinterpret cast so the stub
            // does not require the concrete type definition merely to emit the
            // managed handle conversion.
            expr = Utils::String::Format("ScriptingObject::ToManaged(reinterpret_cast<ScriptingObject*>({0}))", expr);
            return true;
        }
        if (conversion.kind == BindingTypeKind::OpaquePointer)
        {
            // Opaque pointers are address values. Cast through const void* so
            // const-qualified native pointees can be returned through the
            // pointer-sized ABI slot without changing their native type.
            expr = Utils::String::Format("const_cast<void*>(reinterpret_cast<const void*>({0}))", expr);
            return true;
        }
        return false;
    }

    bool BindingsCppGenerator::GetManagedToNativeConvert(TypeInfo const& type, std::string& expr, std::string_view marshalAs) const
    {
        const std::string baseType = type.typeID.ToString();
        const CppTypeConversion conversion = ResolveConversion(type, marshalAs);
        if (baseType == "TypeID" || baseType == "SE::TypeID")
        {
            expr = Utils::String::Format("::SE::TypeID((uint32){0})", expr);
            return true;
        }
        if (conversion.kind == BindingTypeKind::Blittable)
            return true;

        if (conversion.kind == BindingTypeKind::String || conversion.kind == BindingTypeKind::StringView)
        {
            if (IsTypeName(baseType, "String") || IsTypeName(baseType, "StringView"))
            {
                expr = Utils::String::Format("CLRUtils::ToString((CLRString*){0})", expr);
            }
            else
            {
                expr = Utils::String::Format("CLRUtils::ToStringAnsi((CLRString*){0})", expr);
            }

            return true;
        }
        if (conversion.kind == BindingTypeKind::InteropStruct)
        {
            expr = Utils::String::Format("BindingsInterop::ToNative({0})", expr);
            return true;
        }
        if (conversion.kind == BindingTypeKind::ObjectRef)
        {
            const std::string targetType = type.genericityArgs.empty()
                ? "ScriptingObject" : CodeGeneratorUtils::QualifyCppType(type.genericityArgs[0].ToString(false));
            expr = Utils::String::Format("{0}(({1}*)ScriptingObject::ToNative((CLRObject*){2}))",
                type.ToString(false, true), targetType, expr);
            return true;
        }
        if (IsTypeName(baseType, "Variant"))
        {
            expr = Utils::String::Format("CLRUtils::UnboxVariant((CLRObject*){0})", expr);
            return true;
        }
        if (IsTypeName(baseType, "VariantType"))
        {
            expr = Utils::String::Format("CLRUtils::UnboxVariantType((CLRTypeObject*){0})", expr);
            return true;
        }

        if (IsTypeName(baseType, "ScriptingTypeHandle"))
        {
            expr = Utils::String::Format("CLRUtils::UnboxScriptingTypeHandle((CLRTypeObject*){0})", expr);
            return true;
        }
        if (conversion.kind == BindingTypeKind::ScriptingObject || conversion.kind == BindingTypeKind::NativeObject ||
            conversion.kind == BindingTypeKind::OpaquePointer)
        {
            std::string nativeType = conversion.nativeValueType;

            expr = Utils::String::Format("({0}){1}", nativeType, expr);
            return true;
        }

        return false;
    }

    std::string BindingsCppGenerator::GetNativeToVariantConvert(TypeInfo const& type, const std::string& expr) const
    {
        TypeInfoBase const* registeredType = GetRegisteredType(type.typeID);
        const std::string baseType = type.typeID.ToString();
        const BindingTypeSemantics conversion = ResolveBindingTypeSemantics(m_Database, type);
        if (registeredType != nullptr && registeredType->IsFlag(TypeInfoBase::Flag::IsEnum))
            return Utils::String::Format("Variant((uint64){0})", expr);
        if (baseType == "bool" || baseType == "int32" || baseType == "uint32"
            || baseType == "int64" || baseType == "uint64" || baseType == "float"
            || baseType == "double")
            return Utils::String::Format("Variant({0})", expr);
        if (conversion.kind == BindingTypeKind::ScriptingObject)
        {
            return Utils::String::Format("Variant((ScriptingObject*){0})", expr);
        }
        if (conversion.kind == BindingTypeKind::ObjectRef)
        {
            return Utils::String::Format("Variant((ScriptingObject*){0}.Get())", expr);
        }
        if (conversion.kind == BindingTypeKind::OpaquePointer)
        {
            return Utils::String::Format("Variant(const_cast<void*>(reinterpret_cast<const void*>({0})))", expr);
        }
        return Utils::String::Format("Variant({0})", expr);
    }

    std::string BindingsCppGenerator::GetVariantToNativeConvert(TypeInfo const& type, const std::string& expr) const
    {
        TypeInfoBase const* registeredType = GetRegisteredType(type.typeID);
        const std::string baseType = type.typeID.ToString();
        TypeInfo valueType = type;
        valueType.isConst = false;
        valueType.isRef = false;
        valueType.isMoveRef = false;
        const std::string nativeType = CodeGeneratorUtils::QualifyCppType(valueType.ToString(false, true));
        const BindingTypeSemantics conversion = ResolveBindingTypeSemantics(m_Database, type);
        if (registeredType != nullptr && registeredType->IsFlag(TypeInfoBase::Flag::IsEnum))
            return Utils::String::Format("({0})(uint64){1}", nativeType, expr);
        if (IsTypeName(baseType, "Variant") || IsTypeName(baseType, "VariantType"))
            return expr;
        if (baseType == "String")
            return Utils::String::Format("(StringView){0}", expr);
        if (baseType == "StringAnsi")
            return Utils::String::Format("(StringAnsiView){0}", expr);
        if (conversion.kind == BindingTypeKind::String || conversion.kind == BindingTypeKind::StringView)
            return Utils::String::Format("(StringView){0}", expr);
        if (conversion.kind == BindingTypeKind::ScriptingObject)
        {
            return Utils::String::Format("({0}*)ScriptingObject::Cast((ScriptingObject*){1})", nativeType, expr);
        }
        if (conversion.kind == BindingTypeKind::ObjectRef)
        {
            const std::string targetType = type.genericityArgs.empty()
                ? "ScriptingObject" : CodeGeneratorUtils::QualifyCppType(type.genericityArgs[0].ToString(false, true));
            return Utils::String::Format("{0}(({1}*)(void*){2})", nativeType, targetType, expr);
        }
        if (conversion.kind == BindingTypeKind::OpaquePointer)
        {
            return Utils::String::Format("({0})(void*){1}", nativeType, expr);
        }
        if (conversion.kind != BindingTypeKind::Blittable)
        {
            return Utils::String::Format("({0})(uint64){1}", nativeType, expr);
        }
        return Utils::String::Format("({0}){1}", nativeType, expr);
    }

    std::string BindingsCppGenerator::BuildWrapperParams(const TypeInfoStruct& cls, const TypeInfoFunc& fn, bool forExport) const
    {
        std::string params;
        const FunctionAbiPlan plan = BuildFunctionAbiPlan(m_Database, cls, fn);
        for (auto const& parameter : plan.parameters)
        {
            if (!params.empty()) params += ", ";
            if (parameter.role == AbiParameterRole::This)
            {
                if (forExport)
                    params += "void* __obj";
                else
                {
                    std::string nativeTypeName = CodeGeneratorUtils::GetFullNativeName(
                        cls.namespaceScopeList, cls.structScopeList, GetCppNativeSimpleName(cls));
                    params += Utils::String::Format("::{0}* __obj", nativeTypeName);
                }
                continue;
            }
            if (parameter.role == AbiParameterRole::HiddenCount)
            {
                if (parameter.publicParameterIndex >= 0)
                {
                    const char* pointer = parameter.type.passMode == AbiPassMode::Value ? "" : "*";
                    params += Utils::String::Format("int32{0} __{1}Count", pointer, fn.params[parameter.publicParameterIndex].name);
                }
                else
                    params += "int32* __returnCount";
                continue;
            }
            if (parameter.role == AbiParameterRole::HiddenResult)
            {
                const CppTypeConversion conversion = ResolveConversion(
                    fn.returnType, fn.marshalAs, BindingUseSite::Return, BindingDirection::Out);
                params += Utils::String::Format("{0}* __resultAsRef", conversion.exportType);
                continue;
            }
            const TypeInfoParam& publicParameter = fn.params[parameter.publicParameterIndex];
            const BindingDirection direction = GetBindingDirection(publicParameter);
            const CppTypeConversion conversion = ResolveConversion(
                publicParameter.type, publicParameter.marshalAs, BindingUseSite::Parameter, direction);
            const char* pointer = parameter.type.passMode == AbiPassMode::Value ? "" : "*";
            params += Utils::String::Format("{0}{1} {2}", conversion.exportType, pointer, publicParameter.name);
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
            TypeInfo paramType = fn.params[i].type;
            const CollectionInfo collection = GetCollectionInfo(paramType);
            if (collection.HasRuntimeCount())
                args += Utils::String::Format(", __{0}Count", fn.params[i].name);
        }
        return args;
    }

    std::string BindingsCppGenerator::BuildCallArgs(const TypeInfoStruct& cls, const TypeInfoFunc& fn,
                                                    std::string& setupOut, std::string& postCallOut) const
    {
        std::string args;
        for (int i = 0; i < fn.params.size(); ++i)
        {
            if (i > 0) args += ", ";

            const TypeInfoParam& param = fn.params[i];
            const TypeInfo& paramType = param.type;
            const BindingDirection direction = GetBindingDirection(param);
            const std::string abiExpression = direction == BindingDirection::In
                ? param.name : Utils::String::Format("*{0}", param.name);
            const CollectionInfo collection = GetCollectionInfo(paramType);
            std::string converted;

            if (collection.IsCollection())
            {
                const TypeInfo& elementType = collection.elementType;
                const std::string nativeElementType = CodeGeneratorUtils::QualifyCppType(elementType.ToString(false));
                const std::string localName = Utils::String::Format("__{0}Native", param.name);
                const std::string countName = Utils::String::Format("__{0}NativeCount", param.name);
                const std::string arrayName = Utils::String::Format("__{0}Array", param.name);
                setupOut += Utils::String::Format("        CLRArray* {0} = {1};\n", arrayName,
                    direction == BindingDirection::Out ? "nullptr" : abiExpression);

                const std::string typeName = paramType.typeID.ToString();
                if (typeName == "BytesContainer" || typeName == "SE::BytesContainer")
                {
                    if (direction == BindingDirection::Out)
                        setupOut += Utils::String::Format("        ::SE::BytesContainer {0};\n", localName);
                    else
                        setupOut += Utils::String::Format("        auto {0} = CLRUtils::LinkArray({1});\n", localName, arrayName);
                    converted = localName;
                }
                else if (typeName == "DataContainer" || typeName == "SE::DataContainer")
                {
                    setupOut += Utils::String::Format("        ::SE::DataContainer<{0}> {1};\n", nativeElementType, localName);
                    if (direction != BindingDirection::Out)
                        setupOut += Utils::String::Format("        CLRUtils::ToArray({0}, {1});\n", arrayName, localName);
                    converted = localName;
                }
                else
                {
                    const std::string requestedCount = collection.HasRuntimeCount()
                        ? (direction == BindingDirection::In
                            ? Utils::String::Format("__{0}Count", param.name)
                            : Utils::String::Format("(__{0}Count != nullptr ? *__{0}Count : 0)", param.name))
                        : Utils::String::Format("CLRCore::Array::GetLength({0})", arrayName);
                    const bool isSpan = typeName == "SE::Span" || typeName == "Span";
                    const std::string nativeCollectionType = isSpan
                        ? Utils::String::Format("::SE::List<{0}, ::SE::HeapAllocation>", nativeElementType)
                        : CodeGeneratorUtils::QualifyCppType(paramType.ToNativeType());
                    setupOut += Utils::String::Format("        {0} {1};\n", nativeCollectionType, localName);
                    setupOut += Utils::String::Format("        const int32 {0} = {1} ? Math::Min((int32)CLRCore::Array::GetLength({1}), (int32){2}) : 0;\n",
                        countName, arrayName, direction == BindingDirection::Out ? "0" : requestedCount);
                    setupOut += Utils::String::Format("        {0}.Resize({1});\n", localName, countName);
                    setupOut += Utils::String::Format("        if ({0} > 0)\n        {{\n", countName);
                    setupOut += Utils::String::Format("            auto* __{0}Items = CLRCore::Array::GetAddress<{1}>({2});\n",
                        param.name, GetInteropValueType(elementType), arrayName);

                    std::string elementExpression = Utils::String::Format("__{0}Items[i]", param.name);
                    GetManagedToNativeConvert(elementType, elementExpression);
                    setupOut += Utils::String::Format("            for (int32 i = 0; i < {0}; ++i) {1}[i] = {2};\n",
                        countName, localName, elementExpression);
                    setupOut += "        }\n";

                    if (isSpan)
                        converted = Utils::String::Format("::SE::Span<{0}>({1}.Get(), {1}.Count())", nativeElementType, localName);
                    else
                        converted = localName;
                }

                if (direction != BindingDirection::In)
                {
                    const bool usesLength = typeName == "BytesContainer" || typeName == "SE::BytesContainer" ||
                        typeName == "DataContainer" || typeName == "SE::DataContainer";
                    const std::string outCountName = Utils::String::Format("__{0}OutCount", param.name);
                    const std::string elementClassName = Utils::String::Format("__{0}ElementClass", param.name);
                    const BindingTypeSemantics elementSemantics = ResolveBindingTypeSemantics(m_Database, elementType);
                    const CSharpTypeConversion elementConversion = ResolveCSharpTypeConversion(
                        m_Database, elementSemantics, BindingUseSite::ArrayElement, BindingDirection::In);
                    postCallOut += Utils::String::Format("        const int32 {0} = {1}{2};\n", outCountName, localName,
                        usesLength ? ".Length()" : ".Count()");
                    postCallOut += Utils::String::Format("        if (__{0}Count != nullptr) *__{0}Count = {1};\n", param.name, outCountName);
                    postCallOut += Utils::String::Format("        CLRClass* {0} = Scripting::FindClass(StringAnsiView(\"{1}\"));\n",
                        elementClassName, elementConversion.publicType);
                    postCallOut += Utils::String::Format("        if ({0} == nullptr || {1} == nullptr)\n        {{\n", elementClassName, param.name);
                    postCallOut += Utils::String::Format("            if ({0} != nullptr) *{0} = nullptr;\n        }}\n        else\n        {{\n", param.name);
                    if (elementSemantics.kind != BindingTypeKind::InteropStruct)
                    {
                        postCallOut += Utils::String::Format("            *{0} = CLRUtils::ToArray(::SE::Span<{1}>({2}.Get(), {3}), {4});\n",
                            param.name, nativeElementType, localName, outCountName, elementClassName);
                    }
                    else
                    {
                        const std::string resultName = Utils::String::Format("__{0}Managed", param.name);
                        postCallOut += Utils::String::Format("            CLRArray* {0} = CLRCore::Array::New({1}, {2});\n",
                            resultName, elementClassName, outCountName);
                        postCallOut += Utils::String::Format("            if ({0} != nullptr && {1} > 0)\n            {{\n", resultName, outCountName);
                        postCallOut += Utils::String::Format("                auto* __{0}OutItems = CLRCore::Array::GetAddress<{1}>({2});\n",
                            param.name, GetInteropValueType(elementType), resultName);
                        std::string elementOut = Utils::String::Format("{0}[i]", localName);
                        GetNativeToManagedConvert(elementType, elementOut);
                        postCallOut += Utils::String::Format("                for (int32 i = 0; i < {0}; ++i) __{1}OutItems[i] = {2};\n",
                            outCountName, param.name, elementOut);
                        postCallOut += "            }\n";
                        postCallOut += Utils::String::Format("            *{0} = {1};\n", param.name, resultName);
                    }
                    postCallOut += "        }\n";
                }
            }
            else
            {
                const BindingTypeSemantics semantics = ResolveBindingTypeSemantics(m_Database, paramType, param.marshalAs);
                const bool manualWriteBack = direction != BindingDirection::In &&
                    (semantics.kind == BindingTypeKind::InteropStruct || semantics.kind == BindingTypeKind::ObjectRef);
                if (manualWriteBack)
                {
                    const std::string localName = Utils::String::Format("__{0}Native", param.name);
                    std::string initialValue = abiExpression;
                    GetManagedToNativeConvert(paramType, initialValue, param.marshalAs);
                    setupOut += Utils::String::Format("        auto {0} = {1};\n", localName, initialValue);
                    converted = localName;
                    std::string writeBack = localName;
                    GetNativeToManagedConvert(paramType, writeBack, param.marshalAs);
                    postCallOut += Utils::String::Format("        *{0} = {1};\n", param.name, writeBack);
                }
                else
                {
                    converted = abiExpression;
                    GetManagedToNativeConvert(paramType, converted, param.marshalAs);
                }
            }
            args += converted;
        }
        return args;
    }

    void BindingsCppGenerator::GenerateCollectionReturn(const TypeInfoFunc& fn, const CollectionInfo& collection,
                                                        const std::string& nativeExpression, const std::string& postCall,
                                                        std::string& output) const
    {
        const TypeInfo& elementType = collection.elementType;
        const std::string nativeElementType = CodeGeneratorUtils::QualifyCppType(elementType.ToString(false));
        const std::string interopElementType = GetInteropValueType(elementType);
        const BindingTypeSemantics elementSemantics = ResolveBindingTypeSemantics(m_Database, elementType);
        const CSharpTypeConversion elementConversion = ResolveCSharpTypeConversion(
            m_Database, elementSemantics, BindingUseSite::ArrayElement, BindingDirection::In);
        const std::string managedElementType = elementConversion.publicType;
        const bool usesInteropStruct = elementConversion.kind == BindingTypeKind::InteropStruct;

        output += Utils::String::Format("        const auto& __collectionValue = {0};\n", nativeExpression);
        if (collection.kind == CollectionKind::Fixed)
            output += Utils::String::Format("        const int32 __collectionCount = {0};\n", collection.fixedElementCount);
        else
        {
            output += Utils::String::Format("        const int32 __collectionCount = __collectionValue{0};\n", (fn.returnType.typeID == TypeID("Span") ? ".Length()": ".Count()"));
        }
        if (collection.HasRuntimeCount())
            output += "        if (__returnCount != nullptr) *__returnCount = __collectionCount;\n";
        output += Utils::String::Format("        CLRClass* __elementClass = Scripting::FindClass(StringAnsiView(\"{0}\"));\n", managedElementType);
        output += "        if (__elementClass == nullptr)\n        {\n";
        output += postCall;
        output += "            return nullptr;\n        }\n";

        if (!usesInteropStruct)
        {
            const std::string dataExpression = collection.kind == CollectionKind::Fixed
                ? "__collectionValue" : GetCollectionDataExpression("__collectionValue");
            output += Utils::String::Format("        auto* __result = CLRUtils::ToArray(::SE::Span<{0}>({1}, __collectionCount), __elementClass);\n",
                nativeElementType, dataExpression);
            output += postCall;
            output += "        return __result;\n";
            return;
        }

        output += "        CLRArray* __result = CLRCore::Array::New(__elementClass, __collectionCount);\n";
        output += "        if (__result == nullptr || __collectionCount == 0)\n        {\n";
        output += postCall;
        output += "            return __result;\n        }\n";
        output += Utils::String::Format("        auto* __resultItems = CLRCore::Array::GetAddress<{0}>(__result);\n", interopElementType);

        std::string out = "__collectionValue[i]";
        GetNativeToManagedConvert(elementType, out);

        output += Utils::String::Format("        for (int32 i = 0; i < __collectionCount; ++i) __resultItems[i] = {0};\n", out);
        output += postCall;
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
        std::vector<std::string> collectedNativeNames;

        auto collectType = [&](auto&& self, TypeInfo const& type, std::string_view marshalAs) -> void
        {
            const BindingTypeSemantics semantics = ResolveBindingTypeSemantics(m_Database, type, marshalAs);
            if (semantics.kind == BindingTypeKind::Collection)
            {
                self(self, semantics.collection.elementType, {});
                return;
            }
            if (semantics.kind != BindingTypeKind::InteropStruct || !semantics.declaration ||
                !semantics.declaration->IsFlag(TypeInfoBase::Flag::IsClassStruct))
            {
                return;
            }

            auto const* cls = static_cast<TypeInfoStruct const*>(semantics.declaration);
            const std::string nativeType = CodeGeneratorUtils::GetFullNativeName(
                cls->namespaceScopeList, cls->structScopeList, GetCppNativeSimpleName(*cls));
            if (Utils::Vector::Contains(collectedNativeNames, nativeType))
                return;
            collectedNativeNames.push_back(nativeType);

            HeaderInfo const* declarationHeader = m_Database.GetHeaderDesc(cls->headerID);
            if (declarationHeader && !Utils::Vector::Contains(includes, declarationHeader->filePath))
                includes.push_back(declarationHeader->filePath);

            // Emit nested ABI layouts before their consumers.
            for (auto const& field : cls->fields)
            {
                if (!field.isStatic)
                    self(self, WithoutArray(field.type), field.marshalAs);
            }
            structs.push_back(cls);
            structNativeNames.push_back(nativeType);
        };

        for (auto const& header : headers)
        {
            for (auto const& cls : header.classes)
            {
                if (cls->isAPI && cls->isStruct && cls->APIInBuildMapType.empty() && cls->APIMarshalAs.empty())
                {
                    // Blittable API structs also need their CLRConverter, so
                    // retain the explicit collection path for first-class APIs.
                    const std::string nativeType = CodeGeneratorUtils::GetFullNativeName(
                        cls->namespaceScopeList, cls->structScopeList, GetCppNativeSimpleName(*cls));
                    if (!Utils::Vector::Contains(collectedNativeNames, nativeType))
                    {
                        collectedNativeNames.push_back(nativeType);
                        if (!Utils::Vector::Contains(includes, header.filePath))
                            includes.push_back(header.filePath);
                        for (auto const& field : cls->fields)
                        {
                            if (!field.isStatic)
                                collectType(collectType, WithoutArray(field.type), field.marshalAs);
                        }
                        structs.push_back(cls);
                        structNativeNames.push_back(nativeType);
                    }
                }

                for (auto const& field : cls->fields)
                    collectType(collectType, field.type, field.marshalAs);
                for (auto const& fn : cls->functions)
                {
                    collectType(collectType, fn.returnType, fn.marshalAs);
                    for (auto const& param : fn.params)
                        collectType(collectType, param.type, param.marshalAs);
                }
                for (auto const& evt : cls->events)
                {
                    for (auto const& param : evt.params)
                        collectType(collectType, param.type, param.marshalAs);
                }
            }
        }

        if (structs.empty())
            return true;

        for (auto const& include : includes)
        {
            output += Utils::String::Format("#include \"{0}\"\n", include);
        }
        output += "#include \"Runtime/Core/Scripting/ManagedCLR/CLRUtils.h\"\n\n";
        output += "namespace SE::BindingsInterop\n{\n";

        for (auto const* cls : structs)
        {
            if (cls->isPod)
            {
                continue;
            }

            const std::string nativeType = CodeGeneratorUtils::GetFullNativeName(cls->namespaceScopeList, cls->structScopeList, GetCppNativeSimpleName(*cls));
            const std::string interopType = ResolveConversion(TypeInfo(cls->typeID), {}, BindingUseSite::Field).exportType;
            const int nameOffset = Utils::String::FindLast(interopType, ':');
            const std::string interopName = nameOffset == INVALID_INDEX ? interopType : interopType.substr(nameOffset + 1);

            output += Utils::String::Format("    struct {0}\n    {{\n", interopName);
            for (auto const& field : cls->fields)
            {
                if (field.isStatic) continue;

                TypeInfo fieldTypeInfo = field.type;
                const int fieldArraySize = fieldTypeInfo.arraySize;
                fieldTypeInfo.arraySize = 0;
                const std::string fieldType = GetInteropValueType(fieldTypeInfo, field.marshalAs);
                if (fieldArraySize > 0)
                {
                    output += Utils::String::Format("        {0} {1}[{2}];\n", fieldType, field.name, fieldArraySize);
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
                TypeInfo fieldTypeInfo = field.type;
                const int fieldArraySize = fieldTypeInfo.arraySize;
                fieldTypeInfo = WithoutArray(fieldTypeInfo);
                if (fieldArraySize > 0)
                {
                    std::string expr = Utils::String::Format("value.{0}[i]", field.name);
                    GetManagedToNativeConvert(fieldTypeInfo, expr, field.marshalAs);

                    output += Utils::String::Format("        for (int32 i = 0; i < {0}; ++i) result.{1}[i] = {2};\n", fieldArraySize, field.name, expr);
                }
                else
                {
                    std::string expr = Utils::String::Format("value.{0}", field.name);
                    GetManagedToNativeConvert(fieldTypeInfo, expr, field.marshalAs);

                    output += Utils::String::Format("        result.{0} = {1};\n", field.name, expr);
                }
            }
            output += "        return result;\n    }\n\n";

            output += Utils::String::Format("    inline {0} ToManaged(const ::{1}& value)\n    {{\n", interopName, nativeType);
            output += Utils::String::Format("        {0} result{{}};\n", interopName);
            for (auto const& field : cls->fields)
            {
                if (field.isStatic)
                    continue;
                TypeInfo fieldTypeInfo = field.type;
                const int fieldArraySize = fieldTypeInfo.arraySize;
                fieldTypeInfo = WithoutArray(fieldTypeInfo);
                if (fieldArraySize > 0)
                {
                    std::string expr = Utils::String::Format("value.{0}[i]", field.name);
                    GetNativeToManagedConvert(fieldTypeInfo, expr, field.marshalAs);

                    output += Utils::String::Format("        for (int32 i = 0; i < {0}; ++i) result.{1}[i] = {2};\n",fieldArraySize, field.name, expr);
                }
                else
                {
                    std::string expr = Utils::String::Format("value.{0}", field.name);
                    GetNativeToManagedConvert(fieldTypeInfo, expr, field.marshalAs);

                    output += Utils::String::Format("        result.{0} = {1};\n", field.name, expr);
                }
            }
            output += "        return result;\n    }\n\n";
        }

        output += "}\n\n";

        output += "namespace SE\n{\n";
        for (auto const* cls : structs)
        {
            // Referenced non-API structs need an ABI mirror and field conversion,
            // but they do not own managed type metadata for CLRConverter.
            if (!cls->isAPI)
                continue;
            const std::string nativeType = CodeGeneratorUtils::GetFullNativeName(cls->namespaceScopeList, cls->structScopeList, GetCppNativeSimpleName(*cls));
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
                const std::string interopType = ResolveConversion(TypeInfo(cls->typeID), {}, BindingUseSite::Field).exportType;
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

    void BindingsCppGenerator::GenerateCppMethodWrapperFunction(const TypeInfoStruct& cls,
                                                                const TypeInfoFunc&   fn,
                                                                std::string& bodyOut, std::string& endOut)
    {
        TypeInfo returnType = fn.returnType;

        const CollectionInfo returnCollection = GetCollectionInfo(returnType);
        std::string retType = GetReturnTypeConver(fn);
        std::string params = BuildWrapperParams(cls, fn, true);

        const bool retIsVoid = returnType.typeID == TypeInfo::Void.typeID;
        const FunctionAbiPlan abiPlan = BuildFunctionAbiPlan(m_Database, cls, fn);
        const bool useOutResult = abiPlan.usesHiddenResult;
        std::string collectionSetup;
        std::string postCall;
        std::string callArgs = BuildCallArgs(cls, fn, collectionSetup, postCall);
        std::string target;
        if (fn.isStatic)
        {
            std::string nativeName = GetCppNativeInvokeTypeName(cls);
            target = Utils::String::Format("::{0}::{1}", nativeName, fn.name);
        }
        else
        {
            const std::string nativeName = GetCppNativeInvokeTypeName(cls);
            target = Utils::String::Format("((::{0}*)__obj)->{1}", nativeName, fn.name);
        }

        const std::string callExpr = Utils::String::Format("{0}({1})", target, callArgs);

        std::string retConvert = callExpr;
        if (!GetNativeToManagedConvert(returnType, retConvert, fn.marshalAs))
        {
        }

        // MSVC exports the C++ helper under the flat C# entry-point name through
        // a linker alias. Other toolchains use the plain-C forwarding wrapper
        // emitted below. Both paths share the same ABI-safe signature.
        bodyOut += Utils::String::Format("    // SE ABI: {0}\n", abiPlan.fingerprint);
        bodyOut += "#if defined(_MSC_VER)\n";
        bodyOut += Utils::String::Format("    DLLEXPORT static {0} {1}({2})\n", useOutResult ? "void" : retType, fn.uniqueName, params);
        bodyOut += "#else\n";
        bodyOut += Utils::String::Format("    static {0} {1}({2})\n", useOutResult ? "void" : retType, fn.uniqueName, params);
        bodyOut += "#endif\n";
        bodyOut += "    {\n";
        bodyOut += "#if defined(_MSC_VER)\n";
        bodyOut += Utils::String::Format("        MSVC_FUNC_EXPORT({0})\n", fn.entryPoint);
        bodyOut += "#endif\n";


        if (returnCollection.IsCollection())
        {
            bodyOut += collectionSetup;
            GenerateCollectionReturn(fn, returnCollection, callExpr, postCall, bodyOut);
        }
        else if (retIsVoid)
        {
            bodyOut += collectionSetup;
            bodyOut += Utils::String::Format("        {0};\n", retConvert);
            bodyOut += postCall;
        }
        else if (useOutResult)
        {
            bodyOut += collectionSetup;
            bodyOut += Utils::String::Format("        *__resultAsRef = {0};\n", retConvert);
            bodyOut += postCall;
        }
        else
        {
            bodyOut += collectionSetup;
            if (postCall.empty())
                bodyOut.append(Utils::String::Format("        return {0};\n", retConvert));
            else
            {
                bodyOut += Utils::String::Format("        auto __returnValue = {0};\n", retConvert);
                bodyOut += postCall;
                bodyOut += "        return __returnValue;\n";
            }
        }
        bodyOut.append("    }\n");

        std::string exportParams = BuildWrapperParams(cls, fn, true);

        std::string forwardArgs = BuildForwardArgs(fn);
        if (useOutResult)
        {
            if (!forwardArgs.empty()) forwardArgs.append(", ");
            forwardArgs.append("__resultAsRef");
        }
        else if (returnCollection.HasRuntimeCount())
        {
            if (!forwardArgs.empty()) forwardArgs.append(", ");
            forwardArgs.append("__returnCount");
        }

        endOut.append(Utils::String::Format("// SE ABI: {0}\n", abiPlan.fingerprint));
        endOut.append("#if !defined(_MSC_VER)\n");
        endOut.append(Utils::String::Format("DEFINE_INTERNAL_CALL({0}) {1}_{2}({3})\n", useOutResult ? "void" : retType, cls.name, fn.uniqueName, exportParams));
        endOut.append("{\n");
        if (!fn.isStatic)
        {
            std::string nativeTypeName = CodeGeneratorUtils::GetFullNativeName(cls.namespaceScopeList, cls.structScopeList, GetCppNativeSimpleName(cls));
            std::string internalName = CodeGeneratorUtils::GetInternalClassName(cls.name);
            std::string castExpr = Utils::String::Format("return {0}::{1}((::{2}*)__obj{3}{4})", internalName, fn.uniqueName, nativeTypeName,!forwardArgs.empty() ? ", " : "", forwardArgs);
            endOut.append(Utils::String::Format("    {0};\n", (retIsVoid || useOutResult) ?
                Utils::String::Format("{0}::{1}((::{2}*)__obj{3}{4})",
                    internalName, fn.uniqueName, nativeTypeName,
                    !forwardArgs.empty() ? ", " : "",
                    forwardArgs) :
                castExpr));
        }
        else
        {
            endOut.append(Utils::String::Format("    {0}{1}Internal::{2}({3});\n", (retIsVoid || useOutResult) ? "" : "return ",
                cls.name, fn.uniqueName, forwardArgs));
        }
        endOut.append("}\n");
        endOut.append("#endif\n");
    }

    void BindingsCppGenerator::GenerateCppFieldWrapperFunction(const TypeInfoStruct& cls,
                                                               const TypeInfoFunc&   fn,
                                                               BindingInvocationKind invocation,
                                                               std::string& bodyOut, std::string& endOut)
    {
        TypeInfo returnType = fn.returnType;

        const CollectionInfo returnCollection = GetCollectionInfo(returnType);
        const bool isSetter = invocation == BindingInvocationKind::FieldSet;
        const CollectionInfo valueCollection = isSetter && !fn.params.empty()
            ? GetCollectionInfo(fn.params[0].type)
            : CollectionInfo();

        std::string retType = GetReturnTypeConver(fn);
        std::string params = BuildWrapperParams(cls, fn, true);

        const bool retIsVoid = returnType.typeID == TypeInfo::Void.typeID;
        const FunctionAbiPlan abiPlan = BuildFunctionAbiPlan(m_Database, cls, fn);
        const bool useOutResult = abiPlan.usesHiddenResult;

        std::string collectionSetup;
        std::string postCall;
        std::string callArgs = BuildCallArgs(cls, fn, collectionSetup, postCall);
        std::string target;
        if (fn.isStatic)
        {
            std::string nativeName = GetCppNativeInvokeTypeName(cls);
            target = Utils::String::Format("::{0}::{1}", nativeName, fn.name);
        }
        else
        {
            const std::string nativeName = GetCppNativeInvokeTypeName(cls);
            target = Utils::String::Format("((::{0}*)__obj)->{1}", nativeName, fn.name);
        }

        const std::string callExpr = isSetter ? Utils::String::Format("{0} = {1}", target, callArgs) : target;
        std::string retConvert = callExpr;
        GetNativeToManagedConvert(returnType, retConvert, fn.marshalAs);

        // MSVC exports the C++ helper under the flat C# entry-point name through
        // a linker alias. Other toolchains use the plain-C forwarding wrapper
        // emitted below. Both paths share the same ABI-safe signature.
        bodyOut += Utils::String::Format("    // SE ABI: {0}\n", abiPlan.fingerprint);
        bodyOut += "#if defined(_MSC_VER)\n";
        bodyOut += Utils::String::Format("    DLLEXPORT static {0} {1}({2})\n", useOutResult ? "void" : retType, fn.uniqueName, params);
        bodyOut += "#else\n";
        bodyOut += Utils::String::Format("    static {0} {1}({2})\n", useOutResult ? "void" : retType, fn.uniqueName, params);
        bodyOut += "#endif\n";
        bodyOut += "    {\n";
        bodyOut += "#if defined(_MSC_VER)\n";
        bodyOut += Utils::String::Format("        MSVC_FUNC_EXPORT({0})\n", fn.entryPoint);
        bodyOut += "#endif\n";

        if (returnCollection.IsCollection())
        {
            bodyOut += collectionSetup;
            GenerateCollectionReturn(fn, returnCollection, callExpr, postCall, bodyOut);
        }
        else if (isSetter && valueCollection.kind == CollectionKind::Fixed)
        {
            const std::string& valueName = fn.params[0].name;
            const TypeInfo& elementType = valueCollection.elementType;
            const std::string elementInteropType = GetInteropValueType(elementType);
            bodyOut += Utils::String::Format("        if ({0} == nullptr || CLRCore::Array::GetLength({0}) != {1}) return;\n",
                valueName, valueCollection.fixedElementCount);
            bodyOut += Utils::String::Format("        auto* __valueItems = CLRCore::Array::GetAddress<{0}>({1});\n", elementInteropType, valueName);

            std::string out = "__valueItems[i]";
            if (!GetManagedToNativeConvert(elementType, out) && (elementType.isPointer || elementType.isRef))
            {
                out = Utils::String::Format("*{0}", out);
            }
            bodyOut += Utils::String::Format("        for (int32 i = 0; i < {0}; ++i) {1}[i] = {2};\n", valueCollection.fixedElementCount, target, out);
        }
        else if (retIsVoid)
        {
            bodyOut += collectionSetup;
            bodyOut += Utils::String::Format("        {0};\n", retConvert);
            bodyOut += postCall;
        }
        else if (useOutResult)
        {
            bodyOut += collectionSetup;
            bodyOut += Utils::String::Format("        *__resultAsRef = {0};\n", retConvert);
            bodyOut += postCall;
        }
        else
        {
            bodyOut += collectionSetup;
            if (postCall.empty())
                bodyOut.append(Utils::String::Format("        return {0};\n", retConvert));
            else
            {
                bodyOut += Utils::String::Format("        auto __returnValue = {0};\n", retConvert);
                bodyOut += postCall;
                bodyOut += "        return __returnValue;\n";
            }
        }
        bodyOut.append("    }\n");

        std::string exportParams = BuildWrapperParams(cls, fn, true);

        std::string forwardArgs = BuildForwardArgs(fn);
        if (useOutResult)
        {
            if (!forwardArgs.empty()) forwardArgs.append(", ");
            forwardArgs.append("__resultAsRef");
        }
        else if (returnCollection.HasRuntimeCount())
        {
            if (!forwardArgs.empty()) forwardArgs.append(", ");
            forwardArgs.append("__returnCount");
        }

        endOut.append(Utils::String::Format("// SE ABI: {0}\n", abiPlan.fingerprint));
        endOut.append("#if !defined(_MSC_VER)\n");
        endOut.append(Utils::String::Format("DEFINE_INTERNAL_CALL({0}) {1}_{2}({3})\n", useOutResult ? "void" : retType, cls.name, fn.uniqueName, exportParams));
        endOut.append("{\n");
        if (!fn.isStatic)
        {
            std::string nativeTypeName = CodeGeneratorUtils::GetFullNativeName(cls.namespaceScopeList, cls.structScopeList, GetCppNativeSimpleName(cls));
            std::string internalName = CodeGeneratorUtils::GetInternalClassName(cls.name);
            std::string castExpr = Utils::String::Format("return {0}::{1}((::{2}*)__obj{3}{4})", internalName, fn.uniqueName, nativeTypeName,!forwardArgs.empty() ? ", " : "", forwardArgs);
            endOut.append(Utils::String::Format("    {0};\n", (retIsVoid || useOutResult) ?
                Utils::String::Format("{0}::{1}((::{2}*)__obj{3}{4})",
                    internalName, fn.uniqueName, nativeTypeName,
                    !forwardArgs.empty() ? ", " : "",
                    forwardArgs) :
                castExpr));
        }
        else
        {
            endOut.append(Utils::String::Format("    {0}{1}Internal::{2}({3});\n", (retIsVoid || useOutResult) ? "" : "return ",
                cls.name, fn.uniqueName, forwardArgs));
        }
        endOut.append("}\n");
        endOut.append("#endif\n");
    }


    void BindingsCppGenerator::GenerateCppEventWrappers(const TypeInfoStruct& cls, const TypeInfoEvent& evt,
                                                         const std::string& assemblyType, std::string& bodyOut, std::string& endOut)
    {
        std::string fullType = CodeGeneratorUtils::GetFullNativeName(cls.namespaceScopeList, cls.structScopeList, GetCppNativeSimpleName(cls));
        std::string internalName = CodeGeneratorUtils::GetInternalClassName(cls.name);

        // Build parameter type list for the event callback
        std::string paramTypes;
        for (int i = 0; i < evt.params.size(); ++i)
        {
            if (i > 0) paramTypes += ", ";

            TypeInfoParam const paramInfo = evt.params[i];
            std::string cppType = CodeGeneratorUtils::QualifyCppType(paramInfo.type.ToString());
            if (paramInfo.type.isConst)
                cppType = "const " + cppType;
            paramTypes += Utils::String::Format("{0} {1}", cppType, paramInfo.name);
        }

        // Managed wrapper - C++ calls C# delegate
        bodyOut += Utils::String::Format("    {0}void {1}_ManagedWrapper({2})\n", evt.isStatic ? "static " : "", evt.name, paramTypes);
        bodyOut += "    {\n";
        bodyOut += "        static CLRMethod* method = nullptr;\n";
        if (evt.isStatic)
        {
            const std::string managedType = CodeGeneratorUtils::GetFullCSTypeName(cls.namespaceScopeList, cls.name);
            bodyOut += Utils::String::Format("        if (!method)\n        {{\n            CLRClass* managedClass = ((ManagedBinaryModule*)GetBinaryModule{0}())->Assembly->GetClass(\"{1}\");\n", assemblyType, managedType);
            bodyOut += Utils::String::Format("            method = managedClass ? managedClass->GetMethod(\"Internal_{0}_Invoke\", {1}) : nullptr; ASSERT(method);\n", evt.name, evt.params.size());
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
                TypeInfoParam const paramInfo = evt.params[i];
                std::string convertExpr = GetNativeToVariantConvert(paramInfo.type, paramInfo.name);
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


    void BindingsCppGenerator::GenerateCppInitRuntime(const TypeInfoStruct& cls, std::string& output)
    {
        output += "    static void InitRuntime()\n    {\n";

        // Register events in ScriptingEvents table
        for (auto& evt : cls.events)
        {
            std::string fullType = CodeGeneratorUtils::GetFullNativeName(cls.namespaceScopeList, cls.structScopeList, GetCppNativeSimpleName(cls));
            output += Utils::String::Format(
                "        ScriptingEvents::EventsTable[Pair<ScriptingTypeHandle, StringView>({0}::TypeInitializer, StringView(SE_TEXT(\"{1}\")))] = (void(*)(ScriptingObject*, void*, bool)){2}Internal::{1}_ManagedWrapper;\n",
                CodeGeneratorUtils::RemovePreNameSpace(fullType), evt.name, cls.name);
        }

        output += "    }\n";
    }

    // -------------------------------------------------------------------------
    // Class generation
    // -------------------------------------------------------------------------

    void BindingsCppGenerator::GenerateCppClass(const TypeInfoStruct& cls, const std::string& assemblyType, std::string& output)
    {
        std::string fullNativeName = CodeGeneratorUtils::GetFullNativeName(cls.namespaceScopeList, cls.structScopeList, cls.name);
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
            BindingCallable getter = MakeBindingFieldGetter(cls, field);
            GenerateCppFieldWrapperFunction(cls, getter.function, getter.invocation, bodyOut, endOut);
            if (!field.APIIsReadOnly)
            {
                BindingCallable setter = MakeBindingFieldSetter(cls, field);
                GenerateCppFieldWrapperFunction(cls, setter.function, setter.invocation, bodyOut, endOut);
            }
        }

        std::vector<bool> consumedFunctions(cls.functions.size(), false);
        for (int i = 0; i < cls.functions.size(); ++i)
        {
            TypeInfoFunc const& fn = cls.functions[i];
            if (fn.APIIsPropertie)
            {
                if (!consumedFunctions[i])
                {
                    consumedFunctions[i] = true;

                    TypeInfo const returnType = fn.returnType;
                    if (fn.params.size() == 1)
                    {
                        BindingCallable setter = MakePropertySetter(cls, fn);

                        GenerateCppMethodWrapperFunction(cls, setter.function, bodyOut, endOut);
                    }

                    if (returnType.typeID != TypeInfo::Void.typeID)
                    {
                        BindingCallable getter = MakePropertyGetter(cls, fn);

                        GenerateCppMethodWrapperFunction(cls, getter.function, bodyOut, endOut);
                    }

                }
                continue;
            }

            GenerateCppMethodWrapperFunction(cls, fn, bodyOut, endOut);
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
                output += "    &ScriptingType::DefaultSpawn,\n";
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
        std::string fullNativeName = CodeGeneratorUtils::GetFullNativeName(cls.namespaceScopeList, cls.structScopeList, cls.name);
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
                BindingCallable getter = MakeBindingFieldGetter(cls, field);
                GenerateCppFieldWrapperFunction(cls, getter.function, getter.invocation, bodyOut, endOut);
                if (!field.APIIsReadOnly)
                {
                    BindingCallable setter = MakeBindingFieldSetter(cls, field);
                    GenerateCppFieldWrapperFunction(cls, setter.function, setter.invocation, bodyOut, endOut);
                }
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
            TypeInfo const fieldTypeInfo = field.type;
            TypeInfo const fieldElementType = WithoutArray(fieldTypeInfo);
            if (field.isStatic || !CanGenerateVariantFieldAccess(fieldElementType))
                continue;

            bodyOut += Utils::String::Format("        {0}if (name == SE_TEXT(\"{1}\"))\n", count == 0 ? "" : "else ", field.name);
            bodyOut += "        {\n";
            const int fieldArraySize = fieldTypeInfo.arraySize;
            if (fieldArraySize > 0)
            {
                bodyOut += "            List<Variant, HeapAllocation> __values;\n";
                bodyOut += Utils::String::Format("            __values.Resize({0});\n", fieldArraySize);
                bodyOut += Utils::String::Format("            for (int32 i = 0; i < {0}; ++i)\n", fieldArraySize);
                bodyOut += Utils::String::Format("                __values[i] = {0};\n",
                    GetNativeToVariantConvert(fieldElementType, Utils::String::Format("((::{0}*)ptr)->{1}[i]", fullNativeName, field.name)));
                bodyOut += "            value = Variant(__values);\n";
            }
            else
            {
                bodyOut += Utils::String::Format("            value = {0};\n",
                    GetNativeToVariantConvert(fieldTypeInfo, Utils::String::Format("((::{0}*)ptr)->{1}", fullNativeName, field.name)));
            }
            bodyOut += "        }\n";
            ++count;
        }
        bodyOut += "    }\n\n";

        bodyOut += "    static void SetField(void* ptr, const String& name, const Variant& value)\n    {\n";
        for (int i = 0, count = 0; i < cls.fields.size(); ++i)
        {
            TypeInfoField const& field = cls.fields[i];
            TypeInfo const fieldTypeInfo = field.type;
            TypeInfo const fieldElementType = WithoutArray(fieldTypeInfo);
            if (field.isStatic || field.APIIsReadOnly || !CanGenerateVariantFieldAccess(fieldElementType))
                continue;

            bodyOut += Utils::String::Format("        {0}if (name == SE_TEXT(\"{1}\"))\n", count == 0 ? "" : "else ", field.name);
            bodyOut += "        {\n";
            const int fieldArraySize = fieldTypeInfo.arraySize;
            if (fieldArraySize > 0)
            {
                bodyOut += "            if (value.Type != VariantTypes::Array)\n";
                bodyOut += "                return;\n";
                bodyOut += "            const auto& __values = value.AsArray();\n";
                bodyOut += Utils::String::Format("            const int32 __count = __values.Count() < {0} ? __values.Count() : {0};\n", fieldArraySize);
                bodyOut += "            for (int32 i = 0; i < __count; ++i)\n";
                bodyOut += Utils::String::Format("                ((::{0}*)ptr)->{1}[i] = {2};\n",
                    fullNativeName, field.name, GetVariantToNativeConvert(fieldElementType, "__values[i]"));
            }
            else
            {
                bodyOut += Utils::String::Format("            ((::{0}*)ptr)->{1} = {2};\n",
                    fullNativeName, field.name, GetVariantToNativeConvert(fieldTypeInfo, "value"));
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
        std::string fullNameName = CodeGeneratorUtils::GetFullNativeName(en.namespaceScopeList, en.structScopeList , en.name);
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
        std::string fullname = CodeGeneratorUtils::GetFullNativeName(
            iface.namespaceScopeList, {}, iface.APIName.empty() ? iface.name : iface.APIName);
        std::string fullTypename = CodeGeneratorUtils::GetFullCSTypeName(iface.namespaceScopeList, iface.name);
        std::string internalName = CodeGeneratorUtils::GetInternalClassName(iface.name);

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
                TypeInfo const paramType = fn.params[i].type;
                paramTypes += Utils::String::Format("{0} {1}", CodeGeneratorUtils::QualifyCppType(paramType.ToString(false)),
                    fn.params[i].name);
            }

            TypeInfo const returnTypeInfo = fn.returnType;
            const bool returnIsVoid = returnTypeInfo.typeID == TypeInfo::Void.typeID;
            std::string const returnType = CodeGeneratorUtils::QualifyCppType(returnTypeInfo.ToString());
            output += Utils::String::Format("    {0} {1}({2}) const override\n", returnType, fn.name, paramTypes);
            output += "    {\n";
            if (fn.params.size() > 0)
            {
                output += Utils::String::Format("        Variant parameters[{0}];\n", fn.params.size());
                for (int i = 0; i < fn.params.size(); ++i)
                {
                    TypeInfoParam const paramInfoType = fn.params[i];
                    std::string convertExpr = GetNativeToVariantConvert(paramInfoType.type, paramInfoType.name);
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
            if (!returnIsVoid)
            {
                output += Utils::String::Format("                return {0};\n", GetVariantToNativeConvert(returnTypeInfo, "__result"));
            }
            else
            {
                output += "                return;\n";
            }
            output += "            }\n";
            output += "            typeHandle = typeHandle.GetType().GetBaseType();\n";
            output += "        }\n";
            if (!returnIsVoid)
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
