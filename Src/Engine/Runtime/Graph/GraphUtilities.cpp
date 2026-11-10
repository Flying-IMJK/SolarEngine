
#include "GraphUtilities.h"

#include "ShaderGraphValue.h"
#include "Runtime/Core/Math/Transform.h"
#include "Runtime/Graphics/GlobalSettings_GPU.h"
#include "Runtime/Utilities/Variant.h"

namespace SE::GraphUtilities
{
    void ApplySomeMathHere(Variant& v, Variant& a, MathOp1 op)
    {
        v.SetType(a.Type);
        switch (a.Type.Type)
        {
        case VariantTypes::Bool:
            v.AsBool = op(a.AsBool ? 1.0f : 0.0f) > Math::ZeroTolerance;
            break;
        case VariantTypes::Int:
            v.AsInt = (int32)op((float)a.AsInt);
            break;
        case VariantTypes::Uint:
            v.AsUint = (uint32)op((float)a.AsUint);
            break;
        case VariantTypes::Float:
            v.AsFloat = op(a.AsFloat);
            break;
        case VariantTypes::Float2:
        {
            Float2& vv = *(Float2*)v.AsData;
            const Float2& aa = *(const Float2*)a.AsData;
            vv.X = op(aa.X);
            vv.Y = op(aa.Y);
            break;
        }
        case VariantTypes::Float3:
        {
            Float3& vv = *(Float3*)v.AsData;
            const Float3& aa = *(const Float3*)a.AsData;
            vv.X = op(aa.X);
            vv.Y = op(aa.Y);
            vv.Z = op(aa.Z);
            break;
        }
        case VariantTypes::Float4:
        case VariantTypes::Color:
        {
            Float4& vv = *(Float4*)v.AsData;
            const Float4& aa = *(const Float4*)a.AsData;
            vv.X = op(aa.X);
            vv.Y = op(aa.Y);
            vv.Z = op(aa.Z);
            vv.W = op(aa.W);
            break;
        }
        case VariantTypes::Double2:
        {
            Double2& vv = *(Double2*)v.AsData;
            const Double2& aa = *(const Double2*)a.AsData;
            vv.X = (double)op((float)aa.X);
            vv.Y = (double)op((float)aa.Y);
            break;
        }
        case VariantTypes::Double3:
        {
            Double3& vv = *(Double3*)v.AsData;
            const Double3& aa = *(const Double3*)a.AsData;
            vv.X = (double)op((float)aa.X);
            vv.Y = (double)op((float)aa.Y);
            vv.Z = (double)op((float)aa.Z);
            break;
        }
        case VariantTypes::Double4:
        {
            Double4& vv = *(Double4*)v.AsBlob.Data;
            const Double4& aa = *(const Double4*)a.AsBlob.Data;
            vv.X = (double)op((float)aa.X);
            vv.Y = (double)op((float)aa.Y);
            vv.Z = (double)op((float)aa.Z);
            vv.W = (double)op((float)aa.W);
            break;
        }
        case VariantTypes::Quaternion:
        {
            Quaternion& vv = *(Quaternion*)v.AsData;
            const Quaternion& aa = *(const Quaternion*)a.AsData;
            vv.X = op(aa.X);
            vv.Y = op(aa.Y);
            vv.Z = op(aa.Z);
            vv.W = op(aa.W);
            break;
        }
        case VariantTypes::Transform:
        {
            Transform& vv = *(Transform*)v.AsBlob.Data;
            const Transform& aa = *(const Transform*)a.AsBlob.Data;
            vv.Translation.X = op((float)aa.Translation.X);
            vv.Translation.Y = op((float)aa.Translation.Y);
            vv.Translation.Z = op((float)aa.Translation.Z);
            vv.Orientation.X = op(aa.Orientation.X);
            vv.Orientation.Y = op(aa.Orientation.Y);
            vv.Orientation.Z = op(aa.Orientation.Z);
            vv.Orientation.W = op(aa.Orientation.W);
            vv.Scale.X = op(aa.Scale.X);
            vv.Scale.Y = op(aa.Scale.Y);
            vv.Scale.Z = op(aa.Scale.Z);
            break;
        }
        default:
            v = a;
            break;
        }
    }

