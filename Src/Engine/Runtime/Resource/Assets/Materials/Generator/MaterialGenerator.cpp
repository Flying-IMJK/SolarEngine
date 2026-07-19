
#include "MaterialGenerator.h"
#include "MaterialLayer.h"
#include "Runtime/Graph/GraphUtilities.h"
#include "Runtime/Render/Assets/Material/MaterialShader.h"
#include "Runtime/Render/Assets/Material/MaterialShaderFeatures.h"
#include "Runtime/Core/Platform/File.h"
#include "Runtime/Core/Serialization/MemoryReadStream.h"
#include "Runtime/Core/Thread/Threading.h"
#include "Runtime/Engine.h"
#include "Runtime/EngineContext.h"
#include "Runtime/Resource/Assets/Materials/Material.h"

namespace SE
{
    /// <summary>
    /// Material shader source code template has special marks for generated code.
    /// Each starts with '@' char and index of the mapped string.
    /// </summary>
    enum MaterialTemplateInputsMapping
    {
        In_VersionNumber = 0,
        In_Constants = 1,
        In_ShaderResources = 2,
        In_Defines = 3,
        In_GetMaterialPS = 4,
        In_GetMaterialVS = 5,
        In_GetMaterialDS = 6,
        In_Includes = 7,
        In_Utilities = 8,
        In_Shaders = 9,

        In_MAX
    };

    /// <summary>
    /// Material shader feature source code template has special marks for generated code. Each starts with '@' char and index of the mapped string.
    /// </summary>
    enum class FeatureTemplateInputsMapping
    {
        Defines = 0,
        Includes = 1,
        Constants = 2,
        Resources = 3,
        Utilities = 4,
        Shaders = 5,
        MAX
    };

    struct FeatureData
    {
        MaterialShaderFeature::GeneratorData Data;
        String Inputs[(int32)FeatureTemplateInputsMapping::MAX];

        bool Init();
    };

    namespace
    {
        // Loaded and parsed features data cache
        Dictionary<StringAnsi, FeatureData> Features;
        CriticalSection FeaturesLock;
    }

    bool FeatureData::Init()
    {
        // Load template file
        const String path = EngineContext::StartupFolder / SE_TEXT("Shaders/MaterialTemplates/") + Data.Template;
        String contents;
        if (!File::ReadAllText(path, contents))
        {
            LOG_ERROR("Resource", "Cannot open file {0}", path);
            return false;
        }

        int32 i = 0;
        const int32 length = contents.Length();

        // Skip until input start
        for (; i < length; i++)
        {
            if (contents[i] == '@')
                break;
        }

        // Load all inputs
        do
        {
            // Parse input type
            i++;
            const int32 inIndex = contents[i++] - '0';
            ASSERT_LOW_LAYER(Math::IsInRange(inIndex, 0, (int32)FeatureTemplateInputsMapping::MAX - 1));

            // Read until next input start
            const Char* start = &contents[i];
            for (; i < length; i++)
            {
                const auto c = contents[i];
                if (c == '@')
                    break;
            }
            const Char* end = contents.Get() + i;

            // Set input
            Inputs[inIndex].Set(start, (int32)(end - start));
        } while (i < length);

        return true;
    }

    MaterialValue MaterialGenerator::getUVs(VariantTypes::Float2, SE_TEXT("input.TexCoord"));
    MaterialValue MaterialGenerator::getTime(VariantTypes::Float, SE_TEXT("TimeParam"));
    MaterialValue MaterialGenerator::getNormal(VariantTypes::Float3, SE_TEXT("input.TBN[2]"));
    MaterialValue MaterialGenerator::getNormalZero(VariantTypes::Float3, SE_TEXT("float3(0, 0, 1)"));
    MaterialValue MaterialGenerator::getVertexColor(VariantTypes::Float4, SE_TEXT("GetVertexColor(input)"));

    MaterialGenerator::MaterialGenerator()
    {
        // Register per group type processing events
        // Note: index must match group id
        _perGroupProcessCall[1].Bind<MaterialGenerator, &MaterialGenerator::ProcessGroupMaterial>(this);
        _perGroupProcessCall[3].Bind<MaterialGenerator, &MaterialGenerator::ProcessGroupMath>(this);
        _perGroupProcessCall[5].Bind<MaterialGenerator, &MaterialGenerator::ProcessGroupTextures>(this);
        _perGroupProcessCall[6].Bind<MaterialGenerator, &MaterialGenerator::ProcessGroupParameters>(this);
        _perGroupProcessCall[7].Bind<MaterialGenerator, &MaterialGenerator::ProcessGroupTools>(this);
        _perGroupProcessCall[8].Bind<MaterialGenerator, &MaterialGenerator::ProcessGroupLayers>(this);
        _perGroupProcessCall[14].Bind<MaterialGenerator, &MaterialGenerator::ProcessGroupParticles>(this);
        _perGroupProcessCall[16].Bind<MaterialGenerator, &MaterialGenerator::ProcessGroupFunction>(this);
    }

