#include "CodeGenerator_BindingsModel.h"
#include "CodeGenerator_BindingsTypeMap.h"

#include <cstring>

namespace SE::BuildTool
{
    namespace
    {
        bool IsKnownPrimitive(const std::string& type)
        {
            const TypeMapping* mapping = FindTypeMapping(type.c_str());
            return mapping != nullptr && !mapping->isString && !mapping->isObject && type != "void";
        }

        void ParseTemplateArguments(const std::string& text, std::vector<std::string>& args)
        {
            int ltPos = Utils::String::Find(text, "<");
            int gtPos = Utils::String::FindLast(text, '>');
            if (ltPos == INVALID_INDEX || gtPos == INVALID_INDEX || gtPos <= ltPos)
            {
                return;
            }

            int depth = 0;
            int start = ltPos + 1;
            const char* raw = text.c_str();
            for (int i = ltPos + 1; i < gtPos; i++)
            {
                if (raw[i] == '<')
                    depth++;
                else if (raw[i] == '>')
                    depth--;
                else if (raw[i] == ',' && depth == 0)
                {
                    std::string arg = StripTypeQualifiers(text.substr(start, i - start));
                    if (!arg.empty())
                        args.push_back(arg);
                    start = i + 1;
                }
            }

            std::string last = StripTypeQualifiers(text.substr(start, gtPos - start));
            if (!last.empty())
                args.push_back(last);
        }
    }

    BindingTypeInfo ResolveBindingType(const std::string& cppType)
    {
        BindingTypeInfo result;
        result.originalType = cppType;

        CppTypeInfo cpp;
        cpp.Parse(cppType);
        result.baseType = cpp.baseType;
        result.isConst = cpp.isConst;
        result.isPointer = cpp.isPointer;
        result.isReference = cpp.isRef;
        result.isMoveReference = cpp.isMoveRef;
        result.isArray = cpp.isArray;
        result.arraySize = cpp.arraySize;
        result.genericArgs = cpp.genericArgs;
        result.normalizedType = cpp.ToString();
        if (result.normalizedType.empty())
            result.normalizedType = StripTypeQualifiers(cppType);

        std::string stripped = StripTypeQualifiers(cppType);
        if (stripped == "void")
        {
            result.kind = BindingTypeKind::Void;
        }
        else if (IsCollectionType(cppType))
        {
            result.kind = BindingTypeKind::Collection;
            if (result.genericArgs.empty())
                ParseTemplateArguments(stripped, result.genericArgs);
        }
        else if (IsObjectTypeRef(cppType))
        {
            result.kind = BindingTypeKind::ObjectReference;
        }
        else if (IsScriptingObjectPointer(cppType) || IsScriptingObjectType(cppType))
        {
            result.kind = BindingTypeKind::ScriptingObject;
        }
        else
        {
            if (IsStringType(cppType))
                result.kind = BindingTypeKind::String;
            else if (IsKnownPrimitive(stripped))
                result.kind = BindingTypeKind::Primitive;
            else
                result.kind = BindingTypeKind::Struct;
        }

        result.passByReference = UsePassByReference(cppType);
        result.useCustomMarshalling = !IsPodType(cppType)
            && result.kind != BindingTypeKind::Void
            && result.kind != BindingTypeKind::Primitive
            && result.kind != BindingTypeKind::String
            && result.kind != BindingTypeKind::ScriptingObject;
        result.csharpPublicType = GetCSharpPublicType(cppType);
        result.csharpInteropType = GetCSharpInteropType(cppType);
        return result;
    }

    BindingCallable MakeBindingMethodCallable(const ApiFunction& function)
    {
        BindingCallable result;
        result.function = function;
        result.invocation = BindingInvocationKind::Method;
        return result;
    }

    BindingCallable MakeBindingPropertyGetter(const ApiProperty& property)
    {
        BindingCallable result;
        result.function.name = property.getterName;
        result.function.returnType = property.getterCppType.empty() ? property.cppType : property.getterCppType;
        result.function.isStatic = property.isStatic;
        result.function.uniqueName = property.getterUniqueName;
        result.function.entryPoint = property.getterEntryPoint;
        result.function.isHidden = property.isHidden;
        result.function.isDeprecated = property.isDeprecated;
        result.function.access = property.getterAccess;
        result.function.attributes = property.attributes;
        result.function.comment = property.comment;
        result.function.marshalAs = property.marshalAs;
        result.function.lineNumber = property.lineNumber;
        result.invocation = BindingInvocationKind::Method;
        return result;
    }