    void ApplySomeMathHere(Variant& v, Variant& a, Variant& b, MathOp2 op)
    {
        v.SetType(a.Type);
        switch (a.Type.Type)
        {
        case VariantTypes::Bool:
            v.AsBool = op(a.AsBool ? 1.0f : 0.0f, b.AsBool ? 1.0f : 0.0f) > Math::ZeroTolerance;
            break;
        case VariantTypes::Int:
            v.AsInt = (int32)op((float)a.AsInt, (float)b.AsInt);
            break;
        case VariantTypes::Uint:
            v.AsUint = (uint32)op((float)a.AsUint, (float)b.AsUint);
            break;
        case VariantTypes::Float:
            v.AsFloat = op(a.AsFloat, b.AsFloat);
            break;
        case VariantTypes::Float2:
        {
            Float2& vv = *(Float2*)v.AsData;
            const Float2& aa = *(const Float2*)a.AsData;
            const Float2& bb = *(const Float2*)b.AsData;
            vv.X = op(aa.X, bb.X);
            vv.Y = op(aa.Y, bb.Y);
            break;
        }
        case VariantTypes::Float3:
        {
            Float3& vv = *(Float3*)v.AsData;
            const Float3& aa = *(const Float3*)a.AsData;
            const Float3& bb = *(const Float3*)b.AsData;
            vv.X = op(aa.X, bb.X);
            vv.Y = op(aa.Y, bb.Y);
            vv.Z = op(aa.Z, bb.Z);
            break;
        }
        case VariantTypes::Float4:
        case VariantTypes::Color:
        {
            Float4& vv = *(Float4*)v.AsData;
            const Float4& aa = *(const Float4*)a.AsData;
            const Float4& bb = *(const Float4*)b.AsData;
            vv.X = op(aa.X, bb.X);
            vv.Y = op(aa.Y, bb.Y);
            vv.Z = op(aa.Z, bb.Z);
            vv.W = op(aa.W, bb.W);
            break;
        }
        case VariantTypes::Double2:
        {
            Double2& vv = *(Double2*)v.AsData;
            const Double2& aa = *(const Double2*)a.AsData;
            const Double2& bb = *(const Double2*)b.AsData;
            vv.X = (double)op((float)aa.X, (float)bb.X);
            vv.Y = (double)op((float)aa.Y, (float)bb.Y);
            break;
        }
        case VariantTypes::Double3:
        {
            Double3& vv = *(Double3*)v.AsData;
            const Double3& aa = *(const Double3*)a.AsData;
            const Double3& bb = *(const Double3*)b.AsData;
            vv.X = (double)op((float)aa.X, (float)bb.X);
            vv.Y = (double)op((float)aa.Y, (float)bb.Y);
            vv.Z = (double)op((float)aa.Z, (float)bb.Z);
            break;
        }
        case VariantTypes::Double4:
        {
            Double4& vv = *(Double4*)v.AsBlob.Data;
            const Double4& aa = *(const Double4*)a.AsBlob.Data;
            const Double4& bb = *(const Double4*)b.AsBlob.Data;
            vv.X = (double)op((float)aa.X, (float)bb.X);
            vv.Y = (double)op((float)aa.Y, (float)bb.Y);
            vv.Z = (double)op((float)aa.Z, (float)bb.Z);
            vv.W = (double)op((float)aa.W, (float)bb.W);
            break;
        }
        case VariantTypes::Quaternion:
        {
            Quaternion& vv = *(Quaternion*)v.AsData;
            const Quaternion& aa = *(const Quaternion*)a.AsData;
            const Quaternion& bb = *(const Quaternion*)b.AsData;
            vv.X = op(aa.X, bb.X);
            vv.Y = op(aa.Y, bb.Y);
            vv.Z = op(aa.Z, bb.Z);
            vv.W = op(aa.W, bb.W);
            break;
        }
        case VariantTypes::Transform:
        {
            Transform& vv = *(Transform*)v.AsBlob.Data;
            const Transform& aa = *(const Transform*)a.AsBlob.Data;
            const Transform& bb = *(const Transform*)b.AsBlob.Data;
            vv.Translation.X = op((float)aa.Translation.X, (float)bb.Translation.X);
            vv.Translation.Y = op((float)aa.Translation.Y, (float)bb.Translation.Y);
            vv.Translation.Z = op((float)aa.Translation.Z, (float)bb.Translation.Z);
            vv.Orientation.X = op(aa.Orientation.X, bb.Orientation.X);
            vv.Orientation.Y = op(aa.Orientation.Y, bb.Orientation.Y);
            vv.Orientation.Z = op(aa.Orientation.Z, bb.Orientation.Z);
            vv.Orientation.W = op(aa.Orientation.W, bb.Orientation.W);
            vv.Scale.X = op(aa.Scale.X, bb.Scale.X);
            vv.Scale.Y = op(aa.Scale.Y, bb.Scale.Y);
            vv.Scale.Z = op(aa.Scale.Z, bb.Scale.Z);
            break;
        }
        default:
            v = a;
            break;
        }
    }

