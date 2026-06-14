

#include "MaterialLayer.h"
#include "MaterialGenerator.h"

namespace SE
{
    MaterialLayer::MaterialLayer(const UID& id)
        : ID(id)
        , Root(nullptr)
        , FeaturesFlags(MaterialFeatures::None)
        , UsageFlags(MaterialUsage::None)
        , Domain(MaterialDomain::Surface)
        , BlendMode(MaterialBlendMode::Opaque)
        , ShadingModel(MaterialShadingModel::Lit)
        , MaskThreshold(0.3f)
        , OpacityThreshold(0.12f)
        , ParamIdsMappings(8)
    {
        ENGINE_ASSERT(ID.IsValid());
    }

    void MaterialLayer::ClearCache()
    {
        for (int32 i = 0; i < Graph.Nodes.Count(); i++)
        {
            auto& node = Graph.Nodes[i];
            for (int32 j = 0; j < node.Boxes.Count(); j++)
            {
                node.Boxes[j].Cache.Clear();
            }
        }
        for (int32 i = 0; i < ARRAY_SIZE(Usage); i++)
        {
            Usage[i].VarName.Clear();
            Usage[i].Hint = nullptr;
        }
    }

    void MaterialLayer::Prepare()
    {
        // Clear cached data
        ClearCache();

        // Ensure to have root node set
        if (Root == nullptr)
        {
            for (int32 i = 0; i < Graph.Nodes.Count(); i++)
            {
                const auto node = &Graph.Nodes[i];
                if (node->Type == ROOT_NODE_TYPE)
                {
                    Root = node;
                    break;
                }
            }
            if (Root == nullptr)
            {
                // Ensure to have root node
                createRootNode();
            }
        }
    }

    UID MaterialLayer::GetMappedParamId(const UID& id)
    {
        // TODO: test ParamIdsMappings using Dictionary. will performance change? maybe we don't wont to allocate too much memory

        for (int32 i = 0; i < ParamIdsMappings.Count(); i++)
        {
            if (ParamIdsMappings[i].SrcId == id)
                return ParamIdsMappings[i].DstId;
        }

        return UID::Empty;
    }

    String& MaterialLayer::GetVariableName(void* hint)
    {
        if (hint == nullptr)
        {
            return Usage[0].VarName;
        }

        for (int32 i = 1; i < ARRAY_SIZE(Usage); i++)
        {
            if (Usage[i].Hint == nullptr)
            {
                Usage[i].Hint = hint;
            }

            if (Usage[i].Hint == hint)
            {
                return Usage[i].VarName;
            }
        }

        LOG_ERROR("Resource", "Too many layer samples per material! Layer {0}", ID);
        return Usage[0].VarName;
    }

    bool MaterialLayer::HasAnyVariableName()
    {
        for (int32 i = 1; i < ARRAY_SIZE(Usage); i++)
        {
            if (Usage[i].Hint || Usage[i].VarName.HasChars())
            {
                return true;
            }
        }

        return false;
    }

    void MaterialLayer::UpdateFeaturesFlags()
    {
        ENGINE_ASSERT(Root);

        // Gather the material features usage
        UsageFlags = MaterialUsage::None;

#define CHECK_BOX_AS_FEATURE(box, feature) \
if (Root->GetBox(static_cast<int32>(MaterialGraphBoxes::box))->HasConnection()) \
UsageFlags.SetFlag(MaterialUsage::feature); \
else \
UsageFlags.RemoveFlag(MaterialUsage::feature);
        CHECK_BOX_AS_FEATURE(Emissive, UseEmissive);
        CHECK_BOX_AS_FEATURE(Normal, UseNormal);
        CHECK_BOX_AS_FEATURE(Mask, UseMask);
        CHECK_BOX_AS_FEATURE(PositionOffset, UsePositionOffset);
        CHECK_BOX_AS_FEATURE(WorldDisplacement, UseDisplacement);
        CHECK_BOX_AS_FEATURE(Refraction, UseRefraction);
#undef CHECK_BOX_AS_FEATURE
    }

    MaterialLayer* MaterialLayer::CreateDefault(const UID& id)
    {
        // Create new layer object
        auto layer = New<MaterialLayer>(id);

        // Create default root node
        layer->createRootNode();

        return layer;
    }

