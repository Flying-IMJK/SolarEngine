

#include "MaterialGenerator.h"
#include "Runtime/Core/Math/Matrix.h"
#include "Runtime/Render/Assets/Texture/Texture.h"

namespace SE
{
    MaterialValue MaterialGenerator::AccessParticleAttribute(Node* caller, const StringView& name, ParticleAttributeValueTypes valueType, const Char* index, ParticleAttributeSpace space)
    {
        // TODO: cache the attribute value during material tree execution (eg. reuse the same Particle Color read for many nodes in graph)

        String mappingName = SE_TEXT("Particle.");
        mappingName += name;
        SerializedMaterialParam* attributeMapping = nullptr;

        // Find if this attribute has been already accessed
        for (int32 i = 0; i < _parameters.Count(); i++)
        {
            SerializedMaterialParam& param = _parameters[i];
            if (!param.IsPublic && param.Type == MaterialParameterType::Integer && param.Name == mappingName)
            {
                // Reuse attribute mapping
                attributeMapping = &param;
                break;
            }
        }
        if (!attributeMapping)
        {
            // Create
            SerializedMaterialParam& param = _parameters.AddOne();
            param.Type = MaterialParameterType::Integer;
            param.IsPublic = false;
            param.Override = true;
            param.Name = mappingName;
            param.ShaderName = getParamName(_parameters.Count());
            param.AsInteger = 0;
            param.ID = UID::New();
            attributeMapping = &param;
        }

        // Read particle data from the buffer
        VariantTypes type;
        const Char* format;
        switch (valueType)
        {
        case ParticleAttributeValueTypes::Float:
            type = VariantTypes::Float;
            format = SE_TEXT("GetParticleFloat({1}, {0})");
            break;
        case ParticleAttributeValueTypes::Float2:
            type = VariantTypes::Float2;
            format = SE_TEXT("GetParticleVec2({1}, {0})");
            break;
        case ParticleAttributeValueTypes::Float3:
            type = VariantTypes::Float3;
            format = SE_TEXT("GetParticleVec3({1}, {0})");
            break;
        case ParticleAttributeValueTypes::Float4:
            type = VariantTypes::Float4;
            format = SE_TEXT("GetParticleVec4({1}, {0})");
            break;
        case ParticleAttributeValueTypes::Int:
            type = VariantTypes::Int;
            format = SE_TEXT("GetParticleInt({1}, {0})");
            break;
        case ParticleAttributeValueTypes::Uint:
            type = VariantTypes::Uint;
            format = SE_TEXT("GetParticleUint({1}, {0})");
            break;
        default:
            return MaterialValue::Zero;
        }
        auto result = writeLocal(type, String::Format(format, attributeMapping->ShaderName, index ? index : SE_TEXT("input.ParticleIndex")), caller);

        // Apply transformation to world-space
        switch (space)
        {
        case ParticleAttributeSpace::AsIs:
            break;
        case ParticleAttributeSpace::LocalPosition:
            _writer.Write(SE_TEXT("\t{0} = TransformParticlePosition({0});\n"), result.Value);
            break;
        case ParticleAttributeSpace::LocalDirection:
            _writer.Write(SE_TEXT("\t{0} = TransformParticleVector({0});\n"), result.Value);
            break;
        default: ;
        }

        return result;
    }

