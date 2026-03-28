#include "GenericEditor.h"
#include "../LayoutElementsContainer.h"
#include "../Values/ValueContainer.h"
#include "../CustomEditorFactory.h"
#include "../Element/LayoutElements.h"
#include "Core/Types/Collections/ListExtensions.h"
#include "Core/Types/Collections/Sorting.h"
#include "Core/TypeSystem/Info/TypeCompositeInfo.h"
#include "Core/TypeSystem/Property/TypeProperty.h"
#include "Core/TypeSystem/MetaData/ShowInEditorAttribute.h"
#include "Core/TypeSystem/MetaData/HideInEditorAttribute.h"
#include "Core/TypeSystem/MetaData/EditorOrderAttribute.h"
#include "Core/TypeSystem/MetaData/EditorDisplayAttribute.h"
#include "Core/TypeSystem/MetaData/ReadOnlyAttribute.h"
#include "Core/TypeSystem/MetaData/VisibleIfAttribute.h"
#include "Core/TypeSystem/MetaData/CustomEditorAttribute.h"
#include "Core/TypeSystem/MetaData/CustomEditorAliasAttribute.h"
#include "Core/TypeSystem/MetaData/SpaceAttribute.h"
#include "Core/TypeSystem/MetaData/HeaderAttribute.h"
#include "Editor/CustomEditors/Element/Container/PropertiesListElement.h"

namespace SE::Editor
{
	// ============================================================================
	// ItemInfo Implementation
	// ============================================================================

	GenericEditor::ItemInfo::ItemInfo(const TypeProperty* info)
		: info(info)
	{
		if (!info)
			return;

		// Extract attributes from metadata
		if (info->metaContainer)
		{
			order = info->metaContainer->Find<EditorOrderAttribute>();
			display = info->metaContainer->Find<EditorDisplayAttribute>();
			customEditor = info->metaContainer->Find<CustomEditorAttribute>();
			customEditorAlias = info->metaContainer->Find<CustomEditorAliasAttribute>();
			space = info->metaContainer->Find<SpaceAttribute>();
			header = info->metaContainer->Find<HeaderAttribute>();

			// Check for ReadOnly attribute
			isReadOnly = info->metaContainer->Has<ReadOnlyAttribute>();

			// Collect all VisibleIf attributes
			// TODO: Implement when TypeMetadataContainer supports getting all attributes of a type
			// VisibleIfs = info->metaContainer->FindAttributes<VisibleIfAttribute>();
		}

		// Set display name
		if (display && !display->Name.IsEmpty())
		{
			displayName = display->Name;
		}
		else
		{
			displayName = info->name;
		}

		// Set tooltip
		if (display && !display->Tooltip.IsEmpty())
		{
			tooltipText = display->Tooltip;
		}
	}

	ValueContainer* GenericEditor::ItemInfo::GetValues(ValueContainer* instanceValues)
	{
		if (!info || !instanceValues)
			return nullptr;

		// Create a value container for this property
		ValueContainer* valueContainer = New<ValueContainer>();
		valueContainer->Add(info, instanceValues->At(0).instance);
		return valueContainer;
	}

	CustomEditor* GenericEditor::ItemInfo::GetOverrideEditor()
	{
		if (customEditor && customEditor->EditorType != TypeID::Invalid)
		{
			return CustomEditorFactory::CreateEditor(customEditor->EditorType, false);
		}

		if (customEditorAlias && customEditorAlias->typeID != TypeID::Invalid)
		{
			return CustomEditorFactory::CreateEditor(customEditorAlias->typeID, false);
		}

		return nullptr;
	}

	int GenericEditor::ItemInfo::CompareTo(const ItemInfo& other) const
	{
		// Compare by order first
		int orderA = order ? order->Order : 0;
		int orderB = other.order ? other.order->Order : 0;

		if (orderA != orderB)
			return orderA - orderB;

		// Then by name
		return displayName.Compare(other.displayName);
	}

	bool GenericEditor::ItemInfo::CanMerge(const ItemInfo& a, const ItemInfo& b)
	{
		if (!a.info || !b.info)
			return false;

		// Check if names match
		if (a.info->name != b.info->name)
			return false;

		// Check if types match
		if (a.info->typeID != b.info->typeID)
			return false;

		return true;
	}

	// ============================================================================
	// GenericEditor Implementation
	// ============================================================================