    MaterialGenerator::~MaterialGenerator()
    {
        _layers.ClearDelete();
    }

    MaterialLayer* MaterialGenerator::GetRootLayer() const
    {
        return _layers.Count() > 0 ? _layers[0] : nullptr;
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

            /*if (iterator->GetTypeName() == MaterialInstance::TypeName)
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
            else*/
            {
                material = ((Material*)iterator);

                // Check if instanced material is not instancing current material
                ENGINE_ASSERT(GetRootLayer());
                if (material->GetID() == GetRootLayer()->ID)
                {
                    OnError(caller, nullptr, SE_TEXT("Cannot use instance of the current material as layer."));
                    return nullptr;
                }
            }
        }
        ENGINE_ASSERT(material);

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
        /*if (asset->GetTypeName() == MaterialInstance::TypeName)
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
        }*/

        // Prepare layer
        layer->Prepare();
        const bool allowPublicParameters = true;
        prepareLayer(layer, allowPublicParameters);

        AddLayer(layer);
        return layer;
    }

    bool MaterialGenerator::Generate(WriteStream& source, MaterialInfo& materialInfo, BytesContainer& parametersData)
    {
        ASSERT_LOW_LAYER(_layers.Count() > 0);

        String inputs[In_MAX];
        List<StringAnsiView, FixedAllocation<8>> features;

        // Setup and prepare layers
        _writer.Clear();
        _includes.Clear();
        _callStack.Clear();
        _parameters.Clear();
        _localIndex = 0;
        _vsToPsInterpolants.Clear();
        _treeLayer = nullptr;
        _graphStack.Clear();
        for (int32 i = 0; i < _layers.Count(); i++)
        {
            auto layer = _layers[i];

            // Prepare
            layer->Prepare();
            prepareLayer(_layers[i], true);

            // Assign layer variable name for initial layers
            layer->Usage[0].VarName = SE_TEXT("material");
            if (i != 0)
                layer->Usage[0].VarName += StringUtils::ToString(i);
        }
        inputs[In_VersionNumber] = StringUtils::ToString(MATERIAL_GRAPH_VERSION);

        // Cache data
        MaterialLayer* baseLayer = GetRootLayer();
        auto* baseNode = baseLayer->Root;
        _treeLayerVarName = baseLayer->GetVariableName(nullptr);
        _treeLayer = baseLayer;
        _graphStack.Add(&_treeLayer->Graph);
        const MaterialGraphBox* layerInputBox = baseLayer->Root->GetBox(0);
        const bool isLayered = layerInputBox->HasConnection();

        // Initialize features
#define ADD_FEATURE(type)                                   \
{                                                           \
    StringAnsiView typeName(#type, ARRAY_SIZE(#type) - 1);  \
    features.Add(typeName);                                 \
    if (!Features.ContainsKey(typeName))                    \
    {                                                       \
        Threading::ScopeLock lock(FeaturesLock);            \
        auto& feature = Features[typeName];                 \
        type::Generate(feature.Data);                       \
        if (!feature.Init())                                \
            return false;                                   \
    }                                                       \
}

        switch (baseLayer->Domain)
        {
        case MaterialDomain::Surface:
            if (materialInfo.TessellationMode != TessellationMethod::None)
                ADD_FEATURE(TessellationFeature);
            if (materialInfo.BlendMode == MaterialBlendMode::Opaque)
                ADD_FEATURE(MotionVectorsFeature);
            if (materialInfo.BlendMode == MaterialBlendMode::Opaque)
                ADD_FEATURE(LightmapFeature);
            if (materialInfo.BlendMode == MaterialBlendMode::Opaque)
                ADD_FEATURE(DeferredShadingFeature);
            if (materialInfo.BlendMode != MaterialBlendMode::Opaque && materialInfo.FeaturesFlags.IsNotFlag(MaterialFeatures::DisableDistortion))
                ADD_FEATURE(DistortionFeature);
            /*if (materialInfo.BlendMode != MaterialBlendMode::Opaque && materialInfo.FeaturesFlags.IsFlagSet(MaterialFeatures::GlobalIllumination))
                ADD_FEATURE(GlobalIlluminationFeature);*/
            if (materialInfo.BlendMode != MaterialBlendMode::Opaque)
                ADD_FEATURE(ForwardShadingFeature);
            break;
        case MaterialDomain::Terrain:
            if (materialInfo.TessellationMode != TessellationMethod::None)
                ADD_FEATURE(TessellationFeature);
            ADD_FEATURE(LightmapFeature);
            ADD_FEATURE(DeferredShadingFeature);
            break;
        case MaterialDomain::Particle:
            if (materialInfo.BlendMode != MaterialBlendMode::Opaque && materialInfo.FeaturesFlags.IsNotFlag(MaterialFeatures::DisableDistortion))
                ADD_FEATURE(DistortionFeature);
            /*if (materialInfo.BlendMode != MaterialBlendMode::Opaque && materialInfo.FeaturesFlags.IsFlagSet(MaterialFeatures::GlobalIllumination))
                ADD_FEATURE(GlobalIlluminationFeature);*/
            ADD_FEATURE(ForwardShadingFeature);
            break;
        case MaterialDomain::Deformable:
            if (materialInfo.TessellationMode != TessellationMethod::None)
                ADD_FEATURE(TessellationFeature);
            if (materialInfo.BlendMode == MaterialBlendMode::Opaque)
                ADD_FEATURE(DeferredShadingFeature);
            if (materialInfo.BlendMode != MaterialBlendMode::Opaque)
                ADD_FEATURE(ForwardShadingFeature);
            break;
        default:
            break;
        }
#undef ADD_FEATURE

        // Check if material is using special features and update the metadata flags
        if (!isLayered)
        {
            baseLayer->UpdateFeaturesFlags();
        }

        // Pixel Shader
        _treeType = MaterialTreeType::PixelShader;
        Value materialVarPS;
        if (isLayered)
        {
            materialVarPS = eatBox(baseNode, layerInputBox->FirstConnection());
        }
        else
        {
            materialVarPS = Value(VariantTypes::Void, baseLayer->GetVariableName(nullptr));
            _writer.Write(SE_TEXT("\tMaterial {0} = (Material)0;\n"), materialVarPS.Value);
            if (baseLayer->Domain == MaterialDomain::Surface || baseLayer->Domain == MaterialDomain::Terrain || baseLayer->Domain == MaterialDomain::Particle || baseLayer->Domain == MaterialDomain::Deformable)
            {
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Emissive);
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Normal);
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Color);
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Metalness);
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Specular);
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::AmbientOcclusion);
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Roughness);
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Opacity);
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Refraction);
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::SubsurfaceColor);
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Mask);
            }
            else if (baseLayer->Domain == MaterialDomain::Decal)
            {
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Emissive);
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Normal);
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Color);
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Metalness);
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Specular);
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Roughness);
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Opacity);
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Mask);

                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::AmbientOcclusion);
                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Refraction);
                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::SubsurfaceColor);
            }
            else if (baseLayer->Domain == MaterialDomain::PostProcess)
            {
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Emissive);
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Opacity);

                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Normal);
                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Color);
                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Metalness);
                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Specular);
                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::AmbientOcclusion);
                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Roughness);
                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Refraction);
                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Mask);
                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::SubsurfaceColor);
            }
            else if (baseLayer->Domain == MaterialDomain::GUI)
            {
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Emissive);
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Opacity);
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Mask);

                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Normal);
                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Color);
                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Metalness);
                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Specular);
                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::AmbientOcclusion);
                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Roughness);
                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Refraction);
                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::SubsurfaceColor);
            }
            else if (baseLayer->Domain == MaterialDomain::VolumeParticle)
            {
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Emissive);
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Opacity);
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Mask);
                eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::Color);

                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Normal);
                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Metalness);
                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Specular);
                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::AmbientOcclusion);
                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Roughness);
                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::Refraction);
                eatMaterialGraphBoxWithDefault(baseLayer, MaterialGraphBoxes::SubsurfaceColor);
            }
            else
            {
                ENGINE_UNREACHABLE_CODE();
            }
        }
        {
            // Flip normal for inverted triangles (used by two sided materials)
            _writer.Write(SE_TEXT("\t{0}.TangentNormal *= input.TwoSidedSign;\n"), materialVarPS.Value);

            // Normalize and transform to world space if need to
            _writer.Write(SE_TEXT("\t{0}.TangentNormal = normalize({0}.TangentNormal);\n"), materialVarPS.Value);
            if (baseLayer->FeaturesFlags.IsFlag(MaterialFeatures::InputWorldSpaceNormal))
            {
                _writer.Write(SE_TEXT("\t{0}.WorldNormal = {0}.TangentNormal;\n"), materialVarPS.Value);
                _writer.Write(SE_TEXT("\t{0}.TangentNormal = normalize(TransformWorldVectorToTangent(input, {0}.WorldNormal));\n"), materialVarPS.Value);
            }
            else
            {
                _writer.Write(SE_TEXT("\t{0}.WorldNormal = normalize(TransformTangentVectorToWorld(input, {0}.TangentNormal));\n"), materialVarPS.Value);
            }

            // Clamp values
            _writer.Write(SE_TEXT("\t{0}.Metalness = saturate({0}.Metalness);\n"), materialVarPS.Value);
            _writer.Write(SE_TEXT("\t{0}.Roughness = max(0.04, {0}.Roughness);\n"), materialVarPS.Value);
            _writer.Write(SE_TEXT("\t{0}.AO = saturate({0}.AO);\n"), materialVarPS.Value);
            _writer.Write(SE_TEXT("\t{0}.Opacity = saturate({0}.Opacity);\n"), materialVarPS.Value);

            // Return result
            _writer.Write(SE_TEXT("\treturn {0};"), materialVarPS.Value);
        }
        inputs[In_GetMaterialPS] = _writer.ToString();
        _writer.Clear();
        clearCache();

        // Domain Shader
        _treeType = MaterialTreeType::DomainShader;
        if (isLayered)
        {
            const Value layer = eatBox(baseNode, layerInputBox->FirstConnection());
            _writer.Write(SE_TEXT("\treturn {0};"), layer.Value);
        }
        else
        {
            _writer.Write(SE_TEXT("\tMaterial material = (Material)0;\n"));
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::WorldDisplacement);
            _writer.Write(SE_TEXT("\treturn material;"));
        }
        inputs[In_GetMaterialDS] = _writer.ToString();
        _writer.Clear();
        clearCache();

        // Vertex Shader
        _treeType = MaterialTreeType::VertexShader;
        if (isLayered)
        {
            const Value layer = eatBox(baseNode, layerInputBox->FirstConnection());
            _writer.Write(SE_TEXT("\treturn {0};"), layer.Value);
        }
        else
        {
            _writer.Write(SE_TEXT("\tMaterial material = (Material)0;\n"));
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::PositionOffset);
            eatMaterialGraphBox(baseLayer, MaterialGraphBoxes::TessellationMultiplier);
            for (int32 i = 0; i < _vsToPsInterpolants.Count(); i++)
            {
                const auto value = tryGetValue(_vsToPsInterpolants[i], Value::Zero).AsFloat4().Value;
                _writer.Write(SE_TEXT("\tmaterial.CustomVSToPS[{0}] = {1};\n"), i, value);
            }
            _writer.Write(SE_TEXT("\treturn material;"));
        }
        inputs[In_GetMaterialVS] = _writer.ToString();
        _writer.Clear();
        clearCache();

        // Update material usage based on material generator outputs
        materialInfo.UsageFlags = baseLayer->UsageFlags;

        // Find all Custom Global Code nodes
        List<const MaterialGraph::Node*, InlinedAllocation<8>> customGlobalCodeNodes;
        List<Graph*, InlinedAllocation<8>> graphs;
        _functions.GetValues(graphs);
        for (MaterialLayer* layer : _layers)
            graphs.Add(&layer->Graph);
        for (Graph* graph : graphs)
        {
            for (MaterialGraph::Node& node : graph->Nodes)
            {
                if (node.Type == GRAPH_NODE_MAKE_TYPE(1, 38) && (bool)node.Values[1])
                {
                    if (node.Values.Count() == 2)
                        node.Values.Add(In_Utilities); // Upgrade old node data
                    customGlobalCodeNodes.Add(&node);
                }
            }
        }

