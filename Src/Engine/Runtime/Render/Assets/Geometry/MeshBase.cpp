#include "MeshBase.h"

#include "ModelBase.h"

namespace SE
{
	void MeshBase::SetMaterialSlotIndex(int32 value)
	{
		if (value < 0 || value >= _model->MaterialSlots.Count())
		{
			LOG_WARNING("Render", "Cannot set mesh material slot to {0} while model has {1} slots.", value, _model->MaterialSlots.Count());
			return;
		}

		_materialSlotIndex = value;
	}

	void MeshBase::SetBounds(const BoundingBox& box)
	{
		_box = box;
		BoundingSphere::FromBox(box, _sphere);
	}

}