	void GenericEditor::OnInitialize(LayoutElementsContainer* layout)
	{
		m_visibleIfCaches.Clear();
		m_isNull = false;

		// Collect editable items
		List<ItemInfo> items;

		if (!HasDifferentTypes())
		{
			ValueContainer* container = GetValues();
			ValueContent& value = container->At(0);

			// Get items for the object's type
			items = GetItemsForType(container->GetValuesTypes()[0]);
		}
		else
		{
			// Multi-type selection - merge common properties
			List<TypeID> types = GetValues()->GetValuesTypes();
			if (types.Count() == 0)
				return;

			items = GetItemsForType(types[0]);

			// Keep only common properties
			for (int i = 1; i < types.Count() && items.Count() > 0; i++)
			{
				List<ItemInfo> otherItems = GetItemsForType(types[i]);

				// Remove items that don't exist in other type
				for (int j = items.Count() - 1; j >= 0; j--)
				{
					bool found = false;
					for (const ItemInfo& otherItem : otherItems)
					{
						if (ItemInfo::CanMerge(items[j], otherItem))
						{
							found = true;
							break;
						}
					}
					if (!found)
					{
						items.RemoveAt(j);
					}
				}
			}
		}

		// Sort items by order and name
		Sorting::QuickSort(items, [](const ItemInfo& a, const ItemInfo& b) {
			return a.CompareTo(b) < 0;
		});

		// Build UI
		// TODO: Implement grouping logic when GroupElement is available
		for (ItemInfo& item : items)
		{
			// Add space if specified
			if (item.space)
			{
				layout->Space(item.space->Height);
			}

			// Add header if specified
			if (item.header && !item.header->Text.IsEmpty())
			{
				layout->Header(item.header->Text);
			}

			// Get value container for this property
			ValueContainer* itemValues = item.GetValues(GetValues());
			if (!itemValues)
				continue;

			// Determine layout container (might be in a group)
			LayoutElementsContainer* itemLayout = layout;
			// TODO: Implement grouping when GroupElement is available
			/*
			if (item.Display && !item.Display->Group.IsEmpty())
			{
				itemLayout = GetOrCreateGroup(layout, item.Display->Group);
			}
			*/

			// Spawn property editor
			SpawnProperty(itemLayout, itemValues, item);
		}
	}

	void GenericEditor::Refresh()
	{
		CustomEditor::Refresh();

		// Re-evaluate VisibleIf conditions
		if (m_isNull)
			return;

		for (const VisibleIfCache& cache : m_visibleIfCaches)
		{
			if (GetValues()->Count() == 0)
				continue;

			ValueContent& instance = GetValues()->At(0);

			bool visible = cache.GetValue(instance);

			// TODO: Update visibility when PropertiesListElement is available
			/*if (cache.PropertiesList && cache.LabelIndex >= 0)
			{
				cache.PropertiesList->SetPropertyVisible(cache.LabelIndex, visible);
			}
			else if (cache.Group)
			{
				cache.Group->SetVisible(visible);
			}*/
		}
	}

	List<GenericEditor::ItemInfo> GenericEditor::GetItemsForType(TypeID type)
	{
		return GetItemsForType(type, true, true, false);
	}

	void GenericEditor::SpawnProperty(LayoutElementsContainer* itemLayout,
	                                  ValueContainer* itemValues,
	                                  ItemInfo& item)
	{
		if (!itemLayout || !itemValues || !item.info)
			return;

		int labelIndex = GetLabelIndex(itemLayout, item);

		// Get override editor if specified
		CustomEditor* overrideEditor = item.GetOverrideEditor();

		// Create property editor
		itemLayout->Property(item.displayName, itemValues, overrideEditor, item.tooltipText);

		// TODO: Set read-only state when supported
		/*
		if (item.IsReadOnly && editor)
		{
			editor->SetReadOnly(true);
		}
		*/

		EvaluateVisibleIf(itemLayout, item, labelIndex);
	}

