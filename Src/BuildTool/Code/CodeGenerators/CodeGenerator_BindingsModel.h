#pragma once

#include "CodeGenerator_BindingsDataTypes.h"

namespace SE::BuildTool
{
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

    BindingCallable MakePropertyGetter(const TypeInfoStruct& cls, const TypeInfoFunc& function);
    BindingCallable MakePropertySetter(const TypeInfoStruct& cls, const TypeInfoFunc& function);
    BindingCallable MakeBindingFieldGetter(const TypeInfoStruct& cls, const TypeInfoField& field);
    BindingCallable MakeBindingFieldSetter(const TypeInfoStruct& cls, const TypeInfoField& field);
    std::string     GetEnumUnderlyingTypeName(Utils::TypeIDCore underlyingType);
}
