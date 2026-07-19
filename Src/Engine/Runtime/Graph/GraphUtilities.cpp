
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
            vv.x = op(aa.x);
            vv.y = op(aa.y);
            break;
        }
        case VariantTypes::Float3:
        {
            Float3& vv = *(Float3*)v.AsData;
            const Float3& aa = *(const Float3*)a.AsData;
            vv.x = op(aa.x);
            vv.y = op(aa.y);
            vv.z = op(aa.z);
            break;
        }
        case VariantTypes::Float4:
        case VariantTypes::Color:
        {
            Float4& vv = *(Float4*)v.AsData;
            const Float4& aa = *(const Float4*)a.AsData;
            vv.x = op(aa.x);
            vv.y = op(aa.y);
            vv.z = op(aa.z);
            vv.w = op(aa.w);
            break;
        }
        case VariantTypes::Double2:
        {
            Double2& vv = *(Double2*)v.AsData;
            const Double2& aa = *(const Double2*)a.AsData;
            vv.x = (double)op((float)aa.x);
            vv.y = (double)op((float)aa.y);
            break;
        }
        case VariantTypes::Double3:
        {
            Double3& vv = *(Double3*)v.AsData;
            const Double3& aa = *(const Double3*)a.AsData;
            vv.x = (double)op((float)aa.x);
            vv.y = (double)op((float)aa.y);
            vv.z = (double)op((float)aa.z);
            break;
        }
        case VariantTypes::Double4:
        {
            Double4& vv = *(Double4*)v.AsBlob.Data;
            const Double4& aa = *(const Double4*)a.AsBlob.Data;
            vv.x = (double)op((float)aa.x);
            vv.y = (double)op((float)aa.y);
            vv.z = (double)op((float)aa.z);
            vv.w = (double)op((float)aa.w);
            break;
        }
        case VariantTypes::Quaternion:
        {
            Quaternion& vv = *(Quaternion*)v.AsData;
            const Quaternion& aa = *(const Quaternion*)a.AsData;
            vv.x = op(aa.x);
            vv.y = op(aa.y);
            vv.z = op(aa.z);
            vv.w = op(aa.w);
            break;
        }
        case VariantTypes::Transform:
        {
            Transform& vv = *(Transform*)v.AsBlob.Data;
            const Transform& aa = *(const Transform*)a.AsBlob.Data;
            vv.Translation.x = op((float)aa.Translation.x);
            vv.Translation.y = op((float)aa.Translation.y);
            vv.Translation.z = op((float)aa.Translation.z);
            vv.Orientation.x = op(aa.Orientation.x);
            vv.Orientation.y = op(aa.Orientation.y);
            vv.Orientation.z = op(aa.Orientation.z);
            vv.Orientation.w = op(aa.Orientation.w);
            vv.Scale.x = op(aa.Scale.x);
            vv.Scale.y = op(aa.Scale.y);
            vv.Scale.z = op(aa.Scale.z);
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
            vv.x = op(aa.x, bb.x);
            vv.y = op(aa.y, bb.y);
            break;
        }
        case VariantTypes::Float3:
        {
            Float3& vv = *(Float3*)v.AsData;
            const Float3& aa = *(const Float3*)a.AsData;
            const Float3& bb = *(const Float3*)b.AsData;
            vv.x = op(aa.x, bb.x);
            vv.y = op(aa.y, bb.y);
            vv.z = op(aa.z, bb.z);
            break;
        }
        case VariantTypes::Float4:
        case VariantTypes::Color:
        {
            Float4& vv = *(Float4*)v.AsData;
            const Float4& aa = *(const Float4*)a.AsData;
            const Float4& bb = *(const Float4*)b.AsData;
            vv.x = op(aa.x, bb.x);
            vv.y = op(aa.y, bb.y);
            vv.z = op(aa.z, bb.z);
            vv.w = op(aa.w, bb.w);
            break;
        }
        case VariantTypes::Double2:
        {
            Double2& vv = *(Double2*)v.AsData;
            const Double2& aa = *(const Double2*)a.AsData;
            const Double2& bb = *(const Double2*)b.AsData;
            vv.x = (double)op((float)aa.x, (float)bb.x);
            vv.y = (double)op((float)aa.y, (float)bb.y);
            break;
        }
        case VariantTypes::Double3:
        {
            Double3& vv = *(Double3*)v.AsData;
            const Double3& aa = *(const Double3*)a.AsData;
            const Double3& bb = *(const Double3*)b.AsData;
            vv.x = (double)op((float)aa.x, (float)bb.x);
            vv.y = (double)op((float)aa.y, (float)bb.y);
            vv.z = (double)op((float)aa.z, (float)bb.z);
            break;
        }
        case VariantTypes::Double4:
        {
            Double4& vv = *(Double4*)v.AsBlob.Data;
            const Double4& aa = *(const Double4*)a.AsBlob.Data;
            const Double4& bb = *(const Double4*)b.AsBlob.Data;
            vv.x = (double)op((float)aa.x, (float)bb.x);
            vv.y = (double)op((float)aa.y, (float)bb.y);
            vv.z = (double)op((float)aa.z, (float)bb.z);
            vv.w = (double)op((float)aa.w, (float)bb.w);
            break;
        }
        case VariantTypes::Quaternion:
        {
            Quaternion& vv = *(Quaternion*)v.AsData;
            const Quaternion& aa = *(const Quaternion*)a.AsData;
            const Quaternion& bb = *(const Quaternion*)b.AsData;
            vv.x = op(aa.x, bb.x);
            vv.y = op(aa.y, bb.y);
            vv.z = op(aa.z, bb.z);
            vv.w = op(aa.w, bb.w);
            break;
        }
        case VariantTypes::Transform:
        {
            Transform& vv = *(Transform*)v.AsBlob.Data;
            const Transform& aa = *(const Transform*)a.AsBlob.Data;
            const Transform& bb = *(const Transform*)b.AsBlob.Data;
            vv.Translation.x = op((float)aa.Translation.x, (float)bb.Translation.x);
            vv.Translation.y = op((float)aa.Translation.y, (float)bb.Translation.y);
            vv.Translation.z = op((float)aa.Translation.z, (float)bb.Translation.z);
            vv.Orientation.x = op(aa.Orientation.x, bb.Orientation.x);
            vv.Orientation.y = op(aa.Orientation.y, bb.Orientation.y);
            vv.Orientation.z = op(aa.Orientation.z, bb.Orientation.z);
            vv.Orientation.w = op(aa.Orientation.w, bb.Orientation.w);
            vv.Scale.x = op(aa.Scale.x, bb.Scale.x);
            vv.Scale.y = op(aa.Scale.y, bb.Scale.y);
            vv.Scale.z = op(aa.Scale.z, bb.Scale.z);
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
            vv.x = op(aa.x, bb.x, cc.x);
            vv.y = op(aa.y, bb.y, cc.y);
            break;
        }
        case VariantTypes::Float3:
        {
            Float3& vv = *(Float3*)v.AsData;
            const Float3& aa = *(const Float3*)a.AsData;
            const Float3& bb = *(const Float3*)b.AsData;
            const Float3& cc = *(const Float3*)b.AsData;
            vv.x = op(aa.x, bb.x, cc.x);
            vv.y = op(aa.y, bb.y, cc.y);
            vv.z = op(aa.z, bb.z, cc.z);
            break;
        }
        case VariantTypes::Float4:
        case VariantTypes::Color:
        {
            Float4& vv = *(Float4*)v.AsData;
            const Float4& aa = *(const Float4*)a.AsData;
            const Float4& bb = *(const Float4*)b.AsData;
            const Float4& cc = *(const Float4*)b.AsData;
            vv.x = op(aa.x, bb.x, cc.x);
            vv.y = op(aa.y, bb.y, cc.y);
            vv.z = op(aa.z, bb.z, cc.z);
            vv.w = op(aa.w, bb.w, cc.w);
            break;
        }
        case VariantTypes::Double2:
        {
            Double2& vv = *(Double2*)v.AsData;
            const Double2& aa = *(const Double2*)a.AsData;
            const Double2& bb = *(const Double2*)b.AsData;
            const Double2& cc = *(const Double2*)b.AsData;
            vv.x = (double)op((float)aa.x, (float)bb.x, (float)cc.x);
            vv.y = (double)op((float)aa.y, (float)bb.y, (float)cc.y);
            break;
        }
        case VariantTypes::Double3:
        {
            Double3& vv = *(Double3*)v.AsData;
            const Double3& aa = *(const Double3*)a.AsData;
            const Double3& bb = *(const Double3*)b.AsData;
            const Double3& cc = *(const Double3*)b.AsData;
            vv.x = (double)op((float)aa.x, (float)bb.x, (float)cc.x);
            vv.y = (double)op((float)aa.y, (float)bb.y, (float)cc.y);
            vv.z = (double)op((float)aa.z, (float)bb.z, (float)cc.z);
            break;
        }
        case VariantTypes::Double4:
        {
            Double4& vv = *(Double4*)v.AsBlob.Data;
            const Double4& aa = *(const Double4*)a.AsBlob.Data;
            const Double4& bb = *(const Double4*)b.AsBlob.Data;
            const Double4& cc = *(const Double4*)b.AsBlob.Data;
            vv.x = (double)op((float)aa.x, (float)bb.x, (float)cc.x);
            vv.y = (double)op((float)aa.y, (float)bb.y, (float)cc.y);
            vv.z = (double)op((float)aa.z, (float)bb.z, (float)cc.z);
            vv.w = (double)op((float)aa.w, (float)bb.w, (float)cc.w);
            break;
        }
        case VariantTypes::Quaternion:
        {
            Quaternion& vv = *(Quaternion*)v.AsData;
            const Quaternion& aa = *(const Quaternion*)a.AsData;
            const Quaternion& bb = *(const Quaternion*)b.AsData;
            const Quaternion& cc = *(const Quaternion*)b.AsData;
            vv.x = op(aa.x, bb.x, cc.x);
            vv.y = op(aa.y, bb.y, cc.y);
            vv.z = op(aa.z, bb.z, cc.z);
            vv.w = op(aa.w, bb.w, cc.w);
            break;
        }
        case VariantTypes::Transform:
        {
            Transform& vv = *(Transform*)v.AsBlob.Data;
            const Transform& aa = *(const Transform*)a.AsBlob.Data;
            const Transform& bb = *(const Transform*)b.AsBlob.Data;
            const Transform& cc = *(const Transform*)c.AsBlob.Data;
            vv.Translation.x = op((float)aa.Translation.x, (float)bb.Translation.x, (float)cc.Translation.x);
            vv.Translation.y = op((float)aa.Translation.y, (float)bb.Translation.y, (float)cc.Translation.y);
            vv.Translation.z = op((float)aa.Translation.z, (float)bb.Translation.z, (float)cc.Translation.z);
            vv.Orientation.x = op(aa.Orientation.x, bb.Orientation.x, cc.Orientation.x);
            vv.Orientation.y = op(aa.Orientation.y, bb.Orientation.y, cc.Orientation.y);
            vv.Orientation.z = op(aa.Orientation.z, bb.Orientation.z, cc.Orientation.z);
            vv.Orientation.w = op(aa.Orientation.w, bb.Orientation.w, cc.Orientation.w);
            vv.Scale.x = op(aa.Scale.x, bb.Scale.x, cc.Scale.x);
            vv.Scale.y = op(aa.Scale.y, bb.Scale.y, cc.Scale.y);
            vv.Scale.z = op(aa.Scale.z, bb.Scale.z, cc.Scale.z);
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
        int32 constantsOffset = 0;
        int32 paddingIndex = 0;
        for (int32 i = 0; i < parameters.Count(); i++)
        {
            auto& param = parameters[i];
            const Char* format = nullptr;
            int32 size;
            int32 alignment;
            bool zeroRegister = true;
            switch (param.Type)
            {
            case MaterialParameterType::Bool:
                size = 4;
                alignment = 4;
                format = SE_TEXT("bool {0};");
                break;
            case MaterialParameterType::Integer:
                size = 4;
                alignment = 4;
                format = SE_TEXT("int {0};");
                break;
            case MaterialParameterType::Float:
                size = 4;
                alignment = 4;
                format = SE_TEXT("float {0};");
                break;
            case MaterialParameterType::Vector2:
                size = 8;
                alignment = 8;
                format = SE_TEXT("float2 {0};");
                break;
            case MaterialParameterType::Vector3:
                size = 12;
                alignment = 16;
                format = SE_TEXT("float3 {0};");
                break;
            case MaterialParameterType::Vector4:
            case MaterialParameterType::ChannelMask:
            case MaterialParameterType::Color:
                size = 16;
                alignment = 16;
                format = SE_TEXT("float4 {0};");
                break;
            case MaterialParameterType::Matrix:
                size = 16 * 4;
                alignment = 16;
                format = SE_TEXT("float4x4 {0};");
                break;
            /*case MaterialParameterType::GameplayGlobal:
            {
                auto asset = Content::LoadAsync<GameplayGlobals>(param.AsGuid);
                if (!asset || asset->WaitForLoaded())
                    break;
                GameplayGlobals::Variable variable;
                if (!asset->Variables.TryGet(param.Name, variable))
                    break;
                switch (variable.DefaultValue.Type.Type)
                {
                case VariantType::Bool:
                    size = 4;
                    alignment = 4;
                    format = SE_TEXT("bool {0};");
                    break;
                case VariantType::Int:
                    size = 4;
                    alignment = 4;
                    format = SE_TEXT("int {0};");
                    break;
                case VariantType::Uint:
                    size = 4;
                    alignment = 4;
                    format = SE_TEXT("uint {0};");
                    break;
                case VariantType::Float:
                case VariantType::Double:
                    size = 4;
                    alignment = 4;
                    format = SE_TEXT("float {0};");
                    break;
                case VariantType::Float2:
                case VariantType::Double2:
                    size = 8;
                    alignment = 8;
                    format = SE_TEXT("float2 {0};");
                    break;
                case VariantType::Float3:
                case VariantType::Double3:
                    size = 12;
                    alignment = 16;
                    format = SE_TEXT("float3 {0};");
                    break;
                case VariantType::Float4:
                case VariantType::Double4:
                case VariantType::Color:
                    size = 16;
                    alignment = 16;
                    format = SE_TEXT("float4 {0};");
                    break;
                }
                break;
            }*/
            /*case MaterialParameterType::GlobalSDF:
                zeroRegister = false;
                size = sizeof(GlobalSignDistanceFieldPass::ConstantsData);
                alignment = 16;
                format = SE_TEXT("GlobalSDFData {0};");
                break;*/
            }
            if (format)
            {
                int32 padding = Math::Abs((alignment - (constantsOffset % 16))) % alignment;
                if (padding != 0)
                {
                    constantsOffset += padding;
                    padding /= 4;
                    while (padding-- > 0)
                    {
                        writer.WriteLine(SE_TEXT("uint PADDING_{0};"), paddingIndex++);
                    }
                }

                if (zeroRegister)
                    param.RegisterIndex = 0;
                param.Offset = constantsOffset;
                writer.WriteLine(format, param.ShaderName);
                constantsOffset += size;
            }
        }
    }

    const Char* GenerateShaderResources(TextWriterUnicode& writer, List<SerializedMaterialParam>& parameters, int32 startRegister)
    {
        for (int32 i = 0; i < parameters.Count(); i++)
        {
            auto& param = parameters[i];
            const Char* format = nullptr;
            bool zeroOffset = true;
            int32 registers = 1;
            switch (param.Type)
            {
            case MaterialParameterType::NormalMap:
            case MaterialParameterType::GPUTexture:
            case MaterialParameterType::SceneTexture:
            case MaterialParameterType::Texture:
                format = SE_TEXT("Texture2D {0} : register(t{1});");
                break;
            case MaterialParameterType::GPUTextureCube:
            case MaterialParameterType::CubeTexture:
                format = SE_TEXT("TextureCube {0} : register(t{1});");
                break;
            case MaterialParameterType::GPUTextureArray:
                format = SE_TEXT("Texture2DArray {0} : register(t{1});");
                break;
            case MaterialParameterType::GPUTextureVolume:
                format = SE_TEXT("Texture3D {0} : register(t{1});");
                break;
            case MaterialParameterType::GlobalSDF:
                format = SE_TEXT("Texture3D<float> {0}_Tex : register(t{1});");
                zeroOffset = false;
                break;
            }
            if (format)
            {
                if (zeroOffset)
                    param.Offset = 0;
                param.RegisterIndex = (byte)startRegister;
                writer.WriteLine(format, param.ShaderName, startRegister);
                startRegister += registers;
                if (param.RegisterIndex >= GPU_MAX_SR_BINDED)
                {
                    return SE_TEXT("Too many textures used. The maximum supported amount is " MACRO_TO_STR(GPU_MAX_SR_BINDED) " (including lightmaps and utility textures for lighting).");
                }
            }
        }
        return nullptr;
    }

    const Char* GenerateSamplers(TextWriterUnicode& writer, List<SerializedMaterialParam>& parameters, int32 startRegister)
    {
        for (int32 i = 0; i < parameters.Count(); i++)
        {
            auto& param = parameters[i];
            const Char* format;
            switch (param.Type)
            {
            case MaterialParameterType::TextureGroupSampler:
                format = SE_TEXT("sampler {0} : register(s{1});");
                break;
            default:
                format = nullptr;
                break;
            }
            if (format)
            {
                param.Offset = 0;
                param.RegisterIndex = (byte)startRegister;
                writer.WriteLine(format, param.ShaderName, startRegister);
                startRegister++;
                if (param.RegisterIndex >= GPU_MAX_SAMPLER_BINDED)
                {
                    return SE_TEXT("Too many samplers used. The maximum supported amount is " MACRO_TO_STR(GPU_MAX_SAMPLER_BINDED) ".");
                }
            }
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