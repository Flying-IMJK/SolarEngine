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

    // A callable is the common ABI surface shared by methods and fields. The
    // TypeInfoFunc owns the managed/native signature while the kind only
    // determines how the native expression is formed.
    enum class BindingInvocationKind
    {
        Method,
        FieldGet,
        FieldSet,
    };

    struct BindingCallable
    {
        TypeInfoFunc function;
        BindingInvocationKind invocation = BindingInvocationKind::Method;
    };

    BindingTypeInfo ResolveBindingType(const std::string& cppType);
    BindingCallable MakeBindingMethodCallable(const TypeInfoFunc& function);
    BindingCallable MakeBindingPropertyGetter(const TypeInfoStruct& cls, const TypeInfoFunc& function);
    BindingCallable MakeBindingPropertySetter(const TypeInfoStruct& cls, const TypeInfoFunc& function);
    BindingCallable MakeBindingFieldGetter(const TypeInfoStruct& cls, const TypeInfoField& field);
    BindingCallable MakeBindingFieldSetter(const TypeInfoStruct& cls, const TypeInfoField& field);
    std::string     GetEnumUnderlyingTypeName(Utils::TypeIDCore underlyingType);
    void            HashCombine(BindingGenerationContext& context, const std::string& value);
    void            AddBindingDiagnostic(BindingGenerationContext& context, const std::string& message,
                                         const std::string& filePath = std::string(), int lineNumber = -1);
}
