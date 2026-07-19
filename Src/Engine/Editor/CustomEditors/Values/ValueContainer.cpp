#include "ValueContainer.h"
#include "Runtime/Core/TypeSystem/IType.h"

namespace SE::Editor
{
	TypeID ValueContent::GetType() const
	{
		if (property == nullptr)
		{
			return static_cast<IType*>(instance)->GetType();
		}
		return property->typeID;
	}

	bool ValueContent::HasNull() const
	{
		return instance == nullptr;
	}

	bool ValueContent::operator==(ValueContent v) const
	{
		if (property == nullptr && v.property == nullptr)
		{
			return instance == v.instance;
		}
		else if (property != nullptr && v.property != nullptr)
		{
			return property->GetPropertyAddress(instance) == v.property->GetPropertyAddress(instance);
		}

		return false;
	}

	bool ValueContent::operator!=(ValueContent v) const
	{
		if (property == nullptr && v.property == nullptr)
		{
			return instance != v.instance;
		}
		else if (property != nullptr && v.property != nullptr)
		{
			return property->GetPropertyAddress(instance) != v.property->GetPropertyAddress(instance);
		}

		return true;
	}

	ValueContent::ValueContent() : instance(nullptr), property(nullptr)
	{
	}

	ValueContent::ValueContent(const TypeProperty* info, void* instance) : instance(instance), property(info)
	{

	}

	ValueContent::ValueContent(IType* obj) : instance(obj), property(nullptr)
	{

	}

	void ValueContainer::Add(const TypeProperty* info, void* instance)
	{
		ValueContent& content = m_values.AddOne();
		content.property = info;
		content.instance = instance;
	}

	void ValueContainer::Add(IType* obj)
	{
		ValueContent& content = m_values.AddOne();
		content.property = nullptr;
		content.instance = obj;;
	}

	List<TypeID> ValueContainer::GetValuesTypes()
	{
		List<TypeID> types;
		
		for (int i = 0; i < m_values.Count(); i++)
		{
			ValueContent& value = m_values[i];

			TypeID valueType = value.GetType();

			// Add only unique types
			bool found = false;
			for (int j = 0; j < types.Count(); j++)
			{
				if (types[j] == valueType)
				{
					found = true;
					break;
				}
			}

			if (!found)
			{
				types.Add(valueType);
			}
		}
		
		return types;
	}

	bool ValueContainer::HasDifferentValues()
	{
		if (m_values.Count() < 2)
			return false;

		// Compare all values with the first one
		ValueContent& firstValue = m_values[0];

		for (int i = 1; i < m_values.Count(); i++)
		{
			ValueContent& currentValue = m_values[i];

			// Use TypeCompositeInfo to compare all properties
			if (currentValue != firstValue)
				return true;



			/*if (firstValue.valueType == ValueContentType::Root &&
				!firstValue.root->GetTypeInfo()->AreAllPropertyValuesEqual(firstValue.root, currentValue.root))
			{
				return true;
			}*/

			/*if (firstValue.valueType == ValueContentType::Property)
			{
				TypeID typeId = firstValue.GetType();
				if (typeId.IsCoreType())
				{
					firstValue.property.
				}
				TypeCompositeInfo const* typeinfo = Types::GetTypeInfo();
			   !typeinfo->AreAllPropertyValuesEqual(firstValue.root, currentValue.root)
				return true;
			}*/
		}

		return false;
	}

	bool ValueContainer::HasDifferentTypes()
	{
		if (m_values.Count() < 2)
			return false;

		TypeID firstType = TypeID::Invalid;
		
		for (int i = 0; i < m_values.Count(); i++)
		{
			ValueContent& value = m_values[i];
			TypeID valueType = value.GetType();

			if (i == 0)
			{
				firstType = valueType;
			}
			else if (firstType != valueType)
			{
				return true;
			}
		}
		
		return false;
	}

	bool ValueContainer::HasNull()
	{
		for (int i = 0; i < m_values.Count(); i++)
		{
			if (m_values[i].HasNull())
				return true;
		}
		return false;
	}

	bool ValueContainer::IsNull()
	{
		if (m_values.Count() == 0)
			return true;

		for (int i = 0; i < m_values.Count(); i++)
		{
			if (!m_values[i].HasNull())
				return false;
		}
		return true;
	}

	bool ValueContainer::HasValueType()
	{
		// Check if any value is a value type (struct)
		/*const TypeCompositeInfo* typeInfo = Types::GetTypeInfo(m_type);
		if (typeInfo)
		{
			// In C++, all types are value types unless they're pointers
			// We can check if the type is a structure
			return m_info && m_info->IsStructureProperty();
		}*/
		return false;
	}

