#pragma once
#include "Core/Types/Collections/Dictionary.h"
#include "Core/TypeSystem/Types.h"

namespace SE
{
	class TypeMetaAttribute;

	class SE_API_CORE TypeMetadataContainer
	{
	public:
		template<typename T>
		bool Has() const
		{
			return Has(Typeof<T>());
		}
		bool Has(TypeID const& metaType) const;

		const TypeMetaAttribute* Find(const TypeID& metaType) const;

		template<typename T>
		const T* Find() const
		{
			const TypeID tid = Typeof<T>();
			return static_cast<const T*>(Find(tid));
		}

		void Add(TypeID const& metaType, TypeMetaAttribute *meta);

		void GetAll(List<TypeMetaAttribute*>& metaList);

	private:
		Dictionary<TypeID, TypeMetaAttribute*> m_MetaDict;
	};
} // SE