#define WRITE_FEATURES(input) FeaturesLock.Lock(); for (auto f : features) _writer.Write(Features[f].Inputs[(int32)FeatureTemplateInputsMapping::input]); FeaturesLock.Unlock();
        // Defines
        {
            _writer.Write(SE_TEXT("#define MATERIAL_MASK_THRESHOLD ({0})\n"), baseLayer->MaskThreshold);
            _writer.Write(SE_TEXT("#define CUSTOM_VERTEX_INTERPOLATORS_COUNT ({0})\n"), _vsToPsInterpolants.Count());
            _writer.Write(SE_TEXT("#define MATERIAL_OPACITY_THRESHOLD ({0})\n"), baseLayer->OpacityThreshold);
            if (materialInfo.BlendMode != MaterialBlendMode::Opaque &&
                materialInfo.FeaturesFlags.IsNotFlag(MaterialFeatures::DisableReflections) &&
                materialInfo.FeaturesFlags.IsFlag(MaterialFeatures::ScreenSpaceReflections))
            {
                // Inject depth and color buffers for Screen Space Reflections used by transparent material
                /*auto sceneDepthTexture = findOrAddSceneTexture(MaterialSceneTextures::SceneDepth);
                auto sceneColorTexture = findOrAddSceneTexture(MaterialSceneTextures::SceneColor);
                _writer.Write(SE_TEXT("#define MATERIAL_REFLECTIONS_SSR_DEPTH ({0})\n"), sceneDepthTexture.ShaderName);
                _writer.Write(SE_TEXT("#define MATERIAL_REFLECTIONS_SSR_COLOR ({0})\n"), sceneColorTexture.ShaderName);*/
            }
            WRITE_FEATURES(Defines);
            inputs[In_Defines] = _writer.ToString();
            WriteCustomGlobalCode(customGlobalCodeNodes, In_Defines);
            _writer.Clear();
        }

        // Includes
        {
            for (auto& include : _includes)
                _writer.Write(SE_TEXT("#include \"{0}\"\n"), include);
            WRITE_FEATURES(Includes);
            WriteCustomGlobalCode(customGlobalCodeNodes, In_Includes);
            inputs[In_Includes] = _writer.ToString();
            _writer.Clear();
        }

        // Constants
        {
            WRITE_FEATURES(Constants);
            if (_parameters.HasItems())
                GraphUtilities::GenerateShaderConstantBuffer(_writer, _parameters);
            WriteCustomGlobalCode(customGlobalCodeNodes, In_Constants);
            inputs[In_Constants] = _writer.ToString();
            _writer.Clear();
        }

        // Resources
        {
            int32 srv = 0, sampler = GPU_STATIC_SAMPLERS_COUNT;
            switch (baseLayer->Domain)
            {
            case MaterialDomain::Surface:
                srv = 2; // Skinning Bones + Prev Bones
                break;
            case MaterialDomain::Decal:
                srv = 1; // Depth buffer
                break;
            case MaterialDomain::Terrain:
                srv = 3; // Heightmap + 2 splatmaps
                break;
            case MaterialDomain::Particle:
                srv = 2; // Particles data + Sorted indices/Ribbon segments
                break;
            case MaterialDomain::Deformable:
                srv = 1; // Mesh deformation buffer
                break;
            case MaterialDomain::VolumeParticle:
                srv = 1; // Particles data
                break;
            }
            for (auto f : features)
            {
                // Process SRV slots used in template
                const auto& SE_TEXT = Features[f].Inputs[(int32)FeatureTemplateInputsMapping::Resources];
                const Char* str = SE_TEXT.Get();
                int32 prevIdx = 0, idx = 0;
                while (true)
                {
                    idx = SE_TEXT.Find(SE_TEXT("__SRV__"), prevIdx, StringSearchCase::CaseSensitive);
                    if (idx == -1)
                        break;
                    int32 len = idx - prevIdx;
                    _writer.Write(StringView(str, len));
                    str += len;
                    _writer.Write(StringUtils::ToString(srv));
                    srv++;
                    str += ARRAY_SIZE("__SRV__") - 1;
                    prevIdx = idx + ARRAY_SIZE("__SRV__") - 1;
                }
                _writer.Write(StringView(str));
            }
            if (_parameters.HasItems())
            {
                auto error = GraphUtilities::GenerateShaderResources(_writer, _parameters, srv);
                if (!error)
                    error = GraphUtilities::GenerateSamplers(_writer, _parameters, sampler);
                if (error)
                {
                    OnError(nullptr, nullptr, error);
                    return false;
                }
            }
            WriteCustomGlobalCode(customGlobalCodeNodes, In_ShaderResources);
            inputs[In_ShaderResources] = _writer.ToString();
            _writer.Clear();
        }

        // Utilities
        {
            WRITE_FEATURES(Utilities);
            WriteCustomGlobalCode(customGlobalCodeNodes, In_Utilities);
            inputs[In_Utilities] = _writer.ToString();
            _writer.Clear();
        }

        // Shaders
        {
            WRITE_FEATURES(Shaders);
            WriteCustomGlobalCode(customGlobalCodeNodes, In_Shaders);
            inputs[In_Shaders] = _writer.ToString();
            _writer.Clear();
        }

        // Save material parameters data
        if (_parameters.HasItems())
            MaterialParams::Save(parametersData, &_parameters);
        else
            parametersData.Release();
        _parameters.Clear();

        // Create source code
        {
            // Open template file
            String path = EngineContext::StartupFolder / SE_TEXT("Shaders/MaterialTemplates/");
            switch (materialInfo.Domain)
            {
            case MaterialDomain::Surface:
                path /= SE_TEXT("Surface.shader");
                break;
            case MaterialDomain::PostProcess:
                path /= SE_TEXT("PostProcess.shader");
                break;
            case MaterialDomain::GUI:
                path /= SE_TEXT("GUI.shader");
                break;
            case MaterialDomain::Decal:
                path /= SE_TEXT("Decal.shader");
                break;
            case MaterialDomain::Terrain:
                path /= SE_TEXT("Terrain.shader");
                break;
            case MaterialDomain::Particle:
                path /= SE_TEXT("Particle.shader");
                break;
            case MaterialDomain::Deformable:
                path /= SE_TEXT("Deformable.shader");
                break;
            case MaterialDomain::VolumeParticle:
                path /= SE_TEXT("VolumeParticle.shader");
                break;
            default:
                LOG_WARNING("Resource", "Unknown material domain.");
                return true;
            }
            auto file = FileReadStream::Open(path);
            if (file == nullptr)
            {
                LOG_ERROR("Resource", "Cannot open file {0}", path);
                return false;
            }

            // Format template
            const uint32 length = file->GetLength();
            List<char> tmp;
            for (uint32 i = 0; i < length; i++)
            {
                char c = file->ReadByte();

                if (c != '@')
                {
                    source.WriteByte(c);
                }
                else
                {
                    i++;
                    int32 inIndex = file->ReadByte() - '0';
                    ASSERT_LOW_LAYER(Math::IsInRange(inIndex, 0, In_MAX - 1));

                    const String& in = inputs[inIndex];
                    if (in.Length() > 0)
                    {
                        tmp.EnsureCapacity(in.Length() + 1, false);
                        StringUtils::ConvertUTF162ANSI(*in, tmp.Get(), in.Length());
                        source.WriteBytes(tmp.Get(), in.Length());
                    }
                }
            }

            // Close file
            Delete(file);

            // Ensure to have null-terminated source code
            source.WriteByte(0);
        }

        return true;
    }

    void MaterialGenerator::clearCache()
    {
        for (int32 i = 0; i < _layers.Count(); i++)
            _layers[i]->ClearCache();
        for (auto& e : _functions)
        {
            for (auto& node : e.Value->Nodes)
            {
                for (int32 j = 0; j < node.Boxes.Count(); j++)
                    node.Boxes[j].Cache.Clear();
            }
        }
        _ddx = Value();
        _ddy = Value();
        _cameraVector = Value();
    }

    void MaterialGenerator::writeBlending(MaterialGraphBoxes box, Value& result, const Value& bottom, const Value& top, const Value& alpha)
    {
        const auto& boxInfo = GetMaterialRootNodeBox(box);
        _writer.Write(SE_TEXT("\t{0}.{1} = lerp({2}.{1}, {3}.{1}, {4});\n"), result.Value, boxInfo.SubName, bottom.Value, top.Value, alpha.Value);
        if (box == MaterialGraphBoxes::Normal)
        {
            _writer.Write(SE_TEXT("\t{0}.{1} = normalize({0}.{1});\n"), result.Value, boxInfo.SubName);
        }
    }

    SerializedMaterialParam* MaterialGenerator::findParam(const UID& id, MaterialLayer* layer)
    {
        // Use per material layer params mapping
        return findParam(layer->GetMappedParamId(id));
    }

    MaterialGraphParameter* MaterialGenerator::findGraphParam(const UID& id)
    {
        MaterialGraphParameter* result = nullptr;

        for (int32 i = 0; i < _layers.Count(); i++)
        {
            result = _layers[i]->Graph.GetParameter(id);
            if (result)
            {
                break;
            }
        }

        return result;
    }

    void MaterialGenerator::createGradients(Node* caller)
    {
        if (_ddx.IsInvalid())
            _ddx = writeLocal(VariantTypes::Float2, SE_TEXT("ddx(input.TexCoord.xy)"), caller);
        if (_ddy.IsInvalid())
            _ddy = writeLocal(VariantTypes::Float2, SE_TEXT("ddy(input.TexCoord.xy)"), caller);
    }

    MaterialGenerator::Value MaterialGenerator::getCameraVector(Node* caller)
    {
        if (_cameraVector.IsInvalid())
            _cameraVector = writeLocal(VariantTypes::Float3, SE_TEXT("normalize(ViewPos.xyz - input.WorldPosition.xyz)"), caller);
        return _cameraVector;
    }

    void MaterialGenerator::eatMaterialGraphBox(String& layerVarName, MaterialGraphBox* nodeBox, MaterialGraphBoxes box)
    {
        // Cache data
        auto& boxInfo = GetMaterialRootNodeBox(box);

        // Get value
        const auto value = Value::Cast(tryGetValue(nodeBox, boxInfo.DefaultValue), boxInfo.DefaultValue.Type);

        // Write formatted value
        _writer.WriteLine(SE_TEXT("\t{0}.{1} = {2};"), layerVarName, boxInfo.SubName, value.Value);
    }

    void MaterialGenerator::eatMaterialGraphBox(MaterialLayer* layer, MaterialGraphBoxes box)
    {
        auto& boxInfo = GetMaterialRootNodeBox(box);
        const auto nodeBox = layer->Root->GetBox(boxInfo.ID);
        eatMaterialGraphBox(_treeLayerVarName, nodeBox, box);
    }

    void MaterialGenerator::eatMaterialGraphBoxWithDefault(MaterialLayer* layer, MaterialGraphBoxes box)
    {
        auto& boxInfo = GetMaterialRootNodeBox(box);
        _writer.WriteLine(SE_TEXT("\t{0}.{1} = {2};"), _treeLayerVarName, boxInfo.SubName, boxInfo.DefaultValue.Value);
    }

    void MaterialGenerator::ProcessGroupMath(Box* box, Node* node, Value& value)
    {
        switch (node->TypeID)
        {
            // Vector Transform
        case 30:
        {
            // Get input vector
            auto v = tryGetValue(node->GetBox(0), Value::InitForZero(VariantTypes::Float3));

            // Select transformation spaces
            ENGINE_ASSERT(node->Values[0].Type == VariantTypes::Int && node->Values[1].Type == VariantTypes::Int);
            ENGINE_ASSERT(Math::RangeInclusive(node->Values[0].AsInt, 0, (int32)TransformCoordinateSystem::MAX - 1));
            ENGINE_ASSERT(Math::RangeInclusive(node->Values[1].AsInt, 0, (int32)TransformCoordinateSystem::MAX - 1));
            auto inputType = static_cast<TransformCoordinateSystem>(node->Values[0].AsInt);
            auto outputType = static_cast<TransformCoordinateSystem>(node->Values[1].AsInt);
            if (inputType == outputType)
            {
                // No space change at all
                value = v;
            }
            else
            {
                // Switch by source space type
                const Char* format = nullptr;
                switch (inputType)
                {
                case TransformCoordinateSystem::Tangent:
                    switch (outputType)
                    {
                case TransformCoordinateSystem::Tangent:
                    format = SE_TEXT("{0}");
                        break;
                case TransformCoordinateSystem::World:
                    format = SE_TEXT("TransformTangentVectorToWorld(input, {0})");
                        break;
                case TransformCoordinateSystem::View:
                    format = SE_TEXT("TransformWorldVectorToView(input, TransformTangentVectorToWorld(input, {0}))");
                        break;
                case TransformCoordinateSystem::Local:
                    format = SE_TEXT("TransformWorldVectorToLocal(input, TransformTangentVectorToWorld(input, {0}))");
                        break;
                    }
                    break;
                case TransformCoordinateSystem::World:
                    switch (outputType)
                    {
                case TransformCoordinateSystem::Tangent:
                    format = SE_TEXT("TransformWorldVectorToTangent(input, {0})");
                        break;
                case TransformCoordinateSystem::World:
                    format = SE_TEXT("{0}");
                        break;
                case TransformCoordinateSystem::View:
                    format = SE_TEXT("TransformWorldVectorToView(input, {0})");
                        break;
                case TransformCoordinateSystem::Local:
                    format = SE_TEXT("TransformWorldVectorToLocal(input, {0})");
                        break;
                    }
                    break;
                case TransformCoordinateSystem::View:
                    switch (outputType)
                    {
                case TransformCoordinateSystem::Tangent:
                    format = SE_TEXT("TransformWorldVectorToTangent(input, TransformViewVectorToWorld(input, {0}))");
                        break;
                case TransformCoordinateSystem::World:
                    format = SE_TEXT("TransformViewVectorToWorld(input, {0})");
                        break;
                case TransformCoordinateSystem::View:
                    format = SE_TEXT("{0}");
                        break;
                case TransformCoordinateSystem::Local:
                    format = SE_TEXT("TransformWorldVectorToLocal(input, TransformViewVectorToWorld(input, {0}))");
                        break;
                    }
                    break;
                case TransformCoordinateSystem::Local:
                    switch (outputType)
                    {
                case TransformCoordinateSystem::Tangent:
                    format = SE_TEXT("TransformWorldVectorToTangent(input, TransformLocalVectorToWorld(input, {0}))");
                        break;
                case TransformCoordinateSystem::World:
                    format = SE_TEXT("TransformLocalVectorToWorld(input, {0})");
                        break;
                case TransformCoordinateSystem::View:
                    format = SE_TEXT("TransformWorldVectorToView(input, TransformLocalVectorToWorld(input, {0}))");
                        break;
                case TransformCoordinateSystem::Local:
                    format = SE_TEXT("{0}");
                        break;
                    }
                    break;
                }
                ENGINE_ASSERT(format != nullptr);

                // Write operation
                value = writeLocal(VariantTypes::Float3, String::Format(format, v.Value), node);
            }
            break;
        }
        default:
            ShaderGenerator::ProcessGroupMath(box, node, value);
            break;
        }
    }

    void MaterialGenerator::WriteCustomGlobalCode(const List<const MaterialGraph::Node*, InlinedAllocation<8>>& nodes, int32 templateInputsMapping)
    {
        for (const MaterialGraph::Node* node : nodes)
        {
            if ((int32)node->Values[2] == templateInputsMapping)
            {
                _writer.Write(SE_TEXT("\n"));
                _writer.Write((StringView)node->Values[0]);
                _writer.Write(SE_TEXT("\n"));
            }
        }
    }

    MaterialGenerator::MaterialGraphBoxesMapping MaterialGenerator::MaterialGraphBoxesMappings[] =
    {
        { 0, nullptr, MaterialTreeType::PixelShader, MaterialValue::Zero },
        { 1, SE_TEXT("Color"), MaterialTreeType::PixelShader, MaterialValue::InitForOne(VariantTypes::Float3) },
        { 2, SE_TEXT("Mask"), MaterialTreeType::PixelShader, MaterialValue::InitForOne(VariantTypes::Float) },
        { 3, SE_TEXT("Emissive"), MaterialTreeType::PixelShader, MaterialValue::InitForZero(VariantTypes::Float3) },
        { 4, SE_TEXT("Metalness"), MaterialTreeType::PixelShader, MaterialValue::InitForZero(VariantTypes::Float) },
        { 5, SE_TEXT("Specular"), MaterialTreeType::PixelShader, MaterialValue::InitForHalf(VariantTypes::Float) },
        { 6, SE_TEXT("Roughness"), MaterialTreeType::PixelShader, MaterialValue::InitForHalf(VariantTypes::Float) },
        { 7, SE_TEXT("AO"), MaterialTreeType::PixelShader, MaterialValue::InitForOne(VariantTypes::Float) },
        { 8, SE_TEXT("TangentNormal"), MaterialTreeType::PixelShader, MaterialValue(VariantTypes::Float3, SE_TEXT("float3(0, 0, 1.0)")) },
        { 9, SE_TEXT("Opacity"), MaterialTreeType::PixelShader, MaterialValue::InitForOne(VariantTypes::Float) },
        { 10, SE_TEXT("Refraction"), MaterialTreeType::PixelShader, MaterialValue::InitForOne(VariantTypes::Float) },
        { 11, SE_TEXT("PositionOffset"), MaterialTreeType::VertexShader, MaterialValue::InitForZero(VariantTypes::Float3) },
        { 12, SE_TEXT("TessellationMultiplier"), MaterialTreeType::VertexShader, MaterialValue(VariantTypes::Float, SE_TEXT("4.0f")) },
        { 13, SE_TEXT("WorldDisplacement"), MaterialTreeType::DomainShader, MaterialValue::InitForZero(VariantTypes::Float3) },
        { 14, SE_TEXT("SubsurfaceColor"), MaterialTreeType::PixelShader, MaterialValue::InitForZero(VariantTypes::Float3) },
    };
    
    const MaterialGenerator::MaterialGraphBoxesMapping& MaterialGenerator::GetMaterialRootNodeBox(MaterialGraphBoxes box)
    {
        static_assert(ARRAY_SIZE(MaterialGenerator::MaterialGraphBoxesMappings) == 15, "Invalid amount of boxes added for root node. Please update the code above");
        return MaterialGraphBoxesMappings[static_cast<int32>(box)];
    }
}
