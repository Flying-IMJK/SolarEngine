#include "ClangTemplateTypes.h"
#include "CodeGenerators/CodeGenerator_Utils.h"
#include "Database/TypeDatabase.h"

#include <algorithm>
#include <cstdlib>
#include <memory>

//-------------------------------------------------------------------------

namespace SE::BuildTool
{
    static std::string Trimmed(std::string value)
    {
        Utils::String::TrimStart(value);
        Utils::String::TrimEnd(value);
        return value;
    }

    static bool IsTemplateParameter(std::string const& name, std::vector<std::string> const& templateParameters)
    {
        return std::find(templateParameters.begin(), templateParameters.end(), name) != templateParameters.end();
    }

    static std::string StripCppKeywordPrefixes(std::string type)
    {
        type = Trimmed(type);
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
        return Trimmed(type);
    }

    static int32 FindTopLevelChar(std::string const& text, char value)
    {
        int32 depth = 0;
        for (int32 i = 0; i < text.length(); i++)
        {
            char const c = text[(size_t)i];
            if (c == '<')
            {
                depth++;
            }
            else if (c == '>')
            {
                depth--;
            }
            else if (c == value && depth == 0)
            {
                return i;
            }
        }
        return INVALID_INDEX;
    }

    static void SplitTopLevelTemplateArgs(std::string const& text, std::vector<std::string>& outArgs)
    {
        int32 depth = 0;
        int32 start = 0;
        for (int32 i = 0; i < text.length(); i++)
        {
            char const c = text[(size_t)i];
            if (c == '<')
            {
                depth++;
            }
            else if (c == '>')
            {
                depth--;
            }
            else if (c == ',' && depth == 0)
            {
                outArgs.push_back(Trimmed(text.substr((size_t)start, (size_t)(i - start))));
                start = i + 1;
            }
        }

        if (start < text.length())
        {
            outArgs.push_back(Trimmed(text.substr((size_t)start)));
        }
    }

    static void StripArraySuffix(std::string& text, TypeRefTemplate& outType)
    {
        int32 const open = Utils::String::FindLast(text, '[');
        int32 const close = Utils::String::FindLast(text, ']');
        if (open == INVALID_INDEX || close == INVALID_INDEX || close <= open)
        {
            return;
        }

        std::string suffix = text.substr((size_t)open + 1, (size_t)(close - open - 1));
        suffix = Trimmed(suffix);
        outType.isArray = true;
        outType.arraySize = suffix.empty() ? 0 : std::atoi(suffix.c_str());
        text = Trimmed(text.substr(0, (size_t)open));
    }

    bool TryParseTemplateTypeRef(std::string const& typeText,
                                 TypeRefTemplate&   outType,
                                 std::vector<std::string> const& templateParameters)
    {
        std::string text = StripCppKeywordPrefixes(typeText);
        if (text.empty())
        {
            return false;
        }

        outType = TypeRefTemplate();
        StripArraySuffix(text, outType);

        if (Utils::String::StartsWith(text, "const "))
        {
            outType.isConst = true;
            text = Trimmed(text.substr(6));
        }

        if (Utils::String::EndsWith(text, "&&"))
        {
            outType.isMoveRef = true;
            outType.isRef = true;
            text = Trimmed(text.substr(0, text.length() - 2));
        }
        else if (Utils::String::EndsWith(text, "&"))
        {
            outType.isRef = true;
            text = Trimmed(text.substr(0, text.length() - 1));
        }

        if (Utils::String::EndsWith(text, "*"))
        {
            outType.isPointer = true;
            text = Trimmed(text.substr(0, text.length() - 1));
        }

        if (Utils::String::StartsWith(text, "const "))
        {
            outType.isConst = true;
            text = Trimmed(text.substr(6));
        }

        int32 const open = FindTopLevelChar(text, '<');
        int32 const close = Utils::String::FindLast(text, '>');
        if (open != INVALID_INDEX && close != INVALID_INDEX && close > open)
        {
            outType.name = StripCppKeywordPrefixes(text.substr(0, (size_t)open));

            std::vector<std::string> args;
            SplitTopLevelTemplateArgs(text.substr((size_t)open + 1, (size_t)(close - open - 1)), args);
            for (auto const& arg : args)
            {
                TypeRefTemplate parsedArg;
                if (!TryParseTemplateTypeRef(arg, parsedArg, templateParameters))
                {
                    return false;
                }
                outType.genericArgs.emplace_back(std::move(parsedArg));
            }
        }
        else
        {
            outType.name = StripCppKeywordPrefixes(text);
        }

        outType.isTemplateParameter = IsTemplateParameter(outType.name, templateParameters);
        return outType.IsValid();
    }