    void ApplySomeMathHere(Variant& v, Variant& a, Variant& b, Variant& c, MathOp3 op)
    {
        v.SetType(a.Type);
        switch (a.Type.Type)
        {
        case VariantTypes::Bool:
            v.AsBool = op(a.AsBool ? 1.0f : 0.0f, b.AsBool ? 1.0f : 0.0f, c.AsBool ? 1.0f : 0.0f) > Math::ZeroTolerance;
            break;
        case VariantTypes::Int:
            v.AsInt = (int32)op((float)a.AsInt, (float)b.AsInt, (float)c.AsInt);
            break;
        case VariantTypes::Uint:
            v.AsUint = (int32)op((float)a.AsUint, (float)b.AsUint, (float)c.AsUint);
            break;
        case VariantTypes::Float:
            v.AsFloat = op(a.AsFloat, b.AsFloat, c.AsFloat);
            break;
        case VariantTypes::Float2:
        {
            Float2& vv = *(Float2*)v.AsData;
            const Float2& aa = *(const Float2*)a.AsData;
            const Float2& bb = *(const Float2*)b.AsData;
            const Float2& cc = *(const Float2*)b.AsData;
            vv.X = op(aa.X, bb.X, cc.X);
            vv.Y = op(aa.Y, bb.Y, cc.Y);
            break;
        }
        case VariantTypes::Float3:
        {
            Float3& vv = *(Float3*)v.AsData;
            const Float3& aa = *(const Float3*)a.AsData;
            const Float3& bb = *(const Float3*)b.AsData;
            const Float3& cc = *(const Float3*)b.AsData;
            vv.X = op(aa.X, bb.X, cc.X);
            vv.Y = op(aa.Y, bb.Y, cc.Y);
            vv.Z = op(aa.Z, bb.Z, cc.Z);
            break;
        }
        case VariantTypes::Float4:
        case VariantTypes::Color:
        {
            Float4& vv = *(Float4*)v.AsData;
            const Float4& aa = *(const Float4*)a.AsData;
            const Float4& bb = *(const Float4*)b.AsData;
            const Float4& cc = *(const Float4*)b.AsData;
            vv.X = op(aa.X, bb.X, cc.X);
            vv.Y = op(aa.Y, bb.Y, cc.Y);
            vv.Z = op(aa.Z, bb.Z, cc.Z);
            vv.W = op(aa.W, bb.W, cc.W);
            break;
        }
        case VariantTypes::Double2:
        {
            Double2& vv = *(Double2*)v.AsData;
            const Double2& aa = *(const Double2*)a.AsData;
            const Double2& bb = *(const Double2*)b.AsData;
            const Double2& cc = *(const Double2*)b.AsData;
            vv.X = (double)op((float)aa.X, (float)bb.X, (float)cc.X);
            vv.Y = (double)op((float)aa.Y, (float)bb.Y, (float)cc.Y);
            break;
        }
        case VariantTypes::Double3:
        {
            Double3& vv = *(Double3*)v.AsData;
            const Double3& aa = *(const Double3*)a.AsData;
            const Double3& bb = *(const Double3*)b.AsData;
            const Double3& cc = *(const Double3*)b.AsData;
            vv.X = (double)op((float)aa.X, (float)bb.X, (float)cc.X);
            vv.Y = (double)op((float)aa.Y, (float)bb.Y, (float)cc.Y);
            vv.Z = (double)op((float)aa.Z, (float)bb.Z, (float)cc.Z);
            break;
        }
        case VariantTypes::Double4:
        {
            Double4& vv = *(Double4*)v.AsBlob.Data;
            const Double4& aa = *(const Double4*)a.AsBlob.Data;
            const Double4& bb = *(const Double4*)b.AsBlob.Data;
            const Double4& cc = *(const Double4*)b.AsBlob.Data;
            vv.X = (double)op((float)aa.X, (float)bb.X, (float)cc.X);
            vv.Y = (double)op((float)aa.Y, (float)bb.Y, (float)cc.Y);
            vv.Z = (double)op((float)aa.Z, (float)bb.Z, (float)cc.Z);
            vv.W = (double)op((float)aa.W, (float)bb.W, (float)cc.W);
            break;
        }
        case VariantTypes::Quaternion:
        {
            Quaternion& vv = *(Quaternion*)v.AsData;
            const Quaternion& aa = *(const Quaternion*)a.AsData;
            const Quaternion& bb = *(const Quaternion*)b.AsData;
            const Quaternion& cc = *(const Quaternion*)b.AsData;
            vv.X = op(aa.X, bb.X, cc.X);
            vv.Y = op(aa.Y, bb.Y, cc.Y);
            vv.Z = op(aa.Z, bb.Z, cc.Z);
            vv.W = op(aa.W, bb.W, cc.W);
            break;
        }
        case VariantTypes::Transform:
        {
            Transform& vv = *(Transform*)v.AsBlob.Data;
            const Transform& aa = *(const Transform*)a.AsBlob.Data;
            const Transform& bb = *(const Transform*)b.AsBlob.Data;
            const Transform& cc = *(const Transform*)c.AsBlob.Data;
            vv.Translation.X = op((float)aa.Translation.X, (float)bb.Translation.X, (float)cc.Translation.X);
            vv.Translation.Y = op((float)aa.Translation.Y, (float)bb.Translation.Y, (float)cc.Translation.Y);
            vv.Translation.Z = op((float)aa.Translation.Z, (float)bb.Translation.Z, (float)cc.Translation.Z);
            vv.Orientation.X = op(aa.Orientation.X, bb.Orientation.X, cc.Orientation.X);
            vv.Orientation.Y = op(aa.Orientation.Y, bb.Orientation.Y, cc.Orientation.Y);
            vv.Orientation.Z = op(aa.Orientation.Z, bb.Orientation.Z, cc.Orientation.Z);
            vv.Orientation.W = op(aa.Orientation.W, bb.Orientation.W, cc.Orientation.W);
            vv.Scale.X = op(aa.Scale.X, bb.Scale.X, cc.Scale.X);
            vv.Scale.Y = op(aa.Scale.Y, bb.Scale.Y, cc.Scale.Y);
            vv.Scale.Z = op(aa.Scale.Z, bb.Scale.Z, cc.Scale.Z);
            break;
        }
        default:
            v = a;
            break;
        }
    }

