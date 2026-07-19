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
            const TypeMapping* mapping = FindTypeMapping(stripped.c_str());
            if (mapping && mapping->isString)
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

    std::string MakeCSharpIdentifier(const std::string& identifier)
    {
        static const char* keywords[] =
        {
            "abstract", "as", "base", "bool", "break", "byte", "case", "catch", "char",
            "checked", "class", "const", "continue", "decimal", "default", "delegate",
            "do", "double", "else", "enum", "event", "explicit", "extern", "false",
            "finally", "fixed", "float", "for", "foreach", "goto", "if", "implicit",
            "in", "int", "interface", "internal", "is", "lock", "long", "namespace",
            "new", "null", "object", "operator", "out", "override", "params", "private",
            "protected", "public", "readonly", "ref", "return", "sbyte", "sealed",
            "short", "sizeof", "stackalloc", "static", "string", "struct", "switch",
            "this", "throw", "true", "try", "typeof", "uint", "ulong", "unchecked",
            "unsafe", "ushort", "using", "virtual", "void", "volatile", "while", nullptr
        };

        for (int i = 0; keywords[i] != nullptr; i++)
        {
            if (identifier == keywords[i])
                return Utils::String::Format("@{0}", identifier);
        }
        return identifier;
    }

    std::string EscapeCSharpXml(const std::string& text)
    {
        std::string result = text;
        Utils::String::ReplaceAll(result, "&", "&amp;");
        Utils::String::ReplaceAll(result, "<", "&lt;");
        Utils::String::ReplaceAll(result, ">", "&gt;");
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