	void GenericEditor::EvaluateVisibleIf(LayoutElementsContainer* itemLayout,
	                                      const ItemInfo& item,
	                                      int labelIndex)
	{
		if (item.visibleIfs.Count() > 0 && itemLayout->Children.Count() > 0)
		{
			PropertiesListElement* list = nullptr;
			GroupElement* group = nullptr;

			PropertiesListElement* list1;
			GroupElement* group1;
			if (TypeTryCast(itemLayout->Children[itemLayout->Children.Count() - 1], list1))
			{
				list = list1;
			}
			else if (TypeTryCast(itemLayout->Children[itemLayout->Children.Count() - 1], group1))
			{
				group = group1;
			}
			else
			{
				return;
			}

			// Get source member used to check rule
			/*var sourceMembers = GetVisibleIfSources(item.Info.DeclaringType, item.VisibleIfs);
			if (sourceMembers.Length == 0)
				return;

			// Resize cache
			if (_visibleIfCaches == null)
				_visibleIfCaches = new VisibleIfCache[8];
			int count = 0;
			while (count < _visibleIfCaches.Length && _visibleIfCaches[count].Target != ScriptType.Null)
				count++;
			if (count >= _visibleIfCaches.Length)
				Array.Resize(ref _visibleIfCaches, count * 2);

			// Add item
			_visibleIfCaches[count] = new VisibleIfCache
			{
				Target = item.Info,
				Sources = sourceMembers,
				PropertiesList = list,
				Group = group,
				LabelIndex = labelIndex,
				InversionList = item.VisibleIfs.Select((x, i) => x.Invert).ToArray(),
			};*/
		}
	}

	List<GenericEditor::ItemInfo> GenericEditor::GetItemsForType(TypeID type,
	                                                             bool useProperties,
	                                                             bool useFields,
	                                                             bool usePropertiesWithoutSetter)
	{
		List<ItemInfo> items;

		const TypeCompositeInfo* typeInfo = Types::GetTypeInfo(type);
		if (!typeInfo)
		{
			LOG_WARNING("Editor", "GenericEditor::GetItemsForType - Type info not found for type: {}", type);
			return items;
		}

		// Collect properties
		if (useProperties)
		{
			const List<TypeProperty*>& properties = typeInfo->properties;
			for (const TypeProperty* prop : properties)
			{
				if (!prop)
					continue;

				// Check if property has metadata
				if (!prop->metaContainer)
					continue;

				// Check for HideInEditor attribute
				if (prop->metaContainer->Has<HideInEditorAttribute>())
					continue;

				// Check if property has setter (unless we allow read-only)
				// TODO: Implement when TypeProperty has setter information
				/*
				if (!usePropertiesWithoutSetter && !prop->HasSetter())
					continue;
				*/

				// Add to items
				items.Add(ItemInfo(prop));
			}
		}

		// TODO: Collect fields when TypeCompositeInfo supports fields
		/*
		if (useFields)
		{
			const List<TypeField*>& fields = typeInfo->GetFields();
			for (const TypeField* field : fields)
			{
				// Similar logic as properties
			}
		}
		*/

		return items;
	}

	int GenericEditor::GetLabelIndex(LayoutElementsContainer* itemLayout, ItemInfo& item)
	{
		int labelIndex = 0;
		PropertiesListElement* propertiesListElement = nullptr;
		if (itemLayout->Children.Count() > 0 && TypeTryCast<PropertiesListElement>(itemLayout->Children[itemLayout->Children.Count() - 1], propertiesListElement))
		{
			labelIndex = propertiesListElement->Labels.Count();
		}

		return labelIndex;
	}

	// ============================================================================
	// VisibleIfCache Implementation
	// ============================================================================

	bool GenericEditor::VisibleIfCache::GetValue(ValueContent& value) const
	{
		if (!value.instance || Sources.Count() == 0)
			return true;

		// Evaluate all conditions with AND logic
		for (int i = 0; i < Sources.Count(); i++)
		{
			const TypeProperty* sourceProp = Sources[i];
			if (!sourceProp)
				continue;

			// Get property value
			void* propValue = sourceProp->GetPropertyAddress(value.instance);
			if (!propValue)
				return false;

			// Evaluate condition (assume boolean for now)
			// TODO: Support different value types
			bool conditionValue = false;
			if (sourceProp->typeID == Typeof<bool>())
			{
				conditionValue = *static_cast<bool*>(propValue);
			}

			// Apply inversion if needed
			if (i < InversionList.Count() && InversionList[i])
			{
				conditionValue = !conditionValue;
			}

			// If any condition is false, property should be hidden
			if (!conditionValue)
				return false;
		}

		return true;
	}

} // namespace SE::Editor