    void ApplySomeMathHere(uint16 typeId, Variant& v, Variant& a)
    {
        // Select operation
        MathOp1 op;
        switch (typeId)
        {
        case 7:
            op = [](float a)
            {
                return Math::Abs(a);
            };
            break;
        case 8:
            op = [](float a)
            {
                return Math::Ceil(a);
            };
            break;
        case 9:
            op = [](float a)
            {
                return Math::Cos(a);
            };
            break;
        case 10:
            op = [](float a)
            {
                return Math::Floor(a);
            };
            break;
        case 13:
            op = [](float a)
            {
                return Math::Round(a);
            };
            break;
        case 14:
            op = [](float a)
            {
                return Math::Saturate(a);
            };
            break;
        case 15:
            op = [](float a)
            {
                return Math::Sin(a);
            };
            break;
        case 16:
            op = [](float a)
            {
                return Math::Sqrt(a);
            };
            break;
        case 17:
            op = [](float a)
            {
                return Math::Tan(a);
            };
            break;
        case 27:
            op = [](float a)
            {
                return -a;
            };
            break;
        case 28:
            op = [](float a)
            {
                return 1 - a;
            };
            break;
        case 33:
            op = [](float a)
            {
                return Math::ASin(a);
            };
            break;
        case 34:
            op = [](float a)
            {
                return Math::ACos(a);
            };
            break;
        case 35:
            op = [](float a)
            {
                return Math::ATan(a);
            };
            break;
        case 38:
            op = [](float a)
            {
                return Math::Trunc(a);
            };
            break;
        case 39:
            op = [](float a)
            {
                float tmp;
                return Math::ModF(a, tmp);
            };
            break;
        case 43:
        {
            op = [](float a)
            {
                return a * Math::RadiansToDegrees;
            };
            break;
        }
        case 44:
        {
            op = [](float a)
            {
                return a * Math::DegreesToRadians;
            };
            break;
        }

        default:
            return;
        }

        // Perform operation
        ApplySomeMathHere(v, a, op);
    }

