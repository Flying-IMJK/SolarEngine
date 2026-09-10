#include "CodeGenerator_BindingsModel.h"

namespace SE::BuildTool
{
    BindingCallable MakePropertyGetter(const TypeInfoStruct& cls, const TypeInfoFunc& fn)
    {
        BindingCallable result;
        result.function.name       = fn.name;
        result.function.returnType = fn.returnType;
        result.function.isStatic   = fn.isStatic;
        result.function.uniqueName = fn.uniqueName;
        result.function.entryPoint = fn.entryPoint;
        result.function.access     = fn.access;
        result.function.attributes = fn.attributes;
        result.function.comment    = fn.comment;
        result.function.marshalAs  = fn.marshalAs;
        result.function.lineNumber = fn.lineNumber;
        result.invocation = BindingInvocationKind::Method;
        return result;
    }

    BindingCallable MakePropertySetter(const TypeInfoStruct& cls, const TypeInfoFunc& fn)
    {
        BindingCallable result;
        result.function.name       = fn.name;
        result.function.returnType = TypeInfo::Void;
        result.function.isStatic   = fn.isStatic;
        result.function.uniqueName = fn.uniqueName;
        result.function.entryPoint = fn.entryPoint;
        result.function.access     = fn.access;
        result.function.attributes = fn.attributes;
        result.function.comment    = fn.comment;
        result.function.marshalAs  = fn.marshalAs;
        result.function.lineNumber = fn.lineNumber;

        TypeInfoParam& value = Utils::Vector::AddOne(result.function.params);
        value.name           = "value";
        value.type           = fn.params[0].type;
        value.marshalAs      = fn.params[0].marshalAs;
        value.direction      = fn.params[0].direction;
        result.invocation    = BindingInvocationKind::Method;
        return result;
    }

    BindingCallable MakeBindingFieldGetter(const TypeInfoStruct& cls, const TypeInfoField& field)
    {
        BindingCallable result;
        result.function.name = field.name;
        result.function.returnType = field.type;
        result.function.isStatic = field.isStatic;
        result.function.uniqueName = field.name + "_Get";
        result.function.entryPoint = Utils::String::Format("{0}_{1}_Get", cls.name, field.name);
        result.function.attributes = field.attributes;
        result.function.comment = field.comment;
        result.function.marshalAs = field.marshalAs;
        result.function.lineNumber = field.lineNumber;
        result.invocation = BindingInvocationKind::FieldGet;
        return result;
    }

    BindingCallable MakeBindingFieldSetter(const TypeInfoStruct& cls, const TypeInfoField& field)
    {
        BindingCallable result;
        result.function.name       = field.name;
        result.function.returnType = TypeInfo::Void;
        result.function.isStatic   = field.isStatic;
        result.function.uniqueName = field.name + "_Set";
        result.function.entryPoint = Utils::String::Format("{0}_{1}_Set", cls.name, field.name);
        result.function.attributes = field.attributes;
        result.function.comment    = field.comment;
        result.function.marshalAs  = field.marshalAs;
        result.function.lineNumber = field.lineNumber;

        TypeInfoParam& value = Utils::Vector::AddOne(result.function.params);
        value.name           = "value";
        value.type           = field.type;
        value.marshalAs      = field.marshalAs;
        result.invocation    = BindingInvocationKind::FieldSet;

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
}
