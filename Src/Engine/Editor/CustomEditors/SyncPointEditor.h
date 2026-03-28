#pragma once
#include "CustomEditor.h"

namespace SE::Editor
{
	class SyncPointEditor : public CustomEditor
	{
		friend class CustomEditorPresenter;
	protected:
		/// <summary>
		/// The 'is dirty' flag.
		/// </summary>
		bool _isDirty;

		/*private object[] _snapshotUndoInternal;

		/// <summary>
		/// The cached token used by the value setter to support batching Undo actions (eg. for sliders or color pickers).
		/// </summary>
		protected object _setValueToken;

		/// <summary>
		/// Gets the undo objects used to record undo operation changes.
		/// </summary>
		public virtual IEnumerable<object> UndoObjects => Presenter.GetUndoObjects(Presenter);

		/// <summary>
		/// Gets the undo.
		/// </summary>
		public virtual Undo Undo => Presenter.Undo;*/

	public:
		/// <inheritdoc />
		void Refresh() override
		{
			CustomEditor::Refresh();

			RefreshRoot();
		}

	protected:
		/// <inheritdoc />
		void OnInitialize(LayoutElementsContainer* layout) override
		{
			_isDirty = false;
		}

		/// <inheritdoc />
		void Deinitialize() override
		{
			// EndUndoRecord();

			CustomEditor::Deinitialize();
		}

		void RefreshInternal() override;

		/// <summary>
		/// Called when data gets modified by the custom editors.
		/// </summary>
		virtual void OnModified()
		{
			/*var parent = ParentEditor;
			while (parent != null)
			{
				if (parent is SyncPointEditor syncPointEditor)
				{
					syncPointEditor.OnModified();
					break;
				}
				parent = parent.ParentEditor;
			}*/
		}

		/// <inheritdoc />
		/*bool OnDirty(CustomEditor editor, object value, object token = null) override
		{
			// End any active Undo action batching
			if (token != _setValueToken)
				EndUndoRecord();
			_setValueToken = token;

			// Mark as modified and don't pass event further to the higher editors (don't call parent)
			_isDirty = true;
			return true;
		}*/

		/// <inheritdoc />
		/*void ClearToken() override
		{
			EndUndoRecord();
		}*/

		/// <summary>
		/// Ends the undo recording if started with custom token (eg. by value slider).
		/// </summary>
		/*void EndUndoRecord()
		{
			if (_snapshotUndoInternal != null)
			{
				Undo.RecordMultiEnd(_snapshotUndoInternal);
				_snapshotUndoInternal = null;
			}

			// Clear token
			_setValueToken = null;
		}*/
	};
} // SE