    void ApplySomeMathHere(uint16 typeId, Variant& v, Variant& a, Variant& b)
    {
        // Select operation
        MathOp2 op;
        switch (typeId)
        {
        case 1:
            op = [](float a, float b)
            {
                return a + b;
            };
            break;
        case 2:
            op = [](float a, float b)
            {
                return a - b;
            };
            break;
        case 3:
            op = [](float a, float b)
            {
                return a * b;
            };
            break;
        case 4:
            op = [](float a, float b)
            {
                return (float)((int)a % (int)b);
            };
            break;
        case 5:
            op = [](float a, float b)
            {
                return a / b;
            };
            break;
        case 21:
            op = [](float a, float b)
            {
                return Math::Max(a, b);
            };
            break;
        case 22:
            op = [](float a, float b)
            {
                return Math::Min(a, b);
            };
            break;
        case 23:
            op = [](float a, float b)
            {
                return Math::Pow(a, b);
            };
            break;
        case 40:
            op = [](float a, float b)
            {
                return Math::ModF(a, b);
            };
            break;
        case 41:
            op = [](float a, float b)
            {
                return Math::ATan2(a, b);
            };
            break;
        default:
            return;
        }

        // Perform operation
        ApplySomeMathHere(v, a, b, op);
    }

    int32 CountComponents(VariantTypes type)
    {
        switch (type)
        {
        case VariantTypes::Bool:
        case VariantTypes::Int:
        case VariantTypes::Int64:
        case VariantTypes::Uint:
        case VariantTypes::Uint64:
        case VariantTypes::Float:
        case VariantTypes::Double:
        case VariantTypes::Pointer:
            return 1;
        case VariantTypes::Float2:
        case VariantTypes::Double2:
        case VariantTypes::Int2:
            return 2;
        case VariantTypes::Float3:
        case VariantTypes::Double3:
        case VariantTypes::Int3:
            return 3;
        case VariantTypes::Float4:
        case VariantTypes::Double4:
        case VariantTypes::Int4:
        case VariantTypes::Color:
            return 4;
        default:
            return 0;
        }
    }


    void GenerateShaderConstantBuffer(TextWriterUnicode& writer, List<SerializedMaterialParam>& parameters)
    {
        for (const SerializedMaterialParam& param : parameters)
        {
            const Char* type = nullptr;
            switch (param.Type)
            {
                case MaterialParameterType::Bool:
                    type = SE_TEXT("bool");
                    break;
                case MaterialParameterType::Integer:
                    type = SE_TEXT("int");
                    break;
                case MaterialParameterType::Float:
                    type = SE_TEXT("float");
                    break;
                case MaterialParameterType::Vector2:
                    type = SE_TEXT("float2");
                    break;
                case MaterialParameterType::Vector3:
                    type = SE_TEXT("float3");
                    break;
                case MaterialParameterType::Vector4:
                case MaterialParameterType::Color:
                case MaterialParameterType::ChannelMask:
                    type = SE_TEXT("float4");
                    break;
                case MaterialParameterType::Matrix:
                    type = SE_TEXT("float4x4");
                    break;
                default:
                    break;
            }
            if (type)
                writer.WriteLine(SE_TEXT("{0} {1};"), type, param.ShaderName);
        }
    }

    const Char* GenerateShaderResources(TextWriterUnicode& writer, List<SerializedMaterialParam>& parameters)
    {
        for (const SerializedMaterialParam& param : parameters)
        {
            const Char* type = nullptr;
            switch (param.Type)
            {
                case MaterialParameterType::NormalMap:
                case MaterialParameterType::GPUTexture:
                case MaterialParameterType::SceneTexture:
                case MaterialParameterType::Texture:
                    type = SE_TEXT("Texture2D");
                    break;
                case MaterialParameterType::GPUTextureCube:
                case MaterialParameterType::CubeTexture:
                    type = SE_TEXT("TextureCube");
                    break;
                case MaterialParameterType::GPUTextureArray:
                    type = SE_TEXT("Texture2DArray");
                    break;
                case MaterialParameterType::GPUTextureVolume:
                    type = SE_TEXT("Texture3D");
                    break;
                case MaterialParameterType::GameplayGlobal:
                case MaterialParameterType::GlobalSDF:
                    return SE_TEXT("This material parameter type has no SLC2 runtime provider.");
                default:
                    break;
            }
            if (type)
                writer.WriteLine(SE_TEXT("{0} {1};"), type, param.ShaderName);
        }
        return nullptr;
    }