    BindingCallable MakeBindingPropertySetter(const ApiProperty& property)
    {
        BindingCallable result;
        result.function.name = property.setterName;
        result.function.returnType = "void";
        result.function.isStatic = property.isStatic;
        result.function.uniqueName = property.setterUniqueName;
        result.function.entryPoint = property.setterEntryPoint;
        result.function.isHidden = property.isHidden;
        result.function.isDeprecated = property.isDeprecated;
        result.function.access = property.setterAccess;
        result.function.attributes = property.attributes;
        result.function.comment = property.comment;
        result.function.marshalAs = property.marshalAs;
        result.function.lineNumber = property.lineNumber;
        ApiParam& value = Utils::Vector::AddOne(result.function.params);
        value.name = "value";
        value.cppType = property.setterCppType.empty() ? property.cppType : property.setterCppType;
        result.invocation = BindingInvocationKind::Method;
        return result;
    }

    BindingCallable MakeBindingFieldGetter(const ApiClass& cls, const ApiField& field)
    {
        BindingCallable result;
        result.function.name = field.name;
        result.function.returnType = field.cppType;
        result.function.returnArraySize = field.arraySize;
        result.function.isStatic = field.isStatic;
        result.function.uniqueName = field.name + "_Get";
        result.function.entryPoint = Utils::String::Format("{0}_{1}_Get", cls.name, field.name);
        result.function.isHidden = field.isHidden;
        result.function.isDeprecated = field.isDeprecated;
        result.function.attributes = field.attributes;
        result.function.comment = field.comment;
        result.function.marshalAs = field.marshalAs;
        result.function.lineNumber = field.lineNumber;
        result.invocation = BindingInvocationKind::FieldGet;
        return result;
    }

    BindingCallable MakeBindingFieldSetter(const ApiClass& cls, const ApiField& field)
    {
        BindingCallable result;
        result.function.name = field.name;
        result.function.returnType = "void";
        result.function.isStatic = field.isStatic;
        result.function.uniqueName = field.name + "_Set";
        result.function.entryPoint = Utils::String::Format("{0}_{1}_Set", cls.name, field.name);
        result.function.isHidden = field.isHidden;
        result.function.isDeprecated = field.isDeprecated;
        result.function.attributes = field.attributes;
        result.function.comment = field.comment;
        result.function.marshalAs = field.marshalAs;
        result.function.lineNumber = field.lineNumber;
        ApiParam& value = Utils::Vector::AddOne(result.function.params);
        value.name = "value";
        value.cppType = field.cppType;
        value.arraySize = field.arraySize;
        result.invocation = BindingInvocationKind::FieldSet;
        return result;
    }

    std::string GetEnumUnderlyingTypeName(Utils::TypeIDCore underlyingType)
    {
        switch (underlyingType)
        {
        case Utils::TypeIDCore::Int8:   return "int8";
        case Utils::TypeIDCore::Uint8:  return "uint8";
        case Utils::TypeIDCore::Int16:  return "int16";
        case Utils::TypeIDCore::Uint16: return "uint16";
        case Utils::TypeIDCore::Int32:  return "int32";
        case Utils::TypeIDCore::Uint32: return "uint32";
        default:                 return "int32";
        }
    }

    void HashCombine(BindingGenerationContext& context, const std::string& value)
    {
        const uint64 prime = 1099511628211ull;
        const char* raw = value.c_str();
        for (int i = 0; raw != nullptr && raw[i] != 0; i++)
        {
            context.inputHash ^= (uint8)raw[i];
            context.inputHash *= prime;
        }
    }

    void AddBindingDiagnostic(BindingGenerationContext& context, const std::string& message,
                              const std::string& filePath, int lineNumber)
    {
        BindingDiagnostic& diagnostic = Utils::Vector::AddOne(context.diagnostics);
        diagnostic.message = message;
        diagnostic.filePath = filePath;
        diagnostic.lineNumber = lineNumber;
    }
}
