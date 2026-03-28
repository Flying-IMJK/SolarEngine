
#include "MaterialGenerator.h"
#include "Core/Math/Vector4.h"
#include "Core/Math/Matrix.h"
#include "Runtime/Resource/Assets/Materials/Material.h"
#include "Runtime/Render/Assets/Texture/Texture.h"
#include "Runtime/Engine.h"

namespace SE
{

MaterialGenerator::MaterialGraphBoxesMapping MaterialGenerator::MaterialGraphBoxesMappings[] =
{
    { 0, nullptr, MaterialTreeType::PixelShader, MaterialValue::Zero },
    { 1, SE_TEXT("Color"), MaterialTreeType::PixelShader, MaterialValue::InitForZero(VariantType::Types::Float3) },
    { 2, SE_TEXT("Mask"), MaterialTreeType::PixelShader, MaterialValue::InitForOne(VariantType::Types::Float) },
    { 3, SE_TEXT("Emissive"), MaterialTreeType::PixelShader, MaterialValue::InitForZero(VariantType::Types::Float3) },
    { 4, SE_TEXT("Metalness"), MaterialTreeType::PixelShader, MaterialValue::InitForZero(VariantType::Types::Float) },
    { 5, SE_TEXT("Specular"), MaterialTreeType::PixelShader, MaterialValue::InitForHalf(VariantType::Types::Float) },
    { 6, SE_TEXT("Roughness"), MaterialTreeType::PixelShader, MaterialValue::InitForHalf(VariantType::Types::Float) },
    { 7, SE_TEXT("AO"), MaterialTreeType::PixelShader, MaterialValue::InitForOne(VariantType::Types::Float) },
    { 8, SE_TEXT("TangentNormal"), MaterialTreeType::PixelShader, MaterialValue(VariantType::Types::Float3, SE_TEXT("float3(0, 0, 1.0)")) },
    { 9, SE_TEXT("Opacity"), MaterialTreeType::PixelShader, MaterialValue::InitForOne(VariantType::Types::Float) },
    { 10, SE_TEXT("Refraction"), MaterialTreeType::PixelShader, MaterialValue::InitForOne(VariantType::Types::Float) },
    { 11, SE_TEXT("PositionOffset"), MaterialTreeType::VertexShader, MaterialValue::InitForZero(VariantType::Types::Float3) },
    { 12, SE_TEXT("TessellationMultiplier"), MaterialTreeType::VertexShader, MaterialValue(VariantType::Types::Float, SE_TEXT("4.0f")) },
    { 13, SE_TEXT("WorldDisplacement"), MaterialTreeType::DomainShader, MaterialValue::InitForZero(VariantType::Types::Float3) },
    { 14, SE_TEXT("SubsurfaceColor"), MaterialTreeType::PixelShader, MaterialValue::InitForZero(VariantType::Types::Float3) },
};

const MaterialGenerator::MaterialGraphBoxesMapping& MaterialGenerator::GetMaterialRootNodeBox(MaterialGraphBoxes box)
{
    static_assert(ARRAY_SIZE(MaterialGenerator::MaterialGraphBoxesMappings) == 15, "Invalid amount of boxes added for root node. Please update the code above");
    return MaterialGraphBoxesMappings[static_cast<int32>(box)];
}

void MaterialGenerator::AddLayer(MaterialLayer* layer)
{
    _layers.Add(layer);
}

MaterialLayer* MaterialGenerator::GetLayer(const UID& id, Node* caller)
{
    // Find layer first
    for (int32 i = 0; i < _layers.Count(); i++)
    {
        if (_layers[i]->ID == id)
        {
            // Found
            return _layers[i];
        }
    }

    // Load asset
    Asset* asset = Assets.LoadAsync<MaterialBase>(id);
    if (asset == nullptr || asset->WaitForLoaded(30000))
    {
        OnError(caller, nullptr, SE_TEXT("Failed to load material asset."));
        return nullptr;
    }

    // Special case for engine exit event
    if (Engine::ShouldExit())
    {
        // End
        return nullptr;
    }

    // Check if load failed
    if (!asset->IsLoaded())
    {
        OnError(caller, nullptr, SE_TEXT("Failed to load material asset."));
        return nullptr;
    }

    // Check if it's a material instance
    Material* material = nullptr;
    Asset* iterator = asset;
    while (material == nullptr)
    {
        // Wait for material to be loaded
        if (iterator->WaitForLoaded())
        {
            OnError(caller, nullptr, SE_TEXT("Material asset load failed."));
            return nullptr;
        }

        if (iterator->GetTypeName() == MaterialInstance::TypeName)
        {
            auto instance = ((MaterialInstance*)iterator);

            // Check if base material has been assigned
            if (instance->GetBaseMaterial() == nullptr)
            {
                OnError(caller, nullptr, SE_TEXT("Material instance has missing base material."));
                return nullptr;
            }

            iterator = instance->GetBaseMaterial();
        }
        else
        {
            material = ((Material*)iterator);

            // Check if instanced material is not instancing current material
            ASSERT(GetRootLayer());
            if (material->GetID() == GetRootLayer()->ID)
            {
                OnError(caller, nullptr, SE_TEXT("Cannot use instance of the current material as layer."));
                return nullptr;
            }
        }
    }
    ASSERT(material);

    // Get surface data
    BytesContainer surfaceData = material->LoadSurface(true);
    if (surfaceData.IsInvalid())
    {
        OnError(caller, nullptr, SE_TEXT("Cannot load surface data."));
        return nullptr;
    }
    MemoryReadStream surfaceDataStream(surfaceData.Get(), surfaceData.Length());

    // Load layer
    auto layer = MaterialLayer::Load(id, &surfaceDataStream, material->GetInfo(), material->ToString());

    // Validate layer info
    if (layer == nullptr)
    {
        OnError(caller, nullptr, SE_TEXT("Cannot load layer."));
        return nullptr;
    }
#if 0 // allow mixing material domains because material generator uses the base layer for features usage checks and sub layers produce only Material structure for blending or extracting
	if (layer->Domain != GetRootLayer()->Domain) // TODO: maybe allow solid on transparent?
	{
		OnError(caller, nullptr, SE_TEXT("Cannot mix materials with different Domains."));
		return nullptr;
	}
#endif

    // Override parameters values if using material instance
    if (asset->GetTypeName() == MaterialInstance::TypeName)
    {
        auto instance = ((MaterialInstance*)asset);
        auto instanceParams = &instance->Params;

        // Clone overriden parameters values
        for (auto& param : layer->Graph.Parameters)
        {
            const auto instanceParam = instanceParams->Get(param.Identifier);
            if (instanceParam && instanceParam->IsOverride())
            {
                param.Value = instanceParam->GetValue();

                // Fold object references to their ids
                if (param.Value.Type == VariantType::Types::Object)
                    param.Value = param.Value.AsObject ? param.Value.AsObject->GetID() : UID::Empty;
                else if (param.Value.Type == VariantType::Types::Asset)
                    param.Value = param.Value.AsObject ? param.Value.AsObject->GetID() : UID::Empty;
            }
        }
    }

    // Prepare layer
    layer->Prepare();
    const bool allowPublicParameters = true;
    prepareLayer(layer, allowPublicParameters);

    AddLayer(layer);
    return layer;
}

MaterialLayer* MaterialGenerator::GetRootLayer() const
{
    return _layers.Count() > 0 ? _layers[0] : nullptr;
}

void MaterialGenerator::prepareLayer(MaterialLayer* layer, bool allowVisibleParams)
{
    LayerParamMapping m;

    // Ensure that layer hasn't been used
    ASSERT(layer->HasAnyVariableName() == false);

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
        case VariantType::Types::Bool:
            mp.Type = MaterialParameterType::Bool;
            mp.AsBool = param->Value.AsBool;
            break;
        case VariantType::Types::Int:
            mp.Type = MaterialParameterType::Integer;
            mp.AsInteger = param->Value.AsInt;
            break;
        case VariantType::Types::Int64:
            mp.Type = MaterialParameterType::Integer;
            mp.AsInteger = (int32)param->Value.AsInt64;
            break;
        case VariantType::Types::Uint:
            mp.Type = MaterialParameterType::Integer;
            mp.AsInteger = (int32)param->Value.AsUint;
            break;
        case VariantType::Types::Uint64:
            mp.Type = MaterialParameterType::Integer;
            mp.AsInteger = (int32)param->Value.AsUint64;
            break;
        case VariantType::Types::Float:
            mp.Type = MaterialParameterType::Float;
            mp.AsFloat = param->Value.AsFloat;
            break;
        case VariantType::Types::Double:
            mp.Type = MaterialParameterType::Float;
            mp.AsFloat = (float)param->Value.AsDouble;
            break;
        case VariantType::Types::Float2:
            mp.Type = MaterialParameterType::Vector2;
            mp.AsFloat2 = param->Value.AsFloat2();
            break;
        case VariantType::Types::Float3:
            mp.Type = MaterialParameterType::Vector3;
            mp.AsFloat3 = param->Value.AsFloat3();
            break;
        case VariantType::Types::Float4:
            mp.Type = MaterialParameterType::Vector4;
            *(Float4*)&mp.AsData = param->Value.AsFloat4();
            break;
        case VariantType::Types::Double2:
            mp.Type = MaterialParameterType::Vector2;
            mp.AsFloat2 = (Float2)param->Value.AsDouble2();
            break;
        case VariantType::Types::Double3:
            mp.Type = MaterialParameterType::Vector3;
            mp.AsFloat3 = (Float3)param->Value.AsDouble3();
            break;
        case VariantType::Types::Double4:
            mp.Type = MaterialParameterType::Vector4;
            *(Float4*)&mp.AsData = (Float4)param->Value.AsDouble4();
            break;
        case VariantType::Types::Color:
            mp.Type = MaterialParameterType::Color;
            mp.AsColor = param->Value.AsColor();
            break;
        case VariantType::Types::Matrix:
            mp.Type = MaterialParameterType::Matrix;
            *(Matrix*)&mp.AsData = *(Matrix*)param->Value.AsBlob.Data;
            break;
        case VariantType::Types::Asset:
            if (!param->Type.TypeName)
            {
                OnError(nullptr, nullptr, String::Format(SE_TEXT("Invalid or unsupported material parameter type {0}."), param->Type));
                break;
            }
            if (StringUtils::Compare(param->Type.TypeName, "FlaxEngine.Texture") == 0)
            {
                mp.Type = MaterialParameterType::Texture;

                // Special case for Normal Maps
                auto asset = (Texture*)::LoadAsset((UID)param->Value, Texture::TypeInitializer);
                if (asset && !asset->WaitForLoaded() && asset->IsNormalMap())
                    mp.Type = MaterialParameterType::NormalMap;
            }
            else if (StringUtils::Compare(param->Type.TypeName, "FlaxEngine.CubeTexture") == 0)
                mp.Type = MaterialParameterType::CubeTexture;
            else
                OnError(nullptr, nullptr, String::Format(SE_TEXT("Invalid or unsupported material parameter type {0}."), param->Type));
            mp.AsGuid = (UID)param->Value;
            break;
        case VariantType::Types::Object:
            if (!param->Type.TypeName)
            {
                OnError(nullptr, nullptr, String::Format(SE_TEXT("Invalid or unsupported material parameter type {0}."), param->Type));
                break;
            }
            if (StringUtils::Compare(param->Type.TypeName, "FlaxEngine.GPUTexture") == 0)
                mp.Type = MaterialParameterType::GPUTexture;
            else
                OnError(nullptr, nullptr, String::Format(SE_TEXT("Invalid or unsupported material parameter type {0}."), param->Type));
            mp.AsGuid = (UID)param->Value;
            break;
        case VariantType::Types::Enum:
            if (!param->Type.TypeName)
            {
                OnError(nullptr, nullptr, String::Format(SE_TEXT("Invalid or unsupported material parameter type {0}."), param->Type));
                break;
            }
            if (StringUtils::Compare(param->Type.TypeName, "FlaxEngine.MaterialSceneTextures") == 0)
                mp.Type = MaterialParameterType::SceneTexture;
            else if (StringUtils::Compare(param->Type.TypeName, "FlaxEngine.ChannelMask") == 0)
                mp.Type = MaterialParameterType::ChannelMask;
            else
                OnError(nullptr, nullptr, String::Format(SE_TEXT("Invalid or unsupported material parameter type {0}."), param->Type));
            mp.AsInteger = (int32)param->Value.AsUint64;
            break;
        default:
            OnError(nullptr, nullptr, String::Format(SE_TEXT("Invalid or unsupported material parameter type {0}."), param->Type));
            break;
        }
    }
}

}