    const Char* GenerateSamplers(TextWriterUnicode& writer, List<SerializedMaterialParam>& parameters)
    {
        for (const SerializedMaterialParam& param : parameters)
        {
            if (param.Type == MaterialParameterType::TextureGroupSampler)
                writer.WriteLine(SE_TEXT("SamplerState {0};"), param.ShaderName);
        }
        return nullptr;
    }

    template<typename T>
    const Char* GetTypename()
    {
        return SE_TEXT("");
    }

    template<>
    const Char* GetTypename<float>()
    {
        return SE_TEXT("float");
    }

    template<>
    const Char* GetTypename<Float2>()
    {
        return SE_TEXT("float2");
    }

    template<>
    const Char* GetTypename<Float3>()
    {
        return SE_TEXT("float3");
    }

    template<>
    const Char* GetTypename<Float4>()
    {
        return SE_TEXT("float4");
    }

    template<typename T>
    void SampleCurve(TextWriterUnicode& writer, const Math::BezierCurve<T>& curve, const String& time, const String& value)
    {
        const auto& keyframes = curve.GetKeyframes();

        if (keyframes.Count() == 0)
        {
            writer.Write(
                SE_TEXT(
                    "	{{\n"
                    "		// Curve ({1})\n"
                    "		{0} = 0;\n"
                    "	}}\n"
                ),
                value, // {0}
                GetTypename<T>() // {1}
            );
        }
        else if (keyframes.Count() == 1)
        {
            writer.Write(
                SE_TEXT(
                    "	{{\n"
                    "		// Curve ({1})\n"
                    "		{0} = {2};\n"
                    "	}}\n"
                ),
                value, // {0}
                GetTypename<T>(), // {1}
                ShaderGraphValue(keyframes[0].Value).Value // {2}
            );
        }
        else if (keyframes.Count() == 2)
        {
            writer.Write(
                SE_TEXT(
                    "	{{\n"
                    "		// Curve ({4})\n"
                    "		const float leftTime = {3};\n"
                    "		const float rightTime = {5};\n"
                    "		const float lengthTime = rightTime - leftTime;\n"
                    "		float time = clamp({0}, leftTime, rightTime);\n"
                    "		float alpha = lengthTime < 0.0000001 ? 0.0f : (time - leftTime) / lengthTime;\n"

                    "		const {4} leftValue = {6};\n"
                    "		const {4} rightValue = {7};\n"
                    "		const float oneThird = 1.0f / 3.0f;\n"
                    "		{4} leftTangent = leftValue + {8} * (lengthTime * oneThird);\n"
                    "		{4} rightTangent = rightValue + {1} * (lengthTime * oneThird);\n"

                    "		{4} p01 = lerp(leftValue, leftTangent, alpha);\n"
                    "		{4} p12 = lerp(leftTangent, rightTangent, alpha);\n"
                    "		{4} p23 = lerp(rightTangent, rightValue, alpha);\n"
                    "		{4} p012 = lerp(p01, p12, alpha);\n"
                    "		{4} p123 = lerp(p12, p23, alpha);\n"
                    "		{2} = lerp(p012, p123, alpha);\n"
                    "	}}\n"
                ),
                time, // {0}
                ShaderGraphValue(keyframes[1].TangentIn).Value, // {1}
                value, // {2}
                StringUtils::ToString(keyframes[0].Time), // {3}
                GetTypename<T>(), // {4}
                StringUtils::ToString(keyframes[1].Time), // {5}
                ShaderGraphValue(keyframes[0].Value).Value, // {6}
                ShaderGraphValue(keyframes[1].Value).Value, // {7}
                ShaderGraphValue(keyframes[0].TangentOut).Value // {8}
            );
        }
        else
        {
            StringBuilder keyframesTime, keyframesValue, keyframesTangentIn, keyframesTangentOut;
            for (int32 i = 0; i < keyframes.Count(); i++)
            {
                const auto& keyframe = keyframes[i];
                if (i != 0)
                {
                    keyframesTime.Append(',');
                    keyframesValue.Append(',');
                    keyframesTangentIn.Append(',');
                    keyframesTangentOut.Append(',');
                }
                keyframesTime.Append(StringUtils::ToString(keyframe.Time));
                keyframesValue.Append(ShaderGraphValue(keyframe.Value).Value);
                keyframesTangentIn.Append(ShaderGraphValue(keyframe.TangentIn).Value);
                keyframesTangentOut.Append(ShaderGraphValue(keyframe.TangentOut).Value);
            }
            keyframesTime.Append('\0');
            keyframesValue.Append('\0');
            keyframesTangentIn.Append('\0');
            keyframesTangentOut.Append('\0');

            writer.Write(
                SE_TEXT(
                    "	{{\n"
                    "		// Curve ({4})\n"
                    "		int count = {0};\n"
                    "		float time = clamp({1}, 0.0, {2});\n"

                    "		static float keyframesTime[] = {{ {5} }};\n"
                    "		static {4} keyframesValue[] = {{ {6} }};\n"
                    "		static {4} keyframesTangentIn[] = {{ {7} }};\n"
                    "		static {4} keyframesTangentOut[] = {{ {8} }};\n"

                    "		int start = 0;\n"
                    "		int searchLength = count;\n"
                    "		while (searchLength > 0)\n"
                    "		{{\n"
                    "			int halfPos = searchLength >> 1;\n"
                    "			int midPos = start + halfPos;\n"
                    "			if (time < keyframesTime[midPos])\n"
                    "			{{\n"
                    "				searchLength = halfPos;\n"
                    "			}}\n"
                    "			else\n"
                    "			{{\n"
                    "				start = midPos + 1;\n"
                    "				searchLength -= halfPos + 1;\n"
                    "			}}\n"
                    "		}}\n"
                    "		int leftKey = max(0, start - 1);\n"
                    "		int rightKey = min(start, count - 1);\n"

                    "		const float leftTime = keyframesTime[leftKey];\n"
                    "		const float rightTime = keyframesTime[rightKey];\n"
                    "		const float lengthTime = rightTime - leftTime;\n"
                    "		float alpha = lengthTime < 0.0000001 ? 0.0f : (time - leftTime) / lengthTime;\n"

                    "		const {4} leftValue = keyframesValue[leftKey];\n"
                    "		const {4} rightValue = keyframesValue[rightKey];\n"
                    "		const float oneThird = 1.0f / 3.0f;\n"
                    "		{4} leftTangent = leftValue + keyframesTangentOut[leftKey] * (lengthTime * oneThird);\n"
                    "		{4} rightTangent = rightValue + keyframesTangentIn[rightKey] * (lengthTime * oneThird);\n"

                    "		{4} p01 = lerp(leftValue, leftTangent, alpha);\n"
                    "		{4} p12 = lerp(leftTangent, rightTangent, alpha);\n"
                    "		{4} p23 = lerp(rightTangent, rightValue, alpha);\n"
                    "		{4} p012 = lerp(p01, p12, alpha);\n"
                    "		{4} p123 = lerp(p12, p23, alpha);\n"
                    "		{3} = lerp(p012, p123, alpha);\n"
                    "	}}\n"
                ),
                keyframes.Count(), // {0}
                time, // {1}
                curve.GetLength(), // {2}
                value, // {3}
                GetTypename<T>(), // {4}
                *keyframesTime, // {5}
                *keyframesValue, // {6}
                *keyframesTangentIn, // {7}
                *keyframesTangentOut // {8}
            );
        }
    }

    template void SampleCurve(TextWriterUnicode& writer, const Math::BezierCurve<float>& curve, const String& time, const String& value);
    template void SampleCurve(TextWriterUnicode& writer, const Math::BezierCurve<Float2>& curve, const String& time, const String& value);
    template void SampleCurve(TextWriterUnicode& writer, const Math::BezierCurve<Float3>& curve, const String& time, const String& value);
    template void SampleCurve(TextWriterUnicode& writer, const Math::BezierCurve<Float4>& curve, const String& time, const String& value);
} // SE
