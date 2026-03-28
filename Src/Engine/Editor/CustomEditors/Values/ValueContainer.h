#pragma once

#include "Core/Types/Collections/List.h"
#include "Core/TypeSystem/TypeID.h"
#include "Core/TypeSystem/Property/TypeProperty.h"
#include "Core/TypeSystem/MetaData/TypeMetaContainer.h"

namespace SE::Editor
{
	struct ValueContent
	{
		void* instance = nullptr;
		const TypeProperty* property = nullptr;

		TypeID GetType() const;
		bool HasNull() const;

		bool operator==(ValueContent v) const;
		bool operator!=(ValueContent v) const;

		ValueContent();
		ValueContent(const TypeProperty* info, void* instance);
		ValueContent(IType* obj);
	};


	/// <summary>
	/// Container for values being edited, supporting single and multi-object editing.
	/// </summary>
	class ValueContainer
	{
	public:
		virtual ~ValueContainer() = default;

		// Value access
		int Count() const { return m_values.Count(); }
		void Add(const TypeProperty* info, void* instance);
		void Add(IType* obj);
		void Clear() { m_values.Clear(); }

		ValueContent& operator[](int index) { return m_values[index]; }
		const ValueContent& operator[](int index) const { return m_values[index]; }
		ValueContent& At(int index) { return m_values[index]; }
		const ValueContent& At(int index) const { return m_values[index]; }

		// Type information
		List<TypeID> GetValuesTypes();

		// State queries
		bool IsSingleObject() { return m_values.Count() == 1; }
		bool HasDifferentValues();
		bool HasDifferentTypes();
		bool HasNull();
		bool IsNull();
		bool HasValueType();
		bool IsArray();

		// Default value management
		bool HasDefaultValue() const { return m_hasDefaultValue; }
		void* GetDefaultValue() { return m_defaultValue; }
		bool IsDefaultValueModified() const;
		void SetDefaultValue(void* value);
		void RefreshDefaultValue(void* instanceValue);
		void ClearDefaultValue();

		// Reference value management (for prefab system)
		bool HasReferenceValue() const { return m_hasReferenceValue; }
		void* GetReferenceValue() { return m_referenceValue; }
		bool IsReferenceValueModified() const;
		void SetReferenceValue(void* value);
		void RefreshReferenceValue(void* instanceValue);
		void ClearReferenceValue();

		// Value synchronization
		virtual void Refresh(ValueContainer* instanceValues);
		virtual void Set(ValueContainer* instanceValues, const ValueContent& value);
		virtual void Set(ValueContainer* instanceValues, ValueContainer* values);
		virtual void Set(ValueContainer* instanceValues);

		// Metadata access
		virtual List<TypeMetaAttribute*> GetAttributes() const;

	protected:
		List<ValueContent> m_values;

		bool m_hasDefaultValue = false;
		void* m_defaultValue = nullptr;

		bool m_hasReferenceValue = false;
		void* m_referenceValue = nullptr;
	};

} // namespace SE::Editor