    void MaterialGenerator::prepareLayer(MaterialLayer* layer, bool allowVisibleParams)
    {
        LayerParamMapping m;

        // Ensure that layer hasn't been used
        ENGINE_ASSERT(layer->HasAnyVariableName() == false);

        // Add all parameters to be saved in the result material parameters collection (perform merge)
        bool isRooLayer = GetRootLayer() == layer;
        for (int32 j = 0; j < layer->Graph.Parameters.Count(); j++)
        {
            const auto param = &layer->Graph.Parameters[j];

            // For all not root layers (sub-layers) we won't to change theirs ID in order to prevent duplicated ID)
            m.SrcId = param->Identifier;
            if (isRooLayer)
            {
                // Use the same ID (so we can edit it)
                m.DstId = param->Identifier;
            }
            else
            {
                // Generate new ID
                m.DstId = param->Identifier;
                m.DstId.A += _parameters.Count() * 17 + 13;
            }
            layer->ParamIdsMappings.Add(m);

            SerializedMaterialParam& mp = _parameters.AddOne();
            mp.ID = m.DstId;
            mp.IsPublic = param->IsPublic && allowVisibleParams;
            mp.Override = true;
            mp.Name = param->Name;
            mp.ShaderName = getParamName(_parameters.Count());
            mp.Type = MaterialParameterType::Bool;
            mp.AsBool = false;
            switch (param->Type.Type)
            {
            case VariantTypes::Bool:
                mp.Type = MaterialParameterType::Bool;
                mp.AsBool = param->Value.AsBool;
                break;
            case VariantTypes::Int:
                mp.Type = MaterialParameterType::Integer;
                mp.AsInteger = param->Value.AsInt;
                break;
            case VariantTypes::Int64:
                mp.Type = MaterialParameterType::Integer;
                mp.AsInteger = (int32)param->Value.AsInt64;
                break;
            case VariantTypes::Uint:
                mp.Type = MaterialParameterType::Integer;
                mp.AsInteger = (int32)param->Value.AsUint;
                break;
            case VariantTypes::Uint64:
                mp.Type = MaterialParameterType::Integer;
                mp.AsInteger = (int32)param->Value.AsUint64;
                break;
            case VariantTypes::Float:
                mp.Type = MaterialParameterType::Float;
                mp.AsFloat = param->Value.AsFloat;
                break;
            case VariantTypes::Double:
                mp.Type = MaterialParameterType::Float;
                mp.AsFloat = (float)param->Value.AsDouble;
                break;
            case VariantTypes::Float2:
                mp.Type = MaterialParameterType::Vector2;
                mp.AsFloat2 = param->Value.AsFloat2();
                break;
            case VariantTypes::Float3:
                mp.Type = MaterialParameterType::Vector3;
                mp.AsFloat3 = param->Value.AsFloat3();
                break;
            case VariantTypes::Float4:
                mp.Type = MaterialParameterType::Vector4;
                *(Float4*)&mp.AsData = param->Value.AsFloat4();
                break;
            case VariantTypes::Double2:
                mp.Type = MaterialParameterType::Vector2;
                mp.AsFloat2 = (Float2)param->Value.AsDouble2();
                break;
            case VariantTypes::Double3:
                mp.Type = MaterialParameterType::Vector3;
                mp.AsFloat3 = (Float3)param->Value.AsDouble3();
                break;
            case VariantTypes::Double4:
                mp.Type = MaterialParameterType::Vector4;
                *(Float4*)&mp.AsData = (Float4)param->Value.AsDouble4();
                break;
            case VariantTypes::Color:
                mp.Type = MaterialParameterType::Color;
                mp.AsColor = param->Value.AsColor();
                break;
            case VariantTypes::Matrix:
                mp.Type = MaterialParameterType::Matrix;
                *(Matrix*)&mp.AsData = *(Matrix*)param->Value.AsBlob.Data;
                break;
            case VariantTypes::Asset:
                if (!param->Type.TypeName)
                {
                    OnError(nullptr, nullptr, String::Format(SE_TEXT("Invalid or unsupported material parameter type {0}."), param->Type.ToString()));
                    break;
                }
                if (StringUtils::Compare(param->Type.TypeName, "FlaxEngine.Texture") == 0)
                {
                    mp.Type = MaterialParameterType::Texture;

                    // Special case for Normal Maps
                    auto asset = AssetContent::LoadAsync<Texture>((UID)param->Value);
                    if (asset && !asset->WaitForLoaded() && asset->IsNormalMap())
                        mp.Type = MaterialParameterType::NormalMap;
                }
                else if (StringUtils::Compare(param->Type.TypeName, "FlaxEngine.CubeTexture") == 0)
                    mp.Type = MaterialParameterType::CubeTexture;
                else
                    OnError(nullptr, nullptr, String::Format(SE_TEXT("Invalid or unsupported material parameter type {0}."), param->Type.ToString()));
                mp.AsGuid = (UID)param->Value;
                break;
            case VariantTypes::Object:
                if (!param->Type.TypeName)
                {
                    OnError(nullptr, nullptr, String::Format(SE_TEXT("Invalid or unsupported material parameter type {0}."), param->Type.ToString()));
                    break;
                }
                if (StringUtils::Compare(param->Type.TypeName, "FlaxEngine.GPUTexture") == 0)
                    mp.Type = MaterialParameterType::GPUTexture;
                else
                    OnError(nullptr, nullptr, String::Format(SE_TEXT("Invalid or unsupported material parameter type {0}."), param->Type.ToString()));
                mp.AsGuid = (UID)param->Value;
                break;
            case VariantTypes::Enum:
                if (!param->Type.TypeName)
                {
                    OnError(nullptr, nullptr, String::Format(SE_TEXT("Invalid or unsupported material parameter type {0}."), param->Type.ToString()));
                    break;
                }
                if (StringUtils::Compare(param->Type.TypeName, "FlaxEngine.MaterialSceneTextures") == 0)
                    mp.Type = MaterialParameterType::SceneTexture;
                else if (StringUtils::Compare(param->Type.TypeName, "FlaxEngine.ChannelMask") == 0)
                    mp.Type = MaterialParameterType::ChannelMask;
                else
                    OnError(nullptr, nullptr, String::Format(SE_TEXT("Invalid or unsupported material parameter type {0}."), param->Type.ToString()));
                mp.AsInteger = (int32)param->Value.AsUint64;
                break;
            default:
                OnError(nullptr, nullptr, String::Format(SE_TEXT("Invalid or unsupported material parameter type {0}."), param->Type.ToString()));
                break;
            }
        }
    }