	bool ValueContainer::IsArray()
	{
		return false;
	}

	bool ValueContainer::IsDefaultValueModified() const
	{
		if (!m_hasDefaultValue)
			return false;

		/*const TypeCompositeInfo* typeInfo = Types::GetTypeInfo(m_type);
		if (!typeInfo)
			return false;

		IType* defaultInstance = static_cast<IType*>(m_defaultValue);
		if (!defaultInstance)
			return false;

		// Check if any value differs from default
		for (int i = 0; i < m_values.Count(); i++)
		{
			IType* currentValue = static_cast<IType*>(m_values[i]);
			if (!currentValue)
				return true;

			if (!typeInfo->AreAllPropertyValuesEqual(currentValue, defaultInstance))
				return true;
		}*/

		return false;
	}

	void ValueContainer::SetDefaultValue(void* value)
	{
		m_defaultValue = value;
		m_hasDefaultValue = true;
	}

	void ValueContainer::RefreshDefaultValue(void* instanceValue)
	{
		/*if (instanceValue && m_info)
		{
			m_defaultValue = m_info->GetPropertyAddress(instanceValue);
			m_hasDefaultValue = true;
		}*/
	}

	void ValueContainer::ClearDefaultValue()
	{
		m_defaultValue = nullptr;
		m_hasDefaultValue = false;
	}

	bool ValueContainer::IsReferenceValueModified() const
	{
		if (!m_hasReferenceValue)
			return false;

		/*const TypeCompositeInfo* typeInfo = Types::GetTypeInfo(m_type);
		if (!typeInfo)
			return false;

		IType* referenceInstance = static_cast<IType*>(m_referenceValue);
		if (!referenceInstance)
			return false;

		// Check if any value differs from reference
		for (int i = 0; i < m_values.Count(); i++)
		{
			IType* currentValue = static_cast<IType*>(m_values[i]);
			if (!currentValue)
				return true;

			if (!typeInfo->AreAllPropertyValuesEqual(currentValue, referenceInstance))
				return true;
		}*/

		return false;
	}

	void ValueContainer::SetReferenceValue(void* value)
	{
		m_referenceValue = value;
		m_hasReferenceValue = true;
	}

	void ValueContainer::RefreshReferenceValue(void* instanceValue)
	{
		/*if (instanceValue && m_info)
		{
			m_referenceValue = m_info->GetPropertyAddress(instanceValue);
			m_hasReferenceValue = true;
		}*/
	}

	void ValueContainer::ClearReferenceValue()
	{
		m_referenceValue = nullptr;
		m_hasReferenceValue = false;
	}

	void ValueContainer::Refresh(ValueContainer* instanceValues)
	{
		if (!instanceValues)
			return;

		// Refresh values from parent container
		m_values.Clear();
		m_values.Resize(instanceValues->Count());

		for (int i = 0; i < instanceValues->Count(); i++)
		{
			ValueContent& instance = (*instanceValues)[i];
			/*if (instance)
			{
				void* propValue = m_info->GetPropertyAddress(instance);
				m_values.Add(propValue);
			}
			else
			{
				m_values.Add(nullptr);
			}*/
		}
	}

	void ValueContainer::Set(ValueContainer* instanceValues, const ValueContent& value)
	{
		if (!instanceValues)
			return;

		// Apply value to all instances
		for (int i = 0; i < instanceValues->Count(); i++)
		{
			ValueContent& instance = instanceValues->At(i);
			/*void* propAddress = m_info->GetPropertyAddress(instance);
			if (propAddress)
			{
				// Use type-aware copying from Core/TypeSystem
				typeInfo->CopyValue(propAddress, value);
			}*/
		}

		// Refresh our values
		Refresh(instanceValues);
	}

	void ValueContainer::Set(ValueContainer* instanceValues, ValueContainer* values)
	{
		if (!instanceValues || !values || values->Count() == 0)
			return;

		// Use the first value from the values container
		ValueContent& value = (*values)[0];
		Set(instanceValues, value);
	}

	void ValueContainer::Set(ValueContainer* instanceValues)
	{
		if (!instanceValues || m_values.Count() == 0)
			return;

		// Use our first value
		ValueContent& value = m_values[0];
		Set(instanceValues, value);
	}

	List<TypeMetaAttribute*> ValueContainer::GetAttributes() const
	{
		List<TypeMetaAttribute*> attributes;
		
		/*
		if (m_info && m_info->metaContainer)
		{
			// Get all attributes from the property's metadata container
			m_info->metaContainer->GetAll(attributes);
		}
		*/

		return attributes;
	}

} // namespace SE::Editor
