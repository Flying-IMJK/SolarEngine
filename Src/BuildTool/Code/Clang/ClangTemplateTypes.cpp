#include "ClangTemplateTypes.h"
#include "CodeGenerators/CodeGenerator_Utils.h"
#include "Database/TypeDatabase.h"

#include <algorithm>
#include <memory>

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    static std::string StripCppKeywordPrefixes(std::string type)
    {
        Utils::String::TrimStart(type);
        Utils::String::TrimEnd(type);

        while (Utils::String::StartsWith(type, "::"))
        {
            type = type.substr(2);
        }
        if (Utils::String::StartsWith(type, "class "))
        {
            type = type.substr(6);
        }
        else if (Utils::String::StartsWith(type, "struct "))
        {
            type = type.substr(7);
        }
        else if (Utils::String::StartsWith(type, "enum "))
        {
            type = type.substr(5);
        }

        Utils::String::TrimStart(type);
        Utils::String::TrimEnd(type);
        return type;
    }

    TypeInfoTemplate ParseTemplateType(ClangParserContext* pContext, CXType type, std::vector<std::string> const& templateParam)
    {
        TypeInfoTemplate result;
        CXType workingType = type;
        clang::QualType qualType = ClangUtils::GetQualType(workingType);
        if (qualType->isArrayType())
        {
            if (qualType->isConstantArrayType())
            {
                auto const* pArrayType = (clang::ConstantArrayType*)qualType.getTypePtr();
                result.isArray = true;
                result.arraySize = (int)pArrayType->getSize().getSExtValue();
            }
            else
            {
                result.isArray = true;
            }
            workingType = clang_getElementType(workingType);
            qualType = ClangUtils::GetQualType(workingType);
        }

        CXType canonical = clang_getCanonicalType(workingType);
        if (canonical.kind == CXType_LValueReference || canonical.kind == CXType_RValueReference)
        {
            result.isRef = true;
            result.isMoveRef = canonical.kind == CXType_RValueReference;
            CXType pointeeType = clang_getPointeeType(workingType);
            if (pointeeType.kind == CXType_Invalid)
            {
                pointeeType = clang_getPointeeType(canonical);
            }
            if (pointeeType.kind != CXType_Invalid)
            {
                workingType = pointeeType;
            }
            canonical = clang_getCanonicalType(workingType);
        }

        while (canonical.kind == CXType_Pointer)
        {
            result.isPointer = true;
            ++result.pointerDepth;
            CXType pointeeType = clang_getPointeeType(workingType);
            if (pointeeType.kind == CXType_Invalid)
            {
                pointeeType = clang_getPointeeType(canonical);
            }
            if (pointeeType.kind != CXType_Invalid)
            {
                workingType = pointeeType;
            }
            canonical = clang_getCanonicalType(workingType);
        }

        result.isConst = clang_isConstQualifiedType(workingType) != 0 ||
                         clang_isConstQualifiedType(canonical) != 0;

        std::string qualifiedName;
        if (!ClangUtils::GetQualifiedNameForType(workingType, qualifiedName))
        {
            qualifiedName = ClangUtils::GetTypeSpellingAnsi(workingType);
        }
        result.fullName = StripCppKeywordPrefixes(qualifiedName);

        int const numTemplateArguments = clang_Type_getNumTemplateArguments(workingType);
        if (numTemplateArguments > 0)
        {
            result.genericArgs.clear();
            for (int i = 0; i < numTemplateArguments; i++)
            {
                CXType argType = clang_Type_getTemplateArgumentAsType(workingType, i);
                if (argType.kind == CXType_Invalid)
                {
                    continue;
                }
                result.genericArgs.emplace_back(ParseTemplateType(pContext, argType, templateParam));
            }
        }

        result.isTemplateParameter = std::find(templateParam.begin(), templateParam.end(), result.fullName) != templateParam.end();
        return result;
    }


    static TypeInfoTemplate SubstituteTemplateType(TypeInfoTemplate const& type,
                                                  std::vector<std::string> const& parameters,
                                                  std::vector<TypeInfoTemplate> const& arguments)
    {
        if (type.isTemplateParameter)
        {
            for (int i = 0; i < parameters.size() && i < arguments.size(); i++)
            {
                if (type.fullName == parameters[i])
                {
                    TypeInfoTemplate result = arguments[i];
                    result.isConst = type.isConst || result.isConst;
                    result.isPointer = type.isPointer || result.isPointer;
                    result.pointerDepth += type.pointerDepth;
                    result.isRef = type.isRef || result.isRef;
                    result.isMoveRef = type.isMoveRef || result.isMoveRef;
                    result.isArray = type.isArray || result.isArray;
                    result.arraySize = type.arraySize > 0 ? type.arraySize : result.arraySize;
                    return result;
                }
            }
        }

        TypeInfoTemplate result = type;
        result.genericArgs.clear();
        for (auto const& arg : type.genericArgs)
        {
            result.genericArgs.emplace_back(SubstituteTemplateType(arg, parameters, arguments));
        }
        result.isTemplateParameter = false;
        return result;
    }

    static TypeInfoParam InstantiateParam(TypeInfoParamTemplate const& paramTemplate,
                                          std::vector<std::string> const& parameters,
                                          std::vector<TypeInfoTemplate> const& arguments)
    {
        TypeInfoParam param;
        TypeInfoTemplate type = SubstituteTemplateType(paramTemplate.type, parameters, arguments);
        param.type.typeID = TypeID(type.fullName);
        param.type.arraySize = type.isArray ? type.arraySize : 0;
        param.type.isPointer = type.isPointer;
        param.type.pointerDepth = type.pointerDepth > 0 ? type.pointerDepth : (type.isPointer ? 1 : 0);
        param.type.isConst = type.isConst;
        param.type.isRef = type.isRef;
        param.type.isMoveRef = type.isMoveRef;

        param.name = paramTemplate.name;
        param.direction = paramTemplate.direction;
        param.defaultValue = paramTemplate.defaultValue;
        param.attributes = paramTemplate.attributes;
        param.marshalAs = paramTemplate.marshalAs;
        param.comment = paramTemplate.comment;
        return param;
    }

    static TypeInfoField InstantiateField(TypeInfoFieldTemplate const& fieldTemplate,
                                          std::vector<std::string> const& parameters,
                                          std::vector<TypeInfoTemplate> const& arguments)
    {
        TypeInfoField field;
        TypeInfoTemplate type = SubstituteTemplateType(fieldTemplate.type, parameters, arguments);
        field.isAPI = fieldTemplate.isAPI;
        field.isStatic = fieldTemplate.isStatic;

        field.type.typeID = TypeID(type.fullName);
        field.type.arraySize = type.arraySize;
        field.type.isPointer = type.isPointer;
        field.type.pointerDepth = type.pointerDepth > 0 ? type.pointerDepth : (type.isPointer ? 1 : 0);
        field.type.isConst = type.isConst;
        field.type.isRef = type.isRef;
        field.type.isMoveRef = type.isMoveRef;

        field.name = fieldTemplate.name;
        field.isReflect = fieldTemplate.isReflect;
        field.APIIsReadOnly = fieldTemplate.APIIsReadOnly;
        field.attributes = fieldTemplate.attributes;
        field.defaultValue = fieldTemplate.defaultValue;
        field.comment = fieldTemplate.comment;
        field.marshalAs = fieldTemplate.marshalAs;
        field.lineNumber = fieldTemplate.lineNumber;
        return field;
    }

    static TypeInfoFunc InstantiateFunction(TypeInfoFuncTemplate const& functionTemplate,
                                            std::vector<std::string> const& parameters,
                                            std::vector<TypeInfoTemplate> const& arguments)
    {
        TypeInfoFunc fn;
        TypeInfoTemplate returnType = SubstituteTemplateType(functionTemplate.returnType, parameters, arguments);
        fn.name = functionTemplate.name;
        fn.returnType.typeID = TypeID(returnType.fullName);
        fn.returnType.arraySize = returnType.isArray ? returnType.arraySize : 0;
        fn.returnType.isPointer = returnType.isPointer;
        fn.returnType.pointerDepth = returnType.pointerDepth > 0 ? returnType.pointerDepth : (returnType.isPointer ? 1 : 0);
        fn.returnType.isConst = returnType.isConst;
        fn.returnType.isRef = returnType.isRef;
        fn.returnType.isMoveRef = returnType.isMoveRef;

        for (auto const& paramTemplate : functionTemplate.params)
        {
            fn.params.emplace_back(InstantiateParam(paramTemplate, parameters, arguments));
        }
        fn.isReflect = functionTemplate.isReflect;
        fn.isAPI = functionTemplate.isAPI;
        fn.isStatic = functionTemplate.isStatic;
        fn.isVirtual = functionTemplate.isVirtual;
        fn.isConst = functionTemplate.isConst;
        fn.APINoProxy = functionTemplate.APINoProxy;
        fn.APIIsSealed = functionTemplate.APIIsSealed;
        fn.APIIsStatic = functionTemplate.APIIsStatic;
        fn.APIIsPropertie = functionTemplate.APIIsPropertie;
        fn.uniqueName = functionTemplate.uniqueName;
        fn.entryPoint = functionTemplate.entryPoint;
        fn.access = functionTemplate.access;
        fn.attributes = functionTemplate.attributes;
        fn.comment = functionTemplate.comment;
        fn.marshalAs = functionTemplate.marshalAs;
        fn.lineNumber = functionTemplate.lineNumber;
        return fn;
    }

    static TypeInfoEvent InstantiateEvent(TypeInfoEventTemplate const& eventTemplate,
                                          std::vector<std::string> const& parameters,
                                          std::vector<TypeInfoTemplate> const& arguments)
    {
        TypeInfoEvent evt;
        TypeInfoTemplate cppType = SubstituteTemplateType(eventTemplate.cppType, parameters, arguments);
        evt.isReflect = eventTemplate.isReflect;
        evt.isAPI = eventTemplate.isAPI;
        evt.name = eventTemplate.name;

        evt.cppType.typeID = TypeID(cppType.fullName);
        evt.cppType.arraySize = cppType.arraySize;
        evt.cppType.isPointer = cppType.isPointer;
        evt.cppType.pointerDepth = cppType.pointerDepth > 0 ? cppType.pointerDepth : (cppType.isPointer ? 1 : 0);
        evt.cppType.isConst = cppType.isConst;
        evt.cppType.isRef = cppType.isRef;
        evt.cppType.isMoveRef = cppType.isMoveRef;

        for (auto const& paramTemplate : eventTemplate.params)
        {
            evt.params.emplace_back(InstantiateParam(paramTemplate, parameters, arguments));
        }
        evt.isStatic = eventTemplate.isStatic;
        evt.access = eventTemplate.access;
        evt.attributes = eventTemplate.attributes;
        evt.comment = eventTemplate.comment;
        evt.lineNumber = eventTemplate.lineNumber;
        return evt;
    }

    static bool IsQualifiedTypeName(std::string const& name)
    {
        return name.find("::") != std::string::npos;
    }

    static std::string GetQualifiedTemplateInstantiationName(TypeInfoStructTemplate const& templateType,
                                                             TypeInfoTemplate const&        targetType)
    {
        TypeInfoTemplate qualifiedType = targetType;
        if (!IsQualifiedTypeName(qualifiedType.fullName))
        {
            qualifiedType.fullName = CodeGeneratorUtils::GetFullNativeName(templateType.namespaceScopeList,
                                                 templateType.structScopeList,
                                                 qualifiedType.fullName);
        }
        return qualifiedType.ToCppString(false);
    }

    std::unique_ptr<TypeInfoStruct> InstantiateTemplateType(ClangParserContext*                    pContext,
                                                            TypeInfoStructTemplate const&          templateType,
                                                            ClangParserContext::TypeDefData const& typeDef)
    {
        if (typeDef.targetType.genericArgs.size() != templateType.templateParameters.size())
        {
            pContext->LogError(
                "SE_TYPEDEF typedef ({0}) provides {1} template argument(s), but template ({2}) expects {3}",
                typeDef.name,
                typeDef.targetType.genericArgs.size(),
                templateType.name,
                templateType.templateParameters.size());
            return nullptr;
        }

        std::string const fullAliasName = CodeGeneratorUtils::GetFullNativeName(templateType.namespaceScopeList, templateType.structScopeList, typeDef.name);
        auto type      = std::make_unique<TypeInfoStruct>(pContext->GenerateTypeID(fullAliasName), typeDef.name);
        type->headerID = templateType.headerID;
        type->namespaceScopeList      = templateType.namespaceScopeList;
        type->structScopeList         = templateType.structScopeList;
        type->isAbstract              = templateType.isAbstract;
        type->isStruct                = templateType.isStruct;
        type->isScriptingObject       = templateType.isScriptingObject;
        type->isReflect               = typeDef.macro.hasReflect;
        type->isAPI                   = true;
        type->isTemplateInstantiation = true;
        type->templateInstantiationTypeName = GetQualifiedTemplateInstantiationName(templateType, typeDef.targetType);
        type->comment = typeDef.macro.macroComment.empty() ? templateType.comment : typeDef.macro.macroComment;

        type->APIIsAbstract    = templateType.APIIsAbstract;
        type->APIIsSealed      = templateType.APIIsSealed;
        type->APIIsStatic      = templateType.APIIsStatic;
        type->APINoSpawn       = templateType.APINoSpawn;
        type->APINoConstructor = templateType.APINoConstructor;
        type->APIIsInterface   = templateType.APIIsInterface;
        type->APIIsNativeInvokeUseName = templateType.APIIsNativeInvokeUseName;
        type->APIName          = templateType.APIName;
        type->APIAttributes    = templateType.APIAttributes;
        type->APIMarshalAs     = templateType.APIMarshalAs;

        if (typeDef.macro.HasApi())
        {
            MarkAPI const& api = typeDef.macro.GetApi();
            if (!api.inBuildMapType.empty())
            {
                pContext->LogError("API(InBuild(\"{0}\")) is not supported on SE_TYPEDEF template instantiation ({1})", api.inBuildMapType, typeDef.name);
                return nullptr;
            }
            type->APIIsNativeInvokeUseName = api.IsNativeInvokeUseName;
            if (!api.name.empty())
            {
                type->APIName = api.name;
            }
            if (!api.attributes.empty())
            {
                type->APIAttributes = api.attributes;
            }
            if (!api.marshalAs.empty())
            {
                type->APIMarshalAs = api.marshalAs;
            }
        }

        auto const& parameters = templateType.templateParameters;
        auto const& arguments  = typeDef.targetType.genericArgs;

        if (templateType.baseType.IsValid())
        {
            TypeInfoTemplate baseType = SubstituteTemplateType(templateType.baseType, parameters, arguments);
            type->parentTypeID       = type->parentTypeID;
        }

        for (auto const& fieldTemplate : templateType.fields)
        {
            type->fields.emplace_back(InstantiateField(fieldTemplate, parameters, arguments));
        }
        for (auto const& functionTemplate : templateType.functions)
        {
            type->functions.emplace_back(InstantiateFunction(functionTemplate, parameters, arguments));
        }
        for (auto const& eventTemplate : templateType.events)
        {
            type->events.emplace_back(InstantiateEvent(eventTemplate, parameters, arguments));
        }

        pContext->GetAssemblyInfoForHeader(type->headerID, type->assemblyName, type->assemblyDir);
        return type;
    }
} // namespace SE::BuildTool
