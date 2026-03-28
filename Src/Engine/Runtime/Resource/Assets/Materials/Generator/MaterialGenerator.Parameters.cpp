
#include "MaterialGenerator.h"

namespace SE
{
    void MaterialGenerator::ProcessGroupParameters(Box* box, Node* node, Value& value)
    {
        switch (node->TypeID)
        {
            // Get
        case 1:
        {
            // Get parameter
            const auto param = findParam((UID)node->Values[0], _treeLayer);
            if (param)
            {
                switch (param->Type)
                {
                case MaterialParameterType::Bool:
                    value = Value(VariantTypeHandle::Types::Bool, param->ShaderName);
                    break;
                case MaterialParameterType::Integer:
                case MaterialParameterType::SceneTexture:
                    value = Value(VariantTypeHandle::Types::Int, param->ShaderName);
                    break;
                case MaterialParameterType::Float:
                    value = Value(VariantTypeHandle::Types::Float, param->ShaderName);
                    break;
                case MaterialParameterType::Vector2:
                case MaterialParameterType::Vector3:
                case MaterialParameterType::Vector4:
                case MaterialParameterType::Color:
                {
                    // Set result values based on box ID
                    const Value sample(box->Type.Type, param->ShaderName);
                    switch (box->ID)
                    {
                    case 0:
                        value = sample;
                        break;
                    case 1:
                        value.Value = sample.Value + _subs[0];
                        break;
                    case 2:
                        value.Value = sample.Value + _subs[1];
                        break;
                    case 3:
                        value.Value = sample.Value + _subs[2];
                        break;
                    case 4:
                        value.Value = sample.Value + _subs[3];
                        break;
                    default: ENGINE_UNREACHABLE_CODE();
                        break;
                    }
                    value.Type = box->Type.Type;
                    break;
                }
                case MaterialParameterType::Matrix:
                {
                    value = Value(box->Type.Type, String::Format(SE_TEXT("{0}[{1}]"), param->ShaderName, box->ID));
                    break;
                }
                case MaterialParameterType::ChannelMask:
                {
                    const auto input = tryGetValue(node->GetBox(0), Value::Zero);
                    value = writeLocal(VariantTypeHandle::Types::Float, String::Format(SE_TEXT("dot({0}, {1})"), input.Value, param->ShaderName), node);
                    break;
                }
                case MaterialParameterType::CubeTexture:
                case MaterialParameterType::NormalMap:
                case MaterialParameterType::Texture:
                case MaterialParameterType::GPUTextureArray:
                case MaterialParameterType::GPUTextureCube:
                case MaterialParameterType::GPUTextureVolume:
                case MaterialParameterType::GPUTexture:
                    sampleTexture(node, value, box, param);
                    break;
                default: ENGINE_UNREACHABLE_CODE();
                    break;
                }
            }
            else
            {
                OnError(node, box, String::Format(SE_TEXT("Missing graph parameter {0}."), node->Values[0].ToString()));
                value = Value::Zero;
            }
            break;
        }
        default:
            break;
        }
    }
}