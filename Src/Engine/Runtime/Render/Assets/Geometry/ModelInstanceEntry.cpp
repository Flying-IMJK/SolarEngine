
#include "ModelInstanceEntry.h"
#include "Model.h"

#include "Core/Serialization/Serialization.h"

namespace SE
{
    bool ModelInstanceEntries::HasContentLoaded() const
    {
        bool result = true;
        for (auto& e : *this)
        {
            const auto material = e.Material.Get();
            if (material && !material->IsLoaded())
            {
                result = false;
                break;
            }
        }
        return result;
    }

    void ModelInstanceEntries::Serialize(SerializeContext& context)
    {
        SERIALIZE_GET_OTHER_OBJ(ModelInstanceEntries, context.otherObj);

        context.stream.JKEY("Entries");
        context.stream.StartArray();
        if (other && other->Count() == Count())
        {
            for (int32 i = 0; i < Count(); i++)
                context.stream.Object(&At(i), &other->At(i));
        }
        else
        {
            for (auto& e : *this)
                context.stream.Object(&e, nullptr);
        }
        context.stream.EndArray();
    }

    void ModelInstanceEntries::Deserialize(DeserializeContext& context)
    {
        const DeserializeStream& entries = context.stream->operator[]("Entries");
        ENGINE_ASSERT(entries.IsArray());
        Resize(entries.Size());
        for (rapidjson::SizeType i = 0; i < entries.Size(); i++)
        {
            DeserializeContext::StreamScope scope(context, entries[i]);
            At(i).Deserialize(context);
        }
    }

    bool ModelInstanceEntry::operator==(const ModelInstanceEntry& other) const
    {
        return Material.Get() == other.Material.Get() && ShadowsMode == other.ShadowsMode && Visible == other.Visible && ReceiveDecals == other.ReceiveDecals;
    }

    void ModelInstanceEntry::Serialize(SerializeContext& context)
    {

    }

    void ModelInstanceEntry::Deserialize(DeserializeContext& context)
    {

    }

    bool ModelInstanceEntries::IsValidFor(const Model* model) const
    {
        // Just check amount of material slots
        ENGINE_ASSERT(model && model->IsInitialized());
        return model->MaterialSlots.Count() == Count();
    }

    /*bool ModelInstanceEntries::IsValidFor(const SkinnedModel* model) const
    {
        // Just check amount of material slots
        ASSERT(model && model->IsInitialized());
        return model->MaterialSlots.Count() == Count();
    }*/

    void ModelInstanceEntries::Setup(const Model* model)
    {
        ASSERT(model && model->IsInitialized());
        const int32 slotsCount = model->MaterialSlots.Count();
        Setup(slotsCount);
    }

    /*void ModelInstanceEntries::Setup(const SkinnedModel* model)
    {
        ASSERT(model && model->IsInitialized());
        const int32 slotsCount = model->MaterialSlots.Count();
        Setup(slotsCount);
    }*/

    void ModelInstanceEntries::Setup(int32 slotsCount)
    {
        Clear();
        Resize(slotsCount);
    }

    void ModelInstanceEntries::SetupIfInvalid(const Model* model)
    {
        if (!IsValidFor(model))
        {
            Setup(model);
        }
    }

    /*void ModelInstanceEntries::SetupIfInvalid(const SkinnedModel* model)
    {
        if (!IsValidFor(model))
        {
            Setup(model);
        }
    }*/

} // SE