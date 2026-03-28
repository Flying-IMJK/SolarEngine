
#include "ShaderGraphValue.h"

#include "Core/Math/Quaternion.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"

namespace SE
{
    const Char* ShaderGraphValue::_subs[] =
    {
        SE_TEXT(".x"),
        SE_TEXT(".y"),
        SE_TEXT(".z"),
        SE_TEXT(".w")
    };

    const ShaderGraphValue ShaderGraphValue::Zero(VariantTypeHandle::Types::Float, SE_TEXT("0.0"));
    const ShaderGraphValue ShaderGraphValue::Half(VariantTypeHandle::Types::Float, SE_TEXT("0.5"));
    const ShaderGraphValue ShaderGraphValue::One(VariantTypeHandle::Types::Float, SE_TEXT("1.0"));
    const ShaderGraphValue ShaderGraphValue::True(VariantTypeHandle::Types::Bool, SE_TEXT("true"));
    const ShaderGraphValue ShaderGraphValue::False(VariantTypeHandle::Types::Bool, SE_TEXT("false"));

    ShaderGraphValue::ShaderGraphValue(const Variant& v)
    {
        switch (v.Type.Type)
        {
        case VariantTypeHandle::Types::Bool:
            Type = VariantTypeHandle::Types::Bool;
            Value = v.AsBool ? SE_TEXT('1') : SE_TEXT('0');
            break;
        case VariantTypeHandle::Types::Int:
            Type = VariantTypeHandle::Types::Int;
            Value = StringUtils::ToString(v.AsInt);
            break;
        case VariantTypeHandle::Types::Uint:
            Type = VariantTypeHandle::Types::Uint;
            Value = StringUtils::ToString(v.AsUint);
            break;
        case VariantTypeHandle::Types::Float:
            Type = VariantTypeHandle::Types::Float;
            Value = String::Format(SE_TEXT("{}"), v.AsFloat);
            if (Value.Find('.') == -1)
                Value = String::Format(SE_TEXT("{:.1f}"), v.AsFloat);
            break;
        case VariantTypeHandle::Types::Double:
            Type = VariantTypeHandle::Types::Float;
            Value = String::Format(SE_TEXT("{}"), (float)v.AsDouble);
            if (Value.Find('.') == -1)
                Value = String::Format(SE_TEXT("{:.1f}"), (float)v.AsDouble);
            break;
        case VariantTypeHandle::Types::Float2:
        {
            const auto vv = v.AsFloat2();
            Type = VariantTypeHandle::Types::Float2;
            Value = String::Format(SE_TEXT("float2({0}, {1})"), vv.x, vv.y);
            break;
        }
        case VariantTypeHandle::Types::Float3:
        {
            const auto vv = v.AsFloat3();
            Type = VariantTypeHandle::Types::Float3;
            Value = String::Format(SE_TEXT("float3({0}, {1}, {2})"), vv.x, vv.y, vv.z);
            break;
        }
        case VariantTypeHandle::Types::Float4:
        case VariantTypeHandle::Types::Color:
        {
            const auto vv = v.AsFloat4();
            Type = VariantTypeHandle::Types::Float4;
            Value = String::Format(SE_TEXT("float4({0}, {1}, {2}, {3})"), vv.x, vv.y, vv.z, vv.w);
            break;
        }
        case VariantTypeHandle::Types::Double2:
        {
            const auto vv = (::SE::Float2)v.AsDouble2();
            Type = VariantTypeHandle::Types::Float2;
            Value = String::Format(SE_TEXT("float2({0}, {1})"), vv.x, vv.y);
            break;
        }
        case VariantTypeHandle::Types::Double3:
        {
            const auto vv = (::SE::Float3)v.AsDouble3();
            Type = VariantTypeHandle::Types::Float3;
            Value = String::Format(SE_TEXT("float3({0}, {1}, {2})"), vv.x, vv.y, vv.z);
            break;
        }
        case VariantTypeHandle::Types::Double4:
        {
            const auto vv = (::SE::Float4)v.AsDouble4();
            Type = VariantTypeHandle::Types::Float4;
            Value = String::Format(SE_TEXT("float4({0}, {1}, {2}, {3})"), vv.x, vv.y, vv.z, vv.w);
            break;
        }
        case VariantTypeHandle::Types::Quaternion:
        {
            const auto vv = v.AsQuaternion();
            Type = VariantTypeHandle::Types::Quaternion;
            Value = String::Format(SE_TEXT("float4({0}, {1}, {2}, {3})"), vv.x, vv.y, vv.z, vv.w);
            break;
        }
        case VariantTypeHandle::Types::String:
            Type = VariantTypeHandle::Types::String;
            Value = (StringView)v;
            break;
        default:
            Type = VariantTypeHandle::Types::Null;
            break;
        }
    }