    static TypeRefTemplate ParseTemplateTypeRefInternal(ClangParserContext* pContext,
                                                        CXType              type,
                                                        std::vector<std::string> const& templateParameters)
    {
        TypeRefTemplate result;
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
            workingType = clang_getPointeeType(workingType);
            canonical = clang_getCanonicalType(workingType);
        }

        if (canonical.kind == CXType_Pointer)
        {
            result.isPointer = true;
            workingType = clang_getPointeeType(workingType);
            canonical = clang_getCanonicalType(workingType);
        }

        result.isConst = clang_isConstQualifiedType(workingType) != 0 ||
                         clang_isConstQualifiedType(canonical) != 0;

        std::string qualifiedName;
        if (!ClangUtils::GetQualifiedNameForType(workingType, qualifiedName))
        {
            qualifiedName = ClangUtils::GetTypeSpellingAnsi(workingType);
        }

        TypeRefTemplate parsedFromText;
        if (TryParseTemplateTypeRef(qualifiedName, parsedFromText, templateParameters))
        {
            bool const keepArray = result.isArray;
            int const arraySize = result.arraySize;
            bool const keepRef = result.isRef;
            bool const keepMoveRef = result.isMoveRef;
            bool const keepPointer = result.isPointer;
            bool const keepConst = result.isConst || parsedFromText.isConst;
            result = std::move(parsedFromText);
            result.isArray = keepArray || result.isArray;
            result.arraySize = arraySize > 0 ? arraySize : result.arraySize;
            result.isRef = keepRef || result.isRef;
            result.isMoveRef = keepMoveRef || result.isMoveRef;
            result.isPointer = keepPointer || result.isPointer;
            result.isConst = keepConst;
        }

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
                result.genericArgs.emplace_back(ParseTemplateTypeRefInternal(pContext, argType, templateParameters));
            }
        }

        if (result.name.empty())
        {
            result.name = StripCppKeywordPrefixes(ClangUtils::GetTypeSpellingAnsi(workingType));
        }

        int32 const genericStart = FindTopLevelChar(result.name, '<');
        if (genericStart != INVALID_INDEX)
        {
            result.name = StripCppKeywordPrefixes(result.name.substr(0, (size_t)genericStart));
        }
        result.isTemplateParameter = IsTemplateParameter(result.name, templateParameters);
        return result;
    }

    TypeRefTemplate ParseTemplateTypeRef(ClangParserContext* pContext,
                                         CXType              type,
                                         std::vector<std::string> const& templateParameters)
    {
        return ParseTemplateTypeRefInternal(pContext, type, templateParameters);
    }

    static TypeRefTemplate SubstituteTemplateType(TypeRefTemplate const& type,
                                                  std::vector<std::string> const& parameters,
                                                  std::vector<TypeRefTemplate> const& arguments)
    {
        if (type.isTemplateParameter)
        {
            for (int i = 0; i < parameters.size() && i < arguments.size(); i++)
            {
                if (type.name == parameters[i])
                {
                    TypeRefTemplate result = arguments[i];
                    result.isConst = type.isConst || result.isConst;
                    result.isPointer = type.isPointer || result.isPointer;
                    result.isRef = type.isRef || result.isRef;
                    result.isMoveRef = type.isMoveRef || result.isMoveRef;
                    result.isArray = type.isArray || result.isArray;
                    result.arraySize = type.arraySize > 0 ? type.arraySize : result.arraySize;
                    return result;
                }
            }
        }

        TypeRefTemplate result = type;
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
                                          std::vector<TypeRefTemplate> const& arguments)
    {
        TypeInfoParam param;
        TypeRefTemplate type = SubstituteTemplateType(paramTemplate.type, parameters, arguments);
        param.type = TypeID(type.ToCppString(false));
        param.name = paramTemplate.name;
        param.arraySize = type.isArray ? type.arraySize : 0;
        param.isPointer = type.isPointer;
        param.isConst = type.isConst;
        param.isRef = type.isRef;
        param.isOut = paramTemplate.isOut;
        param.defaultValue = paramTemplate.defaultValue;
        param.attributes = paramTemplate.attributes;
        param.marshalAs = paramTemplate.marshalAs;
        param.comment = paramTemplate.comment;
        return param;
    }

    static TypeInfoField InstantiateField(TypeInfoFieldTemplate const& fieldTemplate,
                                          std::vector<std::string> const& parameters,
                                          std::vector<TypeRefTemplate> const& arguments)
    {
        TypeInfoField field;
        TypeRefTemplate type = SubstituteTemplateType(fieldTemplate.type, parameters, arguments);
        field.isAPI = fieldTemplate.isAPI;
        field.isStatic = fieldTemplate.isStatic;
        field.type = TypeID(type.ToCppString(false));
        field.name = fieldTemplate.name;
        field.isReflect = fieldTemplate.isReflect;
        field.APIIsReadOnly = fieldTemplate.APIIsReadOnly;
        field.attributes = fieldTemplate.attributes;
        field.defaultValue = fieldTemplate.defaultValue;
        field.comment = fieldTemplate.comment;
        field.marshalAs = fieldTemplate.marshalAs;
        field.arraySize = type.isArray ? type.arraySize : 0;
        field.lineNumber = fieldTemplate.lineNumber;
        return field;
    }

    static TypeInfoFunc InstantiateFunction(TypeInfoFuncTemplate const& functionTemplate,
                                            std::vector<std::string> const& parameters,
                                            std::vector<TypeRefTemplate> const& arguments)
    {
        TypeInfoFunc fn;
        TypeRefTemplate returnType = SubstituteTemplateType(functionTemplate.returnType, parameters, arguments);
        fn.name = functionTemplate.name;
        fn.returnType = TypeID(returnType.ToCppString(false));
        fn.returnArraySize = returnType.isArray ? returnType.arraySize : 0;
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
                                          std::vector<TypeRefTemplate> const& arguments)
    {
        TypeInfoEvent evt;
        TypeRefTemplate cppType = SubstituteTemplateType(eventTemplate.cppType, parameters, arguments);
        evt.isReflect = eventTemplate.isReflect;
        evt.isAPI = eventTemplate.isAPI;
        evt.name = eventTemplate.name;
        evt.cppType = TypeID(cppType.ToCppString(false));
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

    static std::string GetFullTypeName(std::vector<std::string> const& namespaces,
                                       std::vector<std::string> const& structScopes,
                                       std::string const& name)
    {
        std::string result;
        if (!namespaces.empty())
        {
            result = Utils::CombineStringList(namespaces, "::");
        }
        if (!structScopes.empty())
        {
            if (!result.empty())
            {
                result += "::";
            }
            result += Utils::CombineStringList(structScopes, "::");
        }
        if (!result.empty())
        {
            result += "::";
        }
        result += name;
        return result;
    }

    static bool IsQualifiedTypeName(std::string const& name)
    {
        return name.find("::") != std::string::npos;
    }

    static std::string GetQualifiedTemplateInstantiationName(TypeInfoStructTemplate const& templateType,
                                                             TypeRefTemplate const&        targetType)
    {
        TypeRefTemplate qualifiedType = targetType;
        if (!IsQualifiedTypeName(qualifiedType.name))
        {
            qualifiedType.name = GetFullTypeName(templateType.namespaceScopeList,
                                                 templateType.structScopeList,
                                                 qualifiedType.name);
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

        std::string const fullAliasName =
            GetFullTypeName(templateType.namespaceScopeList, templateType.structScopeList, typeDef.name);
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
            TypeRefTemplate baseType = SubstituteTemplateType(templateType.baseType, parameters, arguments);
            type->baseClassName      = baseType.ToCppString(false);
            type->parentTypeID       = TypeID(type->baseClassName);
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
