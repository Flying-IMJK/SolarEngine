#pragma once
#include "Editor/CustomEditors/CustomEditor.h"
#include "Core/Types/Collections/List.h"
#include "Core/TypeSystem/TypeID.h"
#include "Editor/CustomEditors/Values/ValueContainer.h"

namespace SE
{
	class TypeProperty;
	class TypeMetaAttribute;
	class EditorOrderAttribute;
	class EditorDisplayAttribute;
	class CustomEditorAttribute;
	class CustomEditorAliasAttribute;
	class SpaceAttribute;
	class HeaderAttribute;
	class VisibleIfAttribute;
}

namespace SE::Editor
{
	class LayoutElementsContainer;
	class ValueContainer;
	class GroupElement;
	class PropertiesListElement;

	/// <summary>
	/// Generic editor for objects without custom editors.
	/// Uses reflection to automatically generate property editors.
	/// </summary>
	class GenericEditor : public CustomEditor
	{
		SE_CLASS(GenericEditor, CustomEditor)

	public:
		/// <summary>
		/// Information about a single property item.
		/// </summary>
		struct ItemInfo
		{
			const TypeProperty* info = nullptr;
			const EditorOrderAttribute* order = nullptr;
			const EditorDisplayAttribute* display = nullptr;
			const CustomEditorAttribute* customEditor = nullptr;
			const CustomEditorAliasAttribute* customEditorAlias = nullptr;
			const SpaceAttribute* space = nullptr;
			const HeaderAttribute* header = nullptr;
			List<const VisibleIfAttribute*> visibleIfs;
			bool isReadOnly = false;
			bool expandGroups = false;
			String displayName = String::Empty;
			String tooltipText = String::Empty;

			ItemInfo() = default;

			/// <summary>
			/// Constructs ItemInfo from a TypeProperty.
			/// </summary>
			/// <param name="info">The property information.</param>
			ItemInfo(const TypeProperty* info);

			/// <summary>
			/// Gets the value container for this property.
			/// </summary>
			/// <param name="instanceValues">The instance values container.</param>
			/// <returns>The property value container.</returns>
			ValueContainer* GetValues(ValueContainer* instanceValues);

			/// <summary>
			/// Gets the override editor for this property based on attributes.
			/// </summary>
			/// <returns>The custom editor, or nullptr if none specified.</returns>
			CustomEditor* GetOverrideEditor();

			/// <summary>
			/// Compares this item to another for sorting.
			/// </summary>
			/// <param name="other">The other item.</param>
			/// <returns>Comparison result.</returns>
			int CompareTo(const ItemInfo& other) const;

			/// <summary>
			/// Checks if two items can be merged (have same name and type).
			/// </summary>
			/// <param name="a">First item.</param>
			/// <param name="b">Second item.</param>
			/// <returns>True if items can be merged.</returns>
			static bool CanMerge(const ItemInfo& a, const ItemInfo& b);
		};

		GenericEditor() = default;
		~GenericEditor() override = default;

		// CustomEditor overrides
		void Refresh() override;

	protected:
		void OnInitialize(LayoutElementsContainer* layout) override;

		/// <summary>
		/// Gets the list of editable items for a type.
		/// </summary>
		/// <param name="type">The type to get items for.</param>
		/// <returns>List of item information.</returns>
		virtual List<ItemInfo> GetItemsForType(TypeID type);

		/// <summary>
		/// Spawns a property editor for a single item.
		/// </summary>
		/// <param name="itemLayout">The layout container.</param>
		/// <param name="itemValues">The property values.</param>
		/// <param name="item">The item information.</param>
		virtual void SpawnProperty(LayoutElementsContainer* itemLayout,
		                          ValueContainer* itemValues,
		                          ItemInfo& item);

		/// <summary>
		/// Evaluates VisibleIf conditions for an item.
		/// </summary>
		/// <param name="itemLayout">The layout container.</param>
		/// <param name="item">The item information.</param>
		/// <param name="labelIndex">The label index in the properties list.</param>
		virtual void EvaluateVisibleIf(LayoutElementsContainer* itemLayout,
		                               const ItemInfo& item,
		                               int labelIndex);

		/// <summary>
		/// Gets items for a type with specific options.
		/// </summary>
		/// <param name="type">The type.</param>
		/// <param name="useProperties">Include properties.</param>
		/// <param name="useFields">Include fields.</param>
		/// <param name="usePropertiesWithoutSetter">Include read-only properties.</param>
		/// <returns>List of item information.</returns>
		static List<ItemInfo> GetItemsForType(TypeID type,
		                                      bool useProperties,
		                                      bool useFields,
		                                      bool usePropertiesWithoutSetter = false);

		virtual int GetLabelIndex(LayoutElementsContainer* itemLayout, ItemInfo& item);

	private:
		/// <summary>
		/// Cache for VisibleIf condition evaluation.
		/// </summary>
		struct VisibleIfCache
		{
			const TypeProperty* Target = nullptr;
			List<const TypeProperty*> Sources;
			PropertiesListElement* PropertiesList = nullptr;
			GroupElement* Group = nullptr;
			List<bool> InversionList;
			int LabelIndex = -1;

			/// <summary>
			/// Evaluates the visibility condition for an instance.
			/// </summary>
			/// <param name="instance">The object instance.</param>
			/// <returns>True if the property should be visible.</returns>
			bool GetValue(ValueContent& value) const;
		};

		List<VisibleIfCache> m_visibleIfCaches;
		bool m_isNull = false;
	};

} // namespace SE::Editor