    bool ShaderGraphValue::IsZero() const
    {
        switch (Type)
        {
        case VariantTypeHandle::Types::Bool:
        case VariantTypeHandle::Types::Int:
        case VariantTypeHandle::Types::Uint:
        case VariantTypeHandle::Types::Float:
            return Value == SE_TEXT("0") || Value == SE_TEXT("0.0");
        default:
            return false;
        }
    }

    bool ShaderGraphValue::IsOne() const
    {
        switch (Type)
        {
        case VariantTypeHandle::Types::Bool:
        case VariantTypeHandle::Types::Int:
        case VariantTypeHandle::Types::Uint:
        case VariantTypeHandle::Types::Float:
            return Value == SE_TEXT("1") || Value == SE_TEXT("1.0");
        default:
            return false;
        }
    }

    bool ShaderGraphValue::IsLiteral() const
    {
        switch (Type)
        {
        case VariantTypeHandle::Types::Bool:
        case VariantTypeHandle::Types::Int:
        case VariantTypeHandle::Types::Uint:
        case VariantTypeHandle::Types::Float:
            if (Value.HasChars())
            {
                for (int32 i = 0; i < Value.Length(); i++)
                {
                    const Char c = Value[i];
                    if (!StringUtils::IsDigit(c) && c != '.')
                        return false;
                }
                return true;
            }
        default:
            return false;
        }
    }

    ShaderGraphValue ShaderGraphValue::InitForZero(VariantTypeHandle::Types type)
    {
        const Char* v;
        switch (type)
        {
        case VariantTypeHandle::Types::Float:
        case VariantTypeHandle::Types::Double:
            v = SE_TEXT("0.0");
            break;
        case VariantTypeHandle::Types::Bool:
        case VariantTypeHandle::Types::Int:
        case VariantTypeHandle::Types::Uint:
            v = SE_TEXT("0");
            break;
        case VariantTypeHandle::Types::Float2:
        case VariantTypeHandle::Types::Double2:
            v = SE_TEXT("float2(0, 0)");
            break;
        case VariantTypeHandle::Types::Float3:
        case VariantTypeHandle::Types::Double3:
            v = SE_TEXT("float3(0, 0, 0)");
            break;
        case VariantTypeHandle::Types::Float4:
        case VariantTypeHandle::Types::Double4:
        case VariantTypeHandle::Types::Color:
            v = SE_TEXT("float4(0, 0, 0, 0)");
            break;
        case VariantTypeHandle::Types::Quaternion:
            v = SE_TEXT("float4(0, 0, 0, 1)");
            break;
        case VariantTypeHandle::Types::Void:
            v = SE_TEXT("((Material)0)");
            break;
        default:
            ENGINE_UNREACHABLE_CODE();
            v = nullptr;
        }
        return ShaderGraphValue(type, v);
    }

    ShaderGraphValue ShaderGraphValue::InitForHalf(VariantTypeHandle::Types type)
    {
        const Char* v;
        switch (type)
        {
        case VariantTypeHandle::Types::Float:
        case VariantTypeHandle::Types::Double:
            v = SE_TEXT("0.5");
            break;
        case VariantTypeHandle::Types::Bool:
        case VariantTypeHandle::Types::Int:
        case VariantTypeHandle::Types::Uint:
            v = SE_TEXT("0");
            break;
        case VariantTypeHandle::Types::Float2:
        case VariantTypeHandle::Types::Double2:
            v = SE_TEXT("float2(0.5, 0.5)");
            break;
        case VariantTypeHandle::Types::Float3:
        case VariantTypeHandle::Types::Double3:
            v = SE_TEXT("float3(0.5, 0.5, 0.5)");
            break;
        case VariantTypeHandle::Types::Float4:
        case VariantTypeHandle::Types::Double4:
        case VariantTypeHandle::Types::Quaternion:
        case VariantTypeHandle::Types::Color:
            v = SE_TEXT("float4(0.5, 0.5, 0.5, 0.5)");
            break;
        default:
            ENGINE_UNREACHABLE_CODE();
            v = nullptr;
        }
        return ShaderGraphValue(type, String(v));
    }

