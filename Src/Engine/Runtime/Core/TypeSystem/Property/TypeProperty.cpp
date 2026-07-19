#include "TypeProperty.h"
#include "Runtime/Core/TypeSystem/MetaData/TypeMetaContainer.h"

//-------------------------------------------------------------------------

namespace SE
{
	TypeProperty::TypeProperty() : metaContainer(New<TypeMetadataContainer>())
	{

	}

	TypeProperty::~TypeProperty()
	{
		Delete(metaContainer);
		metaContainer = nullptr;
	}

	void* TypeProperty::GetPropertyAddress(void *pTypeAddress) const
	{
		ENGINE_ASSERT(id != StringID::Invalid);
		return reinterpret_cast<uint8 *>(pTypeAddress) + offset;
	}

	void const* TypeProperty::GetPropertyAddress(void const *pTypeAddress) const
	{
		ENGINE_ASSERT(id != StringID::Invalid);
		return reinterpret_cast<uint8 const *>(pTypeAddress) + offset;
	}

	void const* TypeProperty::GetArrayDefaultElementPtr(int32 elementIdx) const
	{
		ENGINE_ASSERT(IsArrayProperty() && pDefaultArrayData != nullptr);
		ENGINE_ASSERT(elementIdx >= 0 && elementIdx < arraySize);
		ENGINE_ASSERT(arraySize > 0 && arrayElementSize > 0);
		uint8 const *arrayDataPtr = (uint8 const *)pDefaultArrayData;
		return arrayDataPtr + (arrayElementSize * elementIdx);
	}

}
