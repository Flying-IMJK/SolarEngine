#pragma once

#include "CodeGenerator_BindingsDataTypes.h"

namespace SE::BuildTool
{
    enum class BindingTypeKind
    {
        Unknown,
        Void,
        Primitive,
        String,
        Enum,
        Struct,
        ScriptingObject,
        ObjectReference,
        Collection,
    };

    struct BindingTypeInfo
    {
        std::string      originalType;
        std::string      normalizedType;
        std::string      baseType;
        std::vector<std::string> genericArgs;
        BindingTypeKind kind = BindingTypeKind::Unknown;
        bool            isConst = false;
        bool            isPointer = false;
        bool            isReference = false;
        bool            isMoveReference = false;
        bool            isArray = false;
        int             arraySize = 0;
        bool            passByReference = false;
        bool            useCustomMarshalling = false;
        std::string      csharpPublicType;
        std::string      csharpInteropType;
    };

    struct BindingDiagnostic
    {
        std::string message;
        std::string filePath;
        int        lineNumber = -1;
    };

    struct BindingGenerationContext
    {
        std::vector<BindingDiagnostic> diagnostics;
        uint64                  inputHash = 1469598103934665603ull;
    };

    BindingTypeInfo ResolveBindingType(const std::string& cppType);
    std::string     MakeCSharpIdentifier(const std::string& identifier);
    std::string     EscapeCSharpXml(const std::string& text);
    std::string     GetEnumUnderlyingTypeName(Utils::TypeIDCore underlyingType);
    void            HashCombine(BindingGenerationContext& context, const std::string& value);
    void            AddBindingDiagnostic(BindingGenerationContext& context, const std::string& message,
                                         const std::string& filePath = std::string(), int lineNumber = -1);
}