    ShaderGraphValue ShaderGraphValue::InitForOne(VariantTypeHandle::Types type)
    {
        const Char* v;
        switch (type)
        {
        case VariantTypeHandle::Types::Float:
        case VariantTypeHandle::Types::Double:
            v = SE_TEXT("1.0");
            break;
        case VariantTypeHandle::Types::Bool:
        case VariantTypeHandle::Types::Int:
        case VariantTypeHandle::Types::Uint:
            v = SE_TEXT("1");
            break;
        case VariantTypeHandle::Types::Float2:
        case VariantTypeHandle::Types::Double2:
            v = SE_TEXT("float2(1, 1)");
            break;
        case VariantTypeHandle::Types::Float3:
        case VariantTypeHandle::Types::Double3:
            v = SE_TEXT("float3(1, 1, 1)");
            break;
        case VariantTypeHandle::Types::Float4:
        case VariantTypeHandle::Types::Double4:
        case VariantTypeHandle::Types::Quaternion:
        case VariantTypeHandle::Types::Color:
            v = SE_TEXT("float4(1, 1, 1, 1)");
            break;
        default:
            ENGINE_UNREACHABLE_CODE();
            v = nullptr;
        }
        return ShaderGraphValue(type, String(v));
    }

    ShaderGraphValue ShaderGraphValue::GetY() const
    {
        switch (Type)
        {
        case VariantTypeHandle::Types::Float2:
        case VariantTypeHandle::Types::Float3:
        case VariantTypeHandle::Types::Float4:
        case VariantTypeHandle::Types::Double2:
        case VariantTypeHandle::Types::Double3:
        case VariantTypeHandle::Types::Double4:
            return ShaderGraphValue(VariantTypeHandle::Types::Float, Value + _subs[1]);
        default:
            return Zero;
        }
    }

    ShaderGraphValue ShaderGraphValue::GetZ() const
    {
        switch (Type)
        {
        case VariantTypeHandle::Types::Float3:
        case VariantTypeHandle::Types::Float4:
        case VariantTypeHandle::Types::Double3:
        case VariantTypeHandle::Types::Double4:
            return ShaderGraphValue(VariantTypeHandle::Types::Float, Value + _subs[2]);
        default:
            return Zero;
        }
    }

    ShaderGraphValue ShaderGraphValue::GetW() const
    {
        switch (Type)
        {
        case VariantTypeHandle::Types::Float4:
        case VariantTypeHandle::Types::Double4:
            return ShaderGraphValue(VariantTypeHandle::Types::Float, Value + _subs[3]);
        default:
            return One;
        }
    }

