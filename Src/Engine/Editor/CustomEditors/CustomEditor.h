#pragma once
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/TypeSystem/IType.h"
#include "Runtime/Utilities/Variant.h"

namespace SE::Editor
{
	class LayoutElementsContainer;
	class ValueContainer;
	class CustomEditorPresenter;
	class PropertyNameLabel;

	/// <summary>
	/// Custom editor layout style modes.
	/// </summary>
	enum class DisplayStyle
	{
		/// <summary>
		/// Creates a separate group for the editor (drop down element). This is a default value.
		/// </summary>
		Group,

		/// <summary>
		/// Inlines editor contents into the property area without creating a drop-down group.
		/// </summary>
		Inline,

		/// <summary>
		/// Inlines editor contents into the parent editor layout. Won't use property with label name.
		/// </summary>
		InlineIntoParent,
	};

	SE_CLASS(Reflect)
	class CustomEditor : public IType
	{
		SE_DEFINE_CLASS_DEFAULT(CustomEditor, IType)
	public:
		~CustomEditor() override = default;

		/// <summary>
		/// True if Editor is during value setting (eg. by user or from copy/paste).
		/// </summary>
		static bool IsSettingValue;

		/// <summary>
		/// Current custom editor being initialized.
		/// </summary>
		static CustomEditor* CurrentCustomEditor;

		/// <summary>
		/// Linked property name label.
		/// </summary>
		PropertyNameLabel* LinkedLabel = nullptr;

		// Lifecycle methods
		void Initialize(CustomEditorPresenter* presenter, LayoutElementsContainer* layout, ValueContainer* values);
		virtual void Deinitialize();
		void Cleanup();

		// Refresh and update
		virtual void Refresh();
		virtual void RefreshInternal();
		void RefreshRoot();

		// Layout management
		void RebuildLayout();
		void RebuildLayoutOnRefresh();
		void SkipChildrenRefresh();

		// Value operations
		void RevertToDefaultValue();
		void RevertToReferenceValue();
		void SetValueToDefault();
		void SetValueToReference();

		// Copy/paste
		void Copy();
		void Paste();
		bool CanPaste() const;

		// Property accessors
		DisplayStyle GetStyle() const { return m_style; }
		bool IsSingleObject() const;
		bool HasDifferentValues() const;
		bool HasDifferentTypes() const;
		bool CanRevertDefaultValue() const;
		bool CanRevertReferenceValue() const;

		ValueContainer* GetValues() { return m_values; }
		CustomEditor* GetParentEditor() { return m_parent; }
		CustomEditorPresenter* GetPresenter() { return m_presenter; }
		const List<CustomEditor*>& GetChildrenEditors() const { return m_children; }

	protected:
		/// <summary>
		/// Initializes this editor.
		/// </summary>
		/// <param name="layout">The layout builder.</param>
		virtual void OnInitialize(LayoutElementsContainer* layout) = 0;

		// Subclass implementations
		virtual void SetValue(Variant value, void* token = nullptr);
		virtual bool OnDirty(CustomEditor* editor, Variant value, void* token = nullptr);
		virtual void OnUnDirty();
		virtual void ClearToken();
		virtual void SynchronizeValue(void* value);

		bool IsSetBlocked() const { return m_isSetBlocked; }

	private:
		friend class CustomEditorPresenter;
		friend class LayoutElementsContainer;

		void InitializeInternal(CustomEditorPresenter* presenter,
		                       LayoutElementsContainer* layout,
		                       ValueContainer* values);
		void OnChildCreated(CustomEditor* child);

		DisplayStyle m_style = DisplayStyle::Group;
		LayoutElementsContainer* m_layout = nullptr;
		CustomEditorPresenter* m_presenter = nullptr;
		CustomEditor* m_parent = nullptr;
		List<CustomEditor*> m_children;
		ValueContainer* m_values = nullptr;

		bool m_isSetBlocked = false;
		bool m_skipChildrenRefresh = false;
		bool m_hasValueDirty = false;
		bool m_rebuildOnRefresh = false;
		Variant m_valueToSet;
	};

} // namespace SE::Editor
