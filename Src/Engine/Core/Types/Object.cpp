#include "Object.h"

namespace SE
{
	void Object::DeleteObjectNow()
	{
		// ObjectsRemovalService::Dereference(this);

		OnDeleteObject();
	}

	void Object::DeleteObject(float timeToLive, bool useGameTime)
	{
	}

}