    ShaderGraphValue ShaderGraphValue::Cast(const ShaderGraphValue& v, VariantTypeHandle::Types to)
    {
        // If they are the same types or input value is empty, then just return value
        if (v.Type == to || v.Value.IsEmpty())
        {
            return v;
        }

        // Select format string
        const Char* format = nullptr;
        switch (to)
        {
        case VariantTypeHandle::Types::Bool:
            switch (v.Type)
            {
        case VariantTypeHandle::Types::Int:
        case VariantTypeHandle::Types::Uint:
        case VariantTypeHandle::Types::Float:
        case VariantTypeHandle::Types::Double:
            format = SE_TEXT("((bool){0})");
                break;
        case VariantTypeHandle::Types::Float2:
        case VariantTypeHandle::Types::Float3:
        case VariantTypeHandle::Types::Float4:
        case VariantTypeHandle::Types::Double2:
        case VariantTypeHandle::Types::Double3:
        case VariantTypeHandle::Types::Double4:
        case VariantTypeHandle::Types::Quaternion:
        case VariantTypeHandle::Types::Color:
            format = SE_TEXT("((bool){0}.x)");
                break;
            }
            break;
        case VariantTypeHandle::Types::Int:
            switch (v.Type)
            {
        case VariantTypeHandle::Types::Bool:
        case VariantTypeHandle::Types::Uint:
        case VariantTypeHandle::Types::Float:
        case VariantTypeHandle::Types::Double:
            format = SE_TEXT("((int){0})");
                break;
        case VariantTypeHandle::Types::Float2:
        case VariantTypeHandle::Types::Float3:
        case VariantTypeHandle::Types::Float4:
        case VariantTypeHandle::Types::Double2:
        case VariantTypeHandle::Types::Double3:
        case VariantTypeHandle::Types::Double4:
        case VariantTypeHandle::Types::Quaternion:
        case VariantTypeHandle::Types::Color:
            format = SE_TEXT("((int){0}.x)");
                break;
            }
            break;
        case VariantTypeHandle::Types::Uint:
            switch (v.Type)
            {
        case VariantTypeHandle::Types::Bool:
        case VariantTypeHandle::Types::Int:
        case VariantTypeHandle::Types::Float:
        case VariantTypeHandle::Types::Double:
            format = SE_TEXT("((uint){0})");
                break;
        case VariantTypeHandle::Types::Float2:
        case VariantTypeHandle::Types::Float3:
        case VariantTypeHandle::Types::Float4:
        case VariantTypeHandle::Types::Double2:
        case VariantTypeHandle::Types::Double3:
        case VariantTypeHandle::Types::Double4:
        case VariantTypeHandle::Types::Quaternion:
        case VariantTypeHandle::Types::Color:
            format = SE_TEXT("((uint){0}.x)");
                break;
            }
            break;
        case VariantTypeHandle::Types::Float:
        case VariantTypeHandle::Types::Double:
            switch (v.Type)
            {
        case VariantTypeHandle::Types::Bool:
        case VariantTypeHandle::Types::Int:
        case VariantTypeHandle::Types::Uint:
            format = SE_TEXT("((float){0})");
                break;
        case VariantTypeHandle::Types::Float2:
        case VariantTypeHandle::Types::Float3:
        case VariantTypeHandle::Types::Float4:
        case VariantTypeHandle::Types::Double2:
        case VariantTypeHandle::Types::Double3:
        case VariantTypeHandle::Types::Double4:
        case VariantTypeHandle::Types::Quaternion:
        case VariantTypeHandle::Types::Color:
            format = SE_TEXT("((float){0}.x)");
                break;
            }
            break;
        case VariantTypeHandle::Types::Float2:
        case VariantTypeHandle::Types::Double2:
            switch (v.Type)
            {
        case VariantTypeHandle::Types::Bool:
        case VariantTypeHandle::Types::Int:
        case VariantTypeHandle::Types::Uint:
        case VariantTypeHandle::Types::Float:
        case VariantTypeHandle::Types::Double:
            format = SE_TEXT("float2({0}, {0})");
                break;
        case VariantTypeHandle::Types::Float3:
        case VariantTypeHandle::Types::Float4:
        case VariantTypeHandle::Types::Double2:
        case VariantTypeHandle::Types::Double3:
        case VariantTypeHandle::Types::Double4:
        case VariantTypeHandle::Types::Quaternion:
        case VariantTypeHandle::Types::Color:
            format = SE_TEXT("{0}.xy");
                break;
            }
            break;
        case VariantTypeHandle::Types::Float3:
        case VariantTypeHandle::Types::Double3:
            switch (v.Type)
            {
        case VariantTypeHandle::Types::Bool:
        case VariantTypeHandle::Types::Int:
        case VariantTypeHandle::Types::Uint:
        case VariantTypeHandle::Types::Float:
        case VariantTypeHandle::Types::Double:
            format = SE_TEXT("float3({0}, {0}, {0})");
                break;
        case VariantTypeHandle::Types::Float2:
        case VariantTypeHandle::Types::Double2:
            format = SE_TEXT("float3({0}.xy, 0)");
                break;
        case VariantTypeHandle::Types::Double3:
        case VariantTypeHandle::Types::Float4:
        case VariantTypeHandle::Types::Double4:
        case VariantTypeHandle::Types::Color:
            format = SE_TEXT("{0}.xyz");
                break;
        case VariantTypeHandle::Types::Quaternion:
            format = SE_TEXT("QuatRotateVector({0}, float3(0, 0, 1))"); // Returns direction vector
                break;
            }
            break;
        case VariantTypeHandle::Types::Float4:
        case VariantTypeHandle::Types::Double4:
        case VariantTypeHandle::Types::Color:
            switch (v.Type)
            {
        case VariantTypeHandle::Types::Bool:
        case VariantTypeHandle::Types::Int:
        case VariantTypeHandle::Types::Uint:
        case VariantTypeHandle::Types::Float:
        case VariantTypeHandle::Types::Double:
            format = SE_TEXT("float4({0}, {0}, {0}, {0})");
                break;
        case VariantTypeHandle::Types::Float2:
        case VariantTypeHandle::Types::Double2:
            format = SE_TEXT("float4({0}.xy, 0, 0)");
                break;
        case VariantTypeHandle::Types::Float3:
        case VariantTypeHandle::Types::Double3:
            format = SE_TEXT("float4({0}.xyz, 0)");
                break;
        case VariantTypeHandle::Types::Color:
        case VariantTypeHandle::Types::Float4:
        case VariantTypeHandle::Types::Double4:
        case VariantTypeHandle::Types::Quaternion:
            format = SE_TEXT("{}");
                break;
            }
            break;
        case VariantTypeHandle::Types::Quaternion:
            switch (v.Type)
            {
        case VariantTypeHandle::Types::Color:
        case VariantTypeHandle::Types::Float4:
        case VariantTypeHandle::Types::Double4:
            format = SE_TEXT("{}");
                break;
            }
            break;
        }
        if (format == nullptr)
        {
            LOG_ERROR("Graph", "Failed to cast shader graph value of type {0} to {1}", VariantTypeHandle(v.Type).ToString(), VariantTypeHandle(to).ToString());
            return Zero;
        }

        return ShaderGraphValue(to, String::Format(format, v.Value));
    }
} // SE