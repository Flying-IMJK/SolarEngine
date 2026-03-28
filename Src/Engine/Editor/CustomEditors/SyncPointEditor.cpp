

#include "SyncPointEditor.h"

namespace SE::Editor
{
	void SyncPointEditor::RefreshInternal()
	{
		/*bool isDirty = _isDirty;
		_isDirty = false;

		// If any UI control has been modified we should try to record selected objects change
		if (isDirty && Undo != null && Undo.Enabled)
		{
			string actionName = "Edit object(s)";

			// Check if use token
			if (_setValueToken != null)
			{
				// Check if record start
				if (_snapshotUndoInternal == null)
				{
					// Record undo action start only (end called in EndUndoRecord)
					_snapshotUndoInternal = UndoObjects.ToArray();
					Undo.RecordMultiBegin(_snapshotUndoInternal, actionName);
				}

				Refresh();
			}
			else
			{
				// Normal undo action recording
				using (new UndoMultiBlock(Undo, UndoObjects, actionName))
					Refresh();
			}
		}
		else
		{
			Refresh();
		}

		if (isDirty)
			OnModified();*/
	}
} // SE