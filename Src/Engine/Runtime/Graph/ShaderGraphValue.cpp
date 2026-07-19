
#include "ShaderGraphValue.h"

#include "Runtime/Core/Math/Quaternion.h"
#include "Runtime/Core/Math/Vector2.h"
#include "Runtime/Core/Math/Vector3.h"
#include "Runtime/Core/Math/Vector4.h"

namespace SE
{
    const Char* ShaderGraphValue::_subs[] =
    {
        SE_TEXT(".x"),
        SE_TEXT(".y"),
        SE_TEXT(".z"),
        SE_TEXT(".w")
    };

    const ShaderGraphValue ShaderGraphValue::Zero(VariantTypes::Float, SE_TEXT("0.0"));
    const ShaderGraphValue ShaderGraphValue::Half(VariantTypes::Float, SE_TEXT("0.5"));
    const ShaderGraphValue ShaderGraphValue::One(VariantTypes::Float, SE_TEXT("1.0"));
    const ShaderGraphValue ShaderGraphValue::True(VariantTypes::Bool, SE_TEXT("true"));
    const ShaderGraphValue ShaderGraphValue::False(VariantTypes::Bool, SE_TEXT("false"));

    ShaderGraphValue::ShaderGraphValue(const Variant& v)
    {
        switch (v.Type.Type)
        {
        case VariantTypes::Bool:
            Type = VariantTypes::Bool;
            Value = v.AsBool ? SE_TEXT('1') : SE_TEXT('0');
            break;
        case VariantTypes::Int:
            Type = VariantTypes::Int;
            Value = StringUtils::ToString(v.AsInt);
            break;
        case VariantTypes::Uint:
            Type = VariantTypes::Uint;
            Value = StringUtils::ToString(v.AsUint);
            break;
        case VariantTypes::Float:
            Type = VariantTypes::Float;
            Value = String::Format(SE_TEXT("{}"), v.AsFloat);
            if (Value.Find('.') == -1)
                Value = String::Format(SE_TEXT("{:.1f}"), v.AsFloat);
            break;
        case VariantTypes::Double:
            Type = VariantTypes::Float;
            Value = String::Format(SE_TEXT("{}"), (float)v.AsDouble);
            if (Value.Find('.') == -1)
                Value = String::Format(SE_TEXT("{:.1f}"), (float)v.AsDouble);
            break;
        case VariantTypes::Float2:
        {
            const auto vv = v.AsFloat2();
            Type = VariantTypes::Float2;
            Value = String::Format(SE_TEXT("float2({0}, {1})"), vv.x, vv.y);
            break;
        }
        case VariantTypes::Float3:
        {
            const auto vv = v.AsFloat3();
            Type = VariantTypes::Float3;
            Value = String::Format(SE_TEXT("float3({0}, {1}, {2})"), vv.x, vv.y, vv.z);
            break;
        }
        case VariantTypes::Float4:
        case VariantTypes::Color:
        {
            const auto vv = v.AsFloat4();
            Type = VariantTypes::Float4;
            Value = String::Format(SE_TEXT("float4({0}, {1}, {2}, {3})"), vv.x, vv.y, vv.z, vv.w);
            break;
        }
        case VariantTypes::Double2:
        {
            const auto vv = (::SE::Float2)v.AsDouble2();
            Type = VariantTypes::Float2;
            Value = String::Format(SE_TEXT("float2({0}, {1})"), vv.x, vv.y);
            break;
        }
        case VariantTypes::Double3:
        {
            const auto vv = (::SE::Float3)v.AsDouble3();
            Type = VariantTypes::Float3;
            Value = String::Format(SE_TEXT("float3({0}, {1}, {2})"), vv.x, vv.y, vv.z);
            break;
        }
        case VariantTypes::Double4:
        {
            const auto vv = (::SE::Float4)v.AsDouble4();
            Type = VariantTypes::Float4;
            Value = String::Format(SE_TEXT("float4({0}, {1}, {2}, {3})"), vv.x, vv.y, vv.z, vv.w);
            break;
        }
        case VariantTypes::Quaternion:
        {
            const auto vv = v.AsQuaternion();
            Type = VariantTypes::Quaternion;
            Value = String::Format(SE_TEXT("float4({0}, {1}, {2}, {3})"), vv.x, vv.y, vv.z, vv.w);
            break;
        }
        case VariantTypes::String:
            Type = VariantTypes::String;
            Value = (StringView)v;
            break;
        default:
            Type = VariantTypes::Null;
            break;
        }
    }

    bool ShaderGraphValue::IsZero() const
    {
        switch (Type)
        {
        case VariantTypes::Bool:
        case VariantTypes::Int:
        case VariantTypes::Uint:
        case VariantTypes::Float:
            return Value == SE_TEXT("0") || Value == SE_TEXT("0.0");
        default:
            return false;
        }
    }

