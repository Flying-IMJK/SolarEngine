#pragma once

#include "LayoutElementsContainer.h"
#include "SyncPointEditor.h"
#include "Runtime/UI/GUI/Panels/VerticalPanel.h"

namespace SE
{
	class Object;
}

namespace SE::Editor
{
	class EditorViewport;
	class ScenesGraphNode;

	/// <summary>
	/// Feature flags for the custom editor presenter.
	/// </summary>
	enum class FeatureFlags : uint32
	{
		None = 0,
		CacheExpandedGroups = 1 << 0,  // Cache expanded group states
		UsePrefab = 1 << 1,            // Enable prefab-related features
		UseDefault = 1 << 2             // Enable default value features
	};

	/// <summary>
	/// The interface for Editor context that owns the presenter. or other window/panel - custom editor scan use it for more specific features.
	/// </summary>
	class IPresenterOwner
	{
	public:
		virtual ~IPresenterOwner() = default;

		/// <summary>
		/// Gets the viewport linked with properties presenter (optional, null if unused).
		/// </summary>
		virtual EditorViewport* GetPresenterViewport() = 0;

		/// <summary>
		/// Selects the scene objects.
		/// </summary>
		/// <param name="nodes">The nodes to select</param>
		virtual void Select(List<ScenesGraphNode*> nodes) = 0;
	};

	class CustomEditorPresenter : public LayoutElementsContainer
	{
		SE_DEFINE_CLASS(CustomEditorPresenter, LayoutElementsContainer)
	public:
		/// <summary>
		/// The panel control.
		/// </summary>
		class PresenterPanel : public VerticalPanel
		{
		private:
			CustomEditorPresenter* _presenter;

		public:
			/// <summary>
			/// Gets the presenter.
			/// </summary>
			PRO_GET(Presenter, PresenterPanel, CustomEditorPresenter*, __GetPresenter);

			PresenterPanel(CustomEditorPresenter* presenter);

			/// <inheritdoc />
			void Update(float deltaTime) override;

			/// <inheritdoc />
			void OnDestroy() override;

		private:
			CustomEditorPresenter* __GetPresenter();
		};


        /// <summary>
        /// The root editor. Mocks some custom editors events. Created a child editor for the selected objects.
        /// </summary>
        class RootEditor final : public SyncPointEditor
        {
        private:
        	CustomEditor* _overrideEditor = nullptr;

        public:
            /// <summary>
            /// The selected objects editor.
            /// </summary>
        	CustomEditor* Editor = nullptr;

            /// <summary>
            /// The text to show when no object is selected.
            /// </summary>
        	String NoSelectionText = String::Empty;

            /// <summary>
            /// Initializes a new instance of the <see cref="RootEditor"/> class.
            /// </summary>
            /// <param name="noSelectionText">The text to show when no item is selected.</param>
        	RootEditor(StringView noSelectionText);

            /// <summary>
            /// Setups editor for selected objects.
            /// </summary>
            /// <param name="presenter">The presenter.</param>
        	void Setup(CustomEditorPresenter* presenter);

        	CustomEditor* GetOverrideEditor();
        	void SetOverrideEditor(CustomEditor* value);

        protected:
            /// <inheritdoc />
        	void OnModified() override;

        	/// <inheritdoc />
        	void OnInitialize(LayoutElementsContainer* layout) override;
        };

	private:
		bool _buildOnUpdate = false;
		bool _readOnly = false;
		bool _batchUpdateMode = false;
		String _noSelectionText = String::Empty;

	public:

		ValueContainer* Selection = nullptr;

		/// <summary>
		/// The panel.
		/// </summary>
		PresenterPanel* Panel = nullptr;

		/// <summary>
		/// The Editor context that owns this presenter. or other window/panel - custom editor scan use it for more specific features.
		/// </summary>
		IPresenterOwner* Owner = nullptr;

		/// <summary>
		/// Feature flags for the presenter.
		/// </summary>
		FeatureFlags Features = static_cast<FeatureFlags>(
			static_cast<uint32>(FeatureFlags::UsePrefab) | 
			static_cast<uint32>(FeatureFlags::UseDefault)
		);

		/// <summary>
		/// Occurs when selection gets changed.
		/// </summary>
		Action SelectionChanged;

		/// <summary>
		/// Occurs when any property gets changed.
		/// </summary>
		Action Modified;

		/// <summary>
		/// Occurs when before creating layout for the selected objects editor UI. Can be used to inject custom UI to the layout.
		/// </summary>
		Delegate<LayoutElementsContainer*> BeforeLayout;

		/// <summary>
		/// Occurs when after creating layout for the selected objects editor UI. Can be used to inject custom UI to the layout.
		/// </summary>
		Delegate<LayoutElementsContainer*> AfterLayout;

		/// <summary>
		/// Initializes a new instance of the <see cref="CustomEditorPresenter"/> class.
		/// </summary>
		/// <param name="undo">The undo. It's optional.</param>
		/// <param name="noSelectionText">The custom text to display when no object is selected. Default is No selection.</param>
		/// <param name="owner">The owner of the presenter.</param>
		CustomEditorPresenter(/*Undo undo, */StringView noSelectionText = StringView::Empty, IPresenterOwner* owner = nullptr);

		~CustomEditorPresenter() override;

		/// <summary>
		/// Gets the selection count.
		/// </summary>
		int GetSelectionCount() const;

		/// <summary>
		/// Gets the root editor.
		/// </summary>
		CustomEditor* GetRoot() { return Editor; }

		/// <summary>
		/// Gets the override editor.
		/// </summary>
		CustomEditor* GetOverrideEditor() const;

		/// <summary>
		/// Sets the override editor.
		/// </summary>
		void SetOverrideEditor(CustomEditor* editor);

		/// <summary>
		/// Gets whether the presenter is in read-only mode.
		/// </summary>
		bool IsReadOnly() const { return _readOnly; }

		/// <summary>
		/// Sets the read-only mode.
		/// </summary>
		void SetReadOnly(bool readOnly);

		Control* GetControl() override;

		ContainerControl* GetContainerControl() override;

		/// <summary>
		/// Gets the no selection text.
		/// </summary>
		String GetNoSelectionText() const { return _noSelectionText; }

		/// <summary>
		/// Sets the no selection text.
		/// </summary>
		void SetNoSelectionText(const String& text);

		/// <summary>
		/// Deselects all objects.
		/// </summary>
		void Deselect();

		/// <summary>
		/// Opens all groups in the editor.
		/// </summary>
		void OpenAllGroups();

		/// <summary>
		/// Closes all groups in the editor.
		/// </summary>
		void CloseAllGroups();

		/// <summary>
		/// Builds the layout on the next update.
		/// </summary>
		void BuildLayoutOnUpdate();

		/// <summary>
		/// Begins batch update mode (disables refresh).
		/// </summary>
		void BeginBatchUpdate() { _batchUpdateMode = true; }

		/// <summary>
		/// Ends batch update mode (enables refresh).
		/// </summary>
		void EndBatchUpdate() { _batchUpdateMode = false; }

		void OnModified()
		{
			if (Modified.IsBinded())
			{
				Modified();
			}
		}

		virtual void BuildLayout();

		virtual void ClearLayout();

		void Select(::SE::Object* obj);
		void Select(const Span<::SE::Object*>& objs);

	protected:
		/// <summary>
		/// The selected objects editor (root, it generates actual editor for selection).
		/// </summary>
		RootEditor* Editor = nullptr;

		void Update();

		virtual void OnSelectionChanged();

		void UpdateReadOnly();
	};

} // SE
