#include "ModelBase.h"

#include "Runtime/Core/Thread/Threading.h"

#include "MeshBase.h"
#include "MeshDeformation.h"

namespace SE
{
    BoundingBox ModelLODBase::GetBox(const Matrix& world) const
	{
		Float3 tmp, min = Float3::Maximum, max = Float3::Minimum;
		Float3 corners[8];

		int meshCount = GetMeshesCount();
		for (int32 meshIndex = 0; meshIndex < meshCount; meshIndex++)
		{
		    const MeshBase* mesh = GetMesh(meshIndex);
			mesh->GetBox().GetCorners(corners);
			for (int32 i = 0; i < 8; i++)
			{
				Float3::Transform(corners[i], world, tmp);
				min = Float3::Min(min, tmp);
				max = Float3::Max(max, tmp);
			}
		}
		return BoundingBox(min, max);
	}

	BoundingBox ModelLODBase::GetBox(const Transform& transform, const MeshDeformation* deformation) const
	{
		Float3 tmp, min = Float3::Maximum, max = Float3::Minimum;
		Float3 corners[8];

		int meshCount = GetMeshesCount();
		for (int32 meshIndex = 0; meshIndex < meshCount; meshIndex++)
		{
			const MeshBase* mesh = GetMesh(meshIndex);
			BoundingBox box = mesh->GetBox();
			if (deformation)
			{
				deformation->GetBounds(m_LodIndex, meshIndex, box);
			}
			box.GetCorners(corners);
			for (int32 i = 0; i < 8; i++)
			{
				transform.LocalToWorld(corners[i], tmp);
				min = Float3::Min(min, tmp);
				max = Float3::Max(max, tmp);
			}
		}
		return BoundingBox(min, max);
	}

	BoundingBox ModelLODBase::GetBox() const
	{
		Float3 min = Float3::Maximum, max = Float3::Minimum;
		Float3 corners[8];

		int meshCount = GetMeshesCount();
		for (int32 meshIndex = 0; meshIndex < meshCount; meshIndex++)
		{
			const MeshBase* mesh = GetMesh(meshIndex);
			mesh->GetBox().GetCorners(corners);
			for (int32 i = 0; i < 8; i++)
			{
				min = Float3::Min(min, corners[i]);
				max = Float3::Max(max, corners[i]);
			}
		}
		return BoundingBox(min, max);
	}



    MaterialBase* ModelBase::GetMaterial(int32 slotIndex) const
    {
        ENGINE_ASSERT(IsLoaded() && slotIndex >= 0 && slotIndex < MaterialSlots.Count());
        return MaterialSlots[slotIndex].Material.Get();
    }

    void ModelBase::SetMaterial(int32 slotIndex, MaterialBase* material)
    {
		ENGINE_ASSERT(IsLoaded() && slotIndex >= 0 && slotIndex < MaterialSlots.Count());
        return MaterialSlots[slotIndex].Material.Set(material);
    }

    void ModelBase::SetupMaterialSlots(int32 slotsCount)
	{
		ENGINE_ASSERT(slotsCount >= 0 && slotsCount < 4096);
		if (!IsVirtual() && !WaitForLoaded())
		{
		    return;
		}

		Threading::ScopeLock lock(Locker);

		const int32 prevCount = MaterialSlots.Count();
		MaterialSlots.Resize(slotsCount, false);

		// Initialize slot names
		for (int32 i = prevCount; i < slotsCount; i++)
		{
		    MaterialSlots[i].Name = String::Format(SE_TEXT("Material {0}"), i + 1);
		}
	}

	MaterialSlot* ModelBase::GetSlot(const StringView& name)
	{
		MaterialSlot* result = nullptr;
		for (auto& slot : MaterialSlots)
		{
			if (slot.Name == name)
			{
				result = &slot;
				break;
			}
		}
		return result;
	}

}