    MaterialLayer* MaterialLayer::Load(const UID& id, ReadStream* graphData, const MaterialInfo& info, const String& caller)
    {
        // Create new layer object
        auto layer = New<MaterialLayer>(id);

        // Load graph
        if (layer->Graph.Load(graphData, false))
        {
            LOG_WARNING("Resource", "Cannot load graph '{0}'.", caller);
        }

        // Find root node
        for (int32 i = 0; i < layer->Graph.Nodes.Count(); i++)
        {
            if (layer->Graph.Nodes[i].Type == ROOT_NODE_TYPE)
            {
                layer->Root = &layer->Graph.Nodes[i];
                break;
            }
        }

        // Ensure to have root node
        if (layer->Root == nullptr)
        {
            LOG_WARNING("Resource", "Missing root node in '{0}'.", caller);
            layer->createRootNode();
        }
        // Ensure to have valid root node
        else if (layer->Root->Boxes.Count() < static_cast<int32>(MaterialGraphBoxes::MAX))
        {
#define ADD_BOX(type, valueType) \
if (layer->Root->Boxes.Count() <= static_cast<int32>(MaterialGraphBoxes::type)) \
layer->Root->Boxes.Add(MaterialGraphBox(layer->Root, static_cast<int32>(MaterialGraphBoxes::type), VariantTypes::valueType))
            ADD_BOX(TessellationMultiplier, Float);
            ADD_BOX(WorldDisplacement, Float3);
            ADD_BOX(SubsurfaceColor, Float3);
            static_assert(static_cast<int32>(MaterialGraphBoxes::MAX) == 15, "Invalid amount of boxes added for root node. Please update the code above");
            ENGINE_ASSERT(layer->Root->Boxes.Count() == static_cast<int32>(MaterialGraphBoxes::MAX));
#if BUILD_DEBUG
            // Test for valid pointers after node upgrade
            for (int32 i = 0; i < layer->Root->Boxes.Count(); i++)
            {
                if (layer->Root->Boxes[i].Parent != layer->Root)
                {
                    CRASH;
                }
            }
#endif
#undef ADD_BOX
        }

        // Setup
        layer->FeaturesFlags = info.FeaturesFlags;
        layer->UsageFlags = info.UsageFlags;
        layer->Domain = info.Domain;
        layer->BlendMode = info.BlendMode;
        layer->ShadingModel = info.ShadingModel;
        layer->MaskThreshold = info.MaskThreshold;
        layer->OpacityThreshold = info.OpacityThreshold;

        return layer;
    }

    void MaterialLayer::createRootNode()
    {
        // Create node
        auto& rootNode = Graph.Nodes.AddOne();
        rootNode.ID = 1;
        rootNode.Type = ROOT_NODE_TYPE;
        rootNode.Boxes.Resize(static_cast<int32>(MaterialGraphBoxes::MAX));
#define INIT_BOX(type, valueType) rootNode.Boxes[static_cast<int32>(MaterialGraphBoxes::type)] = MaterialGraphBox(&rootNode, static_cast<int32>(MaterialGraphBoxes::type), VariantTypes::valueType)
        INIT_BOX(Layer, Void);
        INIT_BOX(Color, Float3);
        INIT_BOX(Mask, Float);
        INIT_BOX(Emissive, Float3);
        INIT_BOX(Metalness, Float);
        INIT_BOX(Specular, Float);
        INIT_BOX(Roughness, Float);
        INIT_BOX(AmbientOcclusion, Float);
        INIT_BOX(Normal, Float3);
        INIT_BOX(Opacity, Float);
        INIT_BOX(Refraction, Float);
        INIT_BOX(PositionOffset, Float3);
        INIT_BOX(TessellationMultiplier, Float);
        INIT_BOX(WorldDisplacement, Float3);
        INIT_BOX(SubsurfaceColor, Float3);
        static_assert(static_cast<int32>(MaterialGraphBoxes::MAX) == 15, "Invalid amount of boxes created for root node. Please update the code above");
#undef INIT_BOX

        // Mark as root
        Root = &rootNode;
    }
}