    bool ShaderGraphValue::IsOne() const
    {
        switch (Type)
        {
        case VariantTypes::Bool:
        case VariantTypes::Int:
        case VariantTypes::Uint:
        case VariantTypes::Float:
            return Value == SE_TEXT("1") || Value == SE_TEXT("1.0");
        default:
            return false;
        }
    }

    bool ShaderGraphValue::IsLiteral() const
    {
        switch (Type)
        {
        case VariantTypes::Bool:
        case VariantTypes::Int:
        case VariantTypes::Uint:
        case VariantTypes::Float:
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

    ShaderGraphValue ShaderGraphValue::InitForZero(VariantTypes type)
    {
        const Char* v;
        switch (type)
        {
        case VariantTypes::Float:
        case VariantTypes::Double:
            v = SE_TEXT("0.0");
            break;
        case VariantTypes::Bool:
        case VariantTypes::Int:
        case VariantTypes::Uint:
            v = SE_TEXT("0");
            break;
        case VariantTypes::Float2:
        case VariantTypes::Double2:
            v = SE_TEXT("float2(0, 0)");
            break;
        case VariantTypes::Float3:
        case VariantTypes::Double3:
            v = SE_TEXT("float3(0, 0, 0)");
            break;
        case VariantTypes::Float4:
        case VariantTypes::Double4:
        case VariantTypes::Color:
            v = SE_TEXT("float4(0, 0, 0, 0)");
            break;
        case VariantTypes::Quaternion:
            v = SE_TEXT("float4(0, 0, 0, 1)");
            break;
        case VariantTypes::Void:
            v = SE_TEXT("((Material)0)");
            break;
        default:
            ENGINE_UNREACHABLE_CODE();
            v = nullptr;
        }
        return ShaderGraphValue(type, v);
    }

    ShaderGraphValue ShaderGraphValue::InitForHalf(VariantTypes type)
    {
        const Char* v;
        switch (type)
        {
        case VariantTypes::Float:
        case VariantTypes::Double:
            v = SE_TEXT("0.5");
            break;
        case VariantTypes::Bool:
        case VariantTypes::Int:
        case VariantTypes::Uint:
            v = SE_TEXT("0");
            break;
        case VariantTypes::Float2:
        case VariantTypes::Double2:
            v = SE_TEXT("float2(0.5, 0.5)");
            break;
        case VariantTypes::Float3:
        case VariantTypes::Double3:
            v = SE_TEXT("float3(0.5, 0.5, 0.5)");
            break;
        case VariantTypes::Float4:
        case VariantTypes::Double4:
        case VariantTypes::Quaternion:
        case VariantTypes::Color:
            v = SE_TEXT("float4(0.5, 0.5, 0.5, 0.5)");
            break;
        default:
            ENGINE_UNREACHABLE_CODE();
            v = nullptr;
        }
        return ShaderGraphValue(type, String(v));
    }

    ShaderGraphValue ShaderGraphValue::InitForOne(VariantTypes type)
    {
        const Char* v;
        switch (type)
        {
        case VariantTypes::Float:
        case VariantTypes::Double:
            v = SE_TEXT("1.0");
            break;
        case VariantTypes::Bool:
        case VariantTypes::Int:
        case VariantTypes::Uint:
            v = SE_TEXT("1");
            break;
        case VariantTypes::Float2:
        case VariantTypes::Double2:
            v = SE_TEXT("float2(1, 1)");
            break;
        case VariantTypes::Float3:
        case VariantTypes::Double3:
            v = SE_TEXT("float3(1, 1, 1)");
            break;
        case VariantTypes::Float4:
        case VariantTypes::Double4:
        case VariantTypes::Quaternion:
        case VariantTypes::Color:
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
        case VariantTypes::Float2:
        case VariantTypes::Float3:
        case VariantTypes::Float4:
        case VariantTypes::Double2:
        case VariantTypes::Double3:
        case VariantTypes::Double4:
            return ShaderGraphValue(VariantTypes::Float, Value + _subs[1]);
        default:
            return Zero;
        }
    }

    ShaderGraphValue ShaderGraphValue::GetZ() const
    {
        switch (Type)
        {
        case VariantTypes::Float3:
        case VariantTypes::Float4:
        case VariantTypes::Double3:
        case VariantTypes::Double4:
            return ShaderGraphValue(VariantTypes::Float, Value + _subs[2]);
        default:
            return Zero;
        }
    }

    ShaderGraphValue ShaderGraphValue::GetW() const
    {
        switch (Type)
        {
        case VariantTypes::Float4:
        case VariantTypes::Double4:
            return ShaderGraphValue(VariantTypes::Float, Value + _subs[3]);
        default:
            return One;
        }
    }

    ShaderGraphValue ShaderGraphValue::Cast(const ShaderGraphValue& v, VariantTypes to)
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
        case VariantTypes::Bool:
            switch (v.Type)
            {
        case VariantTypes::Int:
        case VariantTypes::Uint:
        case VariantTypes::Float:
        case VariantTypes::Double:
            format = SE_TEXT("((bool){0})");
                break;
        case VariantTypes::Float2:
        case VariantTypes::Float3:
        case VariantTypes::Float4:
        case VariantTypes::Double2:
        case VariantTypes::Double3:
        case VariantTypes::Double4:
        case VariantTypes::Quaternion:
        case VariantTypes::Color:
            format = SE_TEXT("((bool){0}.x)");
                break;
            }
            break;
        case VariantTypes::Int:
            switch (v.Type)
            {
        case VariantTypes::Bool:
        case VariantTypes::Uint:
        case VariantTypes::Float:
        case VariantTypes::Double:
            format = SE_TEXT("((int){0})");
                break;
        case VariantTypes::Float2:
        case VariantTypes::Float3:
        case VariantTypes::Float4:
        case VariantTypes::Double2:
        case VariantTypes::Double3:
        case VariantTypes::Double4:
        case VariantTypes::Quaternion:
        case VariantTypes::Color:
            format = SE_TEXT("((int){0}.x)");
                break;
            }
            break;
        case VariantTypes::Uint:
            switch (v.Type)
            {
        case VariantTypes::Bool:
        case VariantTypes::Int:
        case VariantTypes::Float:
        case VariantTypes::Double:
            format = SE_TEXT("((uint){0})");
                break;
        case VariantTypes::Float2:
        case VariantTypes::Float3:
        case VariantTypes::Float4:
        case VariantTypes::Double2:
        case VariantTypes::Double3:
        case VariantTypes::Double4:
        case VariantTypes::Quaternion:
        case VariantTypes::Color:
            format = SE_TEXT("((uint){0}.x)");
                break;
            }
            break;
        case VariantTypes::Float:
        case VariantTypes::Double:
            switch (v.Type)
            {
        case VariantTypes::Bool:
        case VariantTypes::Int:
        case VariantTypes::Uint:
            format = SE_TEXT("((float){0})");
                break;
        case VariantTypes::Float2:
        case VariantTypes::Float3:
        case VariantTypes::Float4:
        case VariantTypes::Double2:
        case VariantTypes::Double3:
        case VariantTypes::Double4:
        case VariantTypes::Quaternion:
        case VariantTypes::Color:
            format = SE_TEXT("((float){0}.x)");
                break;
            }
            break;
        case VariantTypes::Float2:
        case VariantTypes::Double2:
            switch (v.Type)
            {
        case VariantTypes::Bool:
        case VariantTypes::Int:
        case VariantTypes::Uint:
        case VariantTypes::Float:
        case VariantTypes::Double:
            format = SE_TEXT("float2({0}, {0})");
                break;
        case VariantTypes::Float3:
        case VariantTypes::Float4:
        case VariantTypes::Double2:
        case VariantTypes::Double3:
        case VariantTypes::Double4:
        case VariantTypes::Quaternion:
        case VariantTypes::Color:
            format = SE_TEXT("{0}.xy");
                break;
            }
            break;
        case VariantTypes::Float3:
        case VariantTypes::Double3:
            switch (v.Type)
            {
        case VariantTypes::Bool:
        case VariantTypes::Int:
        case VariantTypes::Uint:
        case VariantTypes::Float:
        case VariantTypes::Double:
            format = SE_TEXT("float3({0}, {0}, {0})");
                break;
        case VariantTypes::Float2:
        case VariantTypes::Double2:
            format = SE_TEXT("float3({0}.xy, 0)");
                break;
        case VariantTypes::Double3:
        case VariantTypes::Float4:
        case VariantTypes::Double4:
        case VariantTypes::Color:
            format = SE_TEXT("{0}.xyz");
                break;
        case VariantTypes::Quaternion:
            format = SE_TEXT("QuatRotateVector({0}, float3(0, 0, 1))"); // Returns direction vector
                break;
            }
            break;
        case VariantTypes::Float4:
        case VariantTypes::Double4:
        case VariantTypes::Color:
            switch (v.Type)
            {
        case VariantTypes::Bool:
        case VariantTypes::Int:
        case VariantTypes::Uint:
        case VariantTypes::Float:
        case VariantTypes::Double:
            format = SE_TEXT("float4({0}, {0}, {0}, {0})");
                break;
        case VariantTypes::Float2:
        case VariantTypes::Double2:
            format = SE_TEXT("float4({0}.xy, 0, 0)");
                break;
        case VariantTypes::Float3:
        case VariantTypes::Double3:
            format = SE_TEXT("float4({0}.xyz, 0)");
                break;
        case VariantTypes::Color:
        case VariantTypes::Float4:
        case VariantTypes::Double4:
        case VariantTypes::Quaternion:
            format = SE_TEXT("{}");
                break;
            }
            break;
        case VariantTypes::Quaternion:
            switch (v.Type)
            {
        case VariantTypes::Color:
        case VariantTypes::Float4:
        case VariantTypes::Double4:
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