    void MaterialGenerator::ProcessGroupParticles(Box* box, Node* node, Value& value)
    {
        // Only particle shaders can access particles data
        if (GetRootLayer()->Domain != MaterialDomain::Particle && GetRootLayer()->Domain != MaterialDomain::VolumeParticle)
        {
            value = MaterialValue::Zero;
            return;
        }

        switch (node->TypeID)
        {
            // Particle Attribute
        case 100:
        {
            value = AccessParticleAttribute(node, (StringView)node->Values[0], static_cast<ParticleAttributeValueTypes>(node->Values[1].AsInt));
            break;
        }
            // Particle Attribute (by index)
        case 303:
        {
            const auto particleIndex = Value::Cast(tryGetValue(node->GetBox(1), Value(VariantTypes::Uint, SE_TEXT("input.ParticleIndex"))), VariantTypes::Uint);
            value = AccessParticleAttribute(node, (StringView)node->Values[0], static_cast<ParticleAttributeValueTypes>(node->Values[1].AsInt), particleIndex.Value.Get());
            break;
        }
            // Particle Position
        case 101:
        {
            value = AccessParticleAttribute(node, SE_TEXT("Position"), ParticleAttributeValueTypes::Float3, nullptr, ParticleAttributeSpace::LocalPosition);
            break;
        }
            // Particle Lifetime
        case 102:
        {
            value = AccessParticleAttribute(node, SE_TEXT("Lifetime"), ParticleAttributeValueTypes::Float);
            break;
        }
            // Particle Age
        case 103:
        {
            value = AccessParticleAttribute(node, SE_TEXT("Age"), ParticleAttributeValueTypes::Float);
            break;
        }
            // Particle Color
        case 104:
        {
            value = AccessParticleAttribute(node, SE_TEXT("Color"), ParticleAttributeValueTypes::Float4);
            break;
        }
            // Particle Velocity
        case 105:
        {
            value = AccessParticleAttribute(node, SE_TEXT("Velocity"), ParticleAttributeValueTypes::Float3, nullptr, ParticleAttributeSpace::LocalDirection);
            break;
        }
            // Particle Sprite Size
        case 106:
        {
            value = AccessParticleAttribute(node, SE_TEXT("SpriteSize"), ParticleAttributeValueTypes::Float2);
            break;
        }
            // Particle Mass
        case 107:
        {
            value = AccessParticleAttribute(node, SE_TEXT("Mass"), ParticleAttributeValueTypes::Float);
            break;
        }
            // Particle Rotation
        case 108:
        {
            value = AccessParticleAttribute(node, SE_TEXT("Rotation"), ParticleAttributeValueTypes::Float3);
            break;
        }
            // Particle Angular Velocity
        case 109:
        {
            value = AccessParticleAttribute(node, SE_TEXT("AngularVelocity"), ParticleAttributeValueTypes::Float3);
            break;
        }
            // Particle Normalized Age
        case 110:
        {
            const auto age = AccessParticleAttribute(node, SE_TEXT("Age"), ParticleAttributeValueTypes::Float);
            const auto lifetime = AccessParticleAttribute(node, SE_TEXT("Lifetime"), ParticleAttributeValueTypes::Float);
            value = writeOperation2(node, age, lifetime, '/');
            break;
        }
            // Particle Radius
        case 111:
        {
            value = AccessParticleAttribute(node, SE_TEXT("Radius"), ParticleAttributeValueTypes::Float);
            break;
        }
        default:
            break;
        }
    }
}
