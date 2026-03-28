#include "ModelBase.h"

#include "Core/Thread/Threading.h"

namespace SE
{
	void ModelBase::SetupMaterialSlots(int32 slotsCount)
	{
		ENGINE_ASSERT(slotsCount >= 0 && slotsCount < 4096);
		if (!IsVirtual() && WaitForLoaded())
			return;

		Threading::ScopeLock lock(Locker);

		const int32 prevCount = MaterialSlots.Count();
		MaterialSlots.Resize(slotsCount, false);

		// Initialize slot names
		for (int32 i = prevCount; i < slotsCount; i++)
			MaterialSlots[i].Name = String::Format(SE_TEXT("Material {0}"), i + 1);
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
