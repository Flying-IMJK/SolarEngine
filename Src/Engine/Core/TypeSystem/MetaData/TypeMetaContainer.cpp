
#include "TypeMetaContainer.h"

namespace SE
{
	bool TypeMetadataContainer::Has(TypeID const& metaType) const
	{
		return m_MetaDict.ContainsKey(metaType);
	}

	const TypeMetaAttribute* TypeMetadataContainer::Find(const TypeID& metaType) const
	{
		TypeMetaAttribute* meta;
		if (m_MetaDict.TryGet(metaType, meta))
		{
			return meta;
		}
		return nullptr;
	}

	void TypeMetadataContainer::Add(TypeID const& metaType, TypeMetaAttribute* meta)
	{
		m_MetaDict.Add(metaType, meta);
	}

	void TypeMetadataContainer::GetAll(List<TypeMetaAttribute*>& metaList)
	{
		m_MetaDict.GetValues(metaList);
	}
} // SE