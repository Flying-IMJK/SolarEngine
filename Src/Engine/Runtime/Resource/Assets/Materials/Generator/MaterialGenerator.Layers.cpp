

#include "MaterialGenerator.h"

namespace SE
{
    void MaterialGenerator::ProcessGroupLayers(Box* box, Node* node, Value& value)
    {
        switch (node->TypeID)
        {
            // Sample Layer
        case 1:
        {
            UID id = (UID)node->Values[0];
            if (!id.IsValid())
            {
                OnError(node, box, SE_TEXT("Missing material."));
                break;
            }
            ASSERT(GetRootLayer() != nullptr && GetRootLayer()->ID.IsValid());
            if (id == GetRootLayer()->ID)
            {
                OnError(node, box, SE_TEXT("Cannot use current material as layer."));
                break;
            }

            // Load material layer
            auto layer = GetLayer(id, node);
            if (layer == nullptr)
            {
                OnError(node, box, SE_TEXT("Cannot load material."));
                break;
            }
            ASSERT(_layers.Contains(layer));

            // Peek material variable name (may be empty if not used before)
            auto uvsBox = node->GetBox(0);
            String& varName = layer->GetVariableName(uvsBox->HasConnection() ? uvsBox->Connections[0] : nullptr);

            // Check if layer has no generated data
            if (varName.IsEmpty())
            {
                // Create material variable
                MaterialValue defaultValue = MaterialValue::InitForZero(VariantTypeHandle::Types::Void);
                varName = writeLocal(defaultValue, node).Value;

                // Check if use custom UVs
                String customUVs;
                String orginalUVs;
                if (uvsBox->HasConnection())
                {
                    // Sample custom UVs
                    MaterialValue v = eatBox(uvsBox->GetParent<Node>(), uvsBox->FirstConnection());
                    customUVs = v.Value;

                    // TODO: better idea would to be to use variable for current UVs, by default=input.TexCoord.xy could be modified when sampling layers

                    // Cache original pixel UVs
                    orginalUVs = writeLocal(VariantTypeHandle::Types::Float2, SE_TEXT("input.TexCoord.xy"), node).Value;

                    // Modify current pixel UVs
                    _writer.Write(*String::Format(SE_TEXT("\tinput.TexCoord.xy = {0};\n"), customUVs));
                }

                // Cache current layer and tree type
                auto callingLayerVarName = _treeLayerVarName;
                auto callingLayer = _treeLayer;
                auto treeType = _treeType;
                _treeLayer = layer;
                _graphStack.Push(&_treeLayer->Graph);
                _treeLayerVarName = varName;

                // Sample layer
                const MaterialGraphBox* layerInputBox = layer->Root->GetBox(0);
                if (layerInputBox->HasConnection())
                {
                    MaterialValue subLayer = eatBox(layer->Root, layerInputBox->FirstConnection());
                    _writer.Write(SE_TEXT("\t{0} = {1};\n"), varName, subLayer.Value);
                }
                else
                {
#define EAT_BOX(type) eatMaterialGraphBox(layer, MaterialGraphBoxes::type)
                    switch (_treeType)
                    {
                    case MaterialTreeType::VertexShader:
                    {
                        EAT_BOX(PositionOffset);
                        EAT_BOX(TessellationMultiplier);
                        break;
                    }
                    case MaterialTreeType::DomainShader:
                    {
                        EAT_BOX(WorldDisplacement);
                        break;
                    }
                    case MaterialTreeType::PixelShader:
                    {
                        EAT_BOX(Normal);
                        EAT_BOX(Color);
                        EAT_BOX(Metalness);
                        EAT_BOX(Specular);
                        EAT_BOX(Roughness);
                        EAT_BOX(AmbientOcclusion);
                        EAT_BOX(Opacity);
                        EAT_BOX(Refraction);
                        EAT_BOX(Mask);
                        EAT_BOX(Emissive);
                        EAT_BOX(SubsurfaceColor);
                        if (!(GetRootLayer()->FeaturesFlags.IsFlag(MaterialFeatures::InputWorldSpaceNormal) && layer->FeaturesFlags.IsFlag(MaterialFeatures::InputWorldSpaceNormal)))
                        {
                            // TODO convert normal vector to match the output layer properties
                            LOG_WARNING("Graph", "TODO: convert normal vector to match the output layer properties");
                        }
                        break;
                    }
                    default:
                        break;
                    }
#undef EAT_BOX
                }

                // Mix usage flags
                callingLayer->UsageFlags.Marge(_treeLayer->UsageFlags);

                // Restore calling tree and layer
                _treeLayerVarName = callingLayerVarName;
                _treeLayer = callingLayer;
                _graphStack.Pop();
                _treeType = treeType;

                // Check was using custom UVs for sampling
                if (uvsBox->HasConnection())
                {
                    // Restore current pixel UVs
                    _writer.Write(*String::Format(SE_TEXT("\tinput.TexCoord.xy = {0};\n"), orginalUVs));
                }
            }
            ASSERT(varName.HasChars());

            // Use value
            value = MaterialValue(VariantTypeHandle::Types::Void, varName);
            break;
        }
            // Blend Linear
        case 2:
        case 5:
        case 8:
        {
            const Value defaultValue = MaterialValue::InitForZero(VariantTypeHandle::Types::Void);
            Value alpha = tryGetValue(node->GetBox(2), 0, Value::Zero).AsFloat();
            if (alpha.IsZero())
            {
                // Bottom-only
                value = tryGetValue(node->GetBox(0), defaultValue);
                return;
            }
            if (alpha.IsOne())
            {
                // Top-only
                value = tryGetValue(node->GetBox(1), defaultValue);
                return;
            }

            // Sample layers
            const MaterialValue bottom = tryGetValue(node->GetBox(0), defaultValue);
            const MaterialValue top = tryGetValue(node->GetBox(1), defaultValue);

            // Create new layer
            value = writeLocal(defaultValue, node);

            // Blend layers
            if (node->TypeID == 8)
            {
                // Height Layer Blend
                auto bottomHeight = tryGetValue(node->GetBox(4), Value::Zero);
                auto topHeight = tryGetValue(node->GetBox(5), Value::Zero);
                auto bottomHeightScaled = writeLocal(VariantTypeHandle::Types::Float, String::Format(SE_TEXT("{0} * (1.0 - {1})"), bottomHeight.Value, alpha.Value), node);
                auto topHeightScaled = writeLocal(VariantTypeHandle::Types::Float, String::Format(SE_TEXT("{0} * {1}"), topHeight.Value, alpha.Value), node);
                auto heightStart = writeLocal(VariantTypeHandle::Types::Float, String::Format(SE_TEXT("max({0}, {1}) - 0.05"), bottomHeightScaled.Value, topHeightScaled.Value), node);
                auto bottomLevel = writeLocal(VariantTypeHandle::Types::Float, String::Format(SE_TEXT("max({0} - {1}, 0.0001)"), topHeightScaled.Value, heightStart.Value), node);
                alpha = writeLocal(VariantTypeHandle::Types::Float, alpha.Value, node);
                _writer.Write(SE_TEXT("\t{0} = {1} / (max({2} - {3}, 0) + {4});\n"), alpha.Value, bottomLevel.Value, bottomHeightScaled.Value, heightStart.Value, bottomLevel.Value);
            }
#define EAT_BOX(type) writeBlending(MaterialGraphBoxes::type, value, bottom, top, alpha)
            switch (_treeType)
            {
            case MaterialTreeType::VertexShader:
            {
                EAT_BOX(PositionOffset);
                EAT_BOX(TessellationMultiplier);
                break;
            }
            case MaterialTreeType::DomainShader:
            {
                EAT_BOX(WorldDisplacement);
                break;
            }
            case MaterialTreeType::PixelShader:
            {
                EAT_BOX(Normal);
                EAT_BOX(Color);
                EAT_BOX(Metalness);
                EAT_BOX(Specular);
                EAT_BOX(Roughness);
                EAT_BOX(AmbientOcclusion);
                EAT_BOX(Opacity);
                EAT_BOX(Refraction);
                EAT_BOX(Mask);
                EAT_BOX(Emissive);
                EAT_BOX(SubsurfaceColor);
                break;
            }
            default:
                break;
            }
#undef EAT_BOX
            break;
        }
            // Pack Material Layer (old: without TessellationMultiplier, SubsurfaceColor and WorldDisplacement support)
            // [Deprecated on 2018.10.01, expires on 2019.10.01]
        case 3:
        {
            // Create new layer
            const MaterialValue defaultValue = MaterialValue::InitForZero(VariantTypeHandle::Types::Void);
            value = writeLocal(defaultValue, node);

            // Sample layer
#define CHECK_MATERIAL_FEATURE(type, feature) if (node->GetBox(static_cast<int32>(MaterialGraphBoxes::type))->HasConnection()) _treeLayer->UsageFlags.SetFlag(MaterialUsage::feature)
#define EAT_BOX(type) eatMaterialGraphBox(value.Value, node->GetBox((int32)MaterialGraphBoxes::type), MaterialGraphBoxes::type)
            switch (_treeType)
            {
            case MaterialTreeType::VertexShader:
            {
                EAT_BOX(PositionOffset);

                CHECK_MATERIAL_FEATURE(PositionOffset, UsePositionOffset);

                break;
            }
            case MaterialTreeType::PixelShader:
            {
                EAT_BOX(Normal);
                EAT_BOX(Color);
                EAT_BOX(Metalness);
                EAT_BOX(Specular);
                EAT_BOX(Roughness);
                EAT_BOX(AmbientOcclusion);
                EAT_BOX(Opacity);
                EAT_BOX(Refraction);
                EAT_BOX(Mask);
                EAT_BOX(Emissive);

                CHECK_MATERIAL_FEATURE(Emissive, UseEmissive);
                CHECK_MATERIAL_FEATURE(Normal, UseNormal);
                CHECK_MATERIAL_FEATURE(Mask, UseMask);
                CHECK_MATERIAL_FEATURE(Refraction, UseRefraction);

                break;
            }
            default:
                break;
            }
#undef CHECK_MATERIAL_FEATURE
#undef EAT_BOX

            break;
        }
            // Unpack Material Layer
            // Node type 4 -> [Deprecated on 2018.10.01, expires on 2019.10.01]
        case 4:
        case 7:
        {
            // Get layer
            MaterialValue defaultValue = MaterialValue::InitForZero(VariantTypeHandle::Types::Void);
            MaterialValue layer = tryGetValue(node->GetBox(0), defaultValue);

            // Extract component or use default value if cannot use that box in the current tree
            auto& nodeMapping = MaterialGraphBoxesMappings[box->ID];
            if (nodeMapping.TreeType == _treeType)
            {
                value = MaterialValue(nodeMapping.DefaultValue.Type, layer.Value + SE_TEXT(".") + nodeMapping.SubName);
            }
            else
            {
                value = nodeMapping.DefaultValue;
            }
            break;
        }
            // Pack Material Layer
        case 6:
        {
            // Create new layer
            const MaterialValue defaultValue = MaterialValue::InitForZero(VariantTypeHandle::Types::Void);
            value = writeLocal(defaultValue, node);

            // Sample layer
#define CHECK_MATERIAL_FEATURE(type, feature) if (node->GetBox(static_cast<int32>(MaterialGraphBoxes::type))->HasConnection()) _treeLayer->UsageFlags.SetFlag(MaterialUsage::feature)
#define EAT_BOX(type) eatMaterialGraphBox(value.Value, node->GetBox((int32)MaterialGraphBoxes::type), MaterialGraphBoxes::type)
            switch (_treeType)
            {
            case MaterialTreeType::VertexShader:
            {
                EAT_BOX(PositionOffset);
                EAT_BOX(TessellationMultiplier);

                CHECK_MATERIAL_FEATURE(PositionOffset, UsePositionOffset);

                break;
            }
            case MaterialTreeType::DomainShader:
            {
                EAT_BOX(WorldDisplacement);

                CHECK_MATERIAL_FEATURE(WorldDisplacement, UseDisplacement);

                break;
            }
            case MaterialTreeType::PixelShader:
            {
                EAT_BOX(Normal);
                EAT_BOX(Color);
                EAT_BOX(Metalness);
                EAT_BOX(Specular);
                EAT_BOX(Roughness);
                EAT_BOX(AmbientOcclusion);
                EAT_BOX(Opacity);
                EAT_BOX(Refraction);
                EAT_BOX(Mask);
                EAT_BOX(Emissive);
                EAT_BOX(SubsurfaceColor);

                CHECK_MATERIAL_FEATURE(Emissive, UseEmissive);
                CHECK_MATERIAL_FEATURE(Normal, UseNormal);
                CHECK_MATERIAL_FEATURE(Mask, UseMask);
                CHECK_MATERIAL_FEATURE(Refraction, UseRefraction);

                break;
            }
            default:
                break;
            }
#undef CHECK_MATERIAL_FEATURE
#undef EAT_BOX
            break;
        }
        }
    }
}