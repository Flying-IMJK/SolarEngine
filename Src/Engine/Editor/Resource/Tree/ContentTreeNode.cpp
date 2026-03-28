
#include "ContentTreeNode.h"

#include "Editor/EditorIcons.h"
#include "Editor/GUI/Drag/DragItems.hpp"
#include "Editor/Resource/Items/ContentFolder.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/Style.h"
#include "Runtime/UI/GUI/WindowRootControl.h"

namespace SE::Editor
{
	ContentFolderType ContentTreeNode::GetFolderType()
	{
		return m_Folder->FolderType;
	}
	
	bool ContentTreeNode::GetCanHaveAssets()
	{
		return m_Folder->CanHaveAssets;
	}
	
	ContentTreeNode* ContentTreeNode::ParentNode()
	{
		return TypeTryCast<ContentTreeNode>(Parent);
	}

	StringView ContentTreeNode::GetPath()
	{
		return m_Folder->Path;
	}

	StringView ContentTreeNode::GetNavButtonLabel()
	{
		return m_Folder->ShortName;
	}

	ContentTreeNode::ContentTreeNode(ContentTreeNode* parent, StringView path) : ContentTreeNode(parent, parent != nullptr ? parent->GetFolderType() : ContentFolderType::Other, path)
	{
	}

	void ContentTreeNode::StartRenaming()
	{
		if (!m_Folder->CanRename)
			return;

		// Start renaming the folder
		/*Editor.Instance.Windows.ContentWin.ScrollingOnTreeView(false);
		var dialog = RenamePopup.Show(this, TextRect, _folder.ShortName, false);
		dialog.Tag = _folder;
		dialog.Renamed += popup =>
		{
			Editor.Instance.Windows.ContentWin.Rename((ContentFolder)popup.Tag, popup.Text);
			Editor.Instance.Windows.ContentWin.ScrollingOnTreeView(true);
		};
		dialog.Closed += popup => { Editor.Instance.Windows.ContentWin.ScrollingOnTreeView(true); };*/
	}
	
	void ContentTreeNode::UpdateFilter(String filterText)
	{
		bool noFilter = filterText.Length() <= 0;

		// Update itself
		bool isThisVisible;
		if (noFilter)
		{
			// Clear filter
			_highlights.Clear();
			isThisVisible = true;
		}
		else
		{
			ENGINE_UNREACHABLE_CODE()
			/*StringView text{ Text };
			if (QueryFilterHelper.Match(filterText, text, out QueryFilterHelper.Range[] ranges))
			{
				// Update highlights
				if (_highlights == null)
					_highlights = new List<Rectangle>(ranges.Length);
				else
					_highlights.Clear();
				var style = Style.Current;
				var font = style.FontSmall;
				var textRect = TextRect;
				for (int i = 0; i < ranges.Length; i++)
				{
					var start = font.GetCharPosition(text, ranges[i].StartIndex);
					var end = font.GetCharPosition(text, ranges[i].EndIndex);
					_highlights.Add(new Rectangle(start.X + textRect.X, textRect.Y, end.X - start.X, textRect.Height));
				}
				isThisVisible = true;
			}
			else
			{
				// Hide
				_highlights.Clear();
				isThisVisible = false;
			}*/
		}

		// Update children
		bool isAnyChildVisible = false;
		for (int i = 0; i < m_Children.Count(); i++)
		{
			ContentTreeNode* child;
			if (TypeTryCast<ContentTreeNode>(m_Children[i], child))
			{
				child->UpdateFilter(filterText);
				isAnyChildVisible |= child->Visible;
			}
		}

		bool isExpanded = isAnyChildVisible;

		if (isExpanded)
		{
			Expand(true);
		}
		else
		{
			Collapse(true);
		}

		Visible = isThisVisible | isAnyChildVisible;
	}
	
	void ContentTreeNode::Draw()
	{
		TreeNode::Draw();

		// Draw all highlights
		if (_highlights.Count() > 0)
		{
			Style* style = Style::Current;
			Color color = style->ProgressNormal * 0.6f;
			for (int i = 0; i < _highlights.Count(); i++)
			{
				Render2D::FillRectangle(_highlights[i], color);
			}
		}
	}
	
	void ContentTreeNode::OnDestroy()
	{
		// Delete folder item
		m_Folder->Dispose();
		Delete(m_Folder);
		m_Folder = nullptr;
		TreeNode::OnDestroy();
	}
	
	bool ContentTreeNode::OnKeyDown(KeyboardKeys key)
	{
		if (IsFocused)
		{
			switch (key)
			{
			case KeyboardKeys::F2:
				StartRenaming();
				return true;
			case KeyboardKeys::Delete:
				if (GetFolder()->Exists && GetCanDelete())
				{
					// Editor.Instance.Windows.ContentWin.Delete(GetFolder());
					ENGINE_UNREACHABLE_CODE();
				}
				return true;
			}
			if (RootWindow()->GetKey(KeyboardKeys::Control))
			{
				switch (key)
				{
				case KeyboardKeys::D:
					if (GetFolder()->Exists && GetCanDuplicate())
					{
						// Editor.Instance.Windows.ContentWin.Duplicate(Folder);
						ENGINE_UNREACHABLE_CODE();
					}
					return true;
				}
			}
		}

		return TreeNode::OnKeyDown(key);
	}

	DragDropEffect ContentTreeNode::GetDragEffect(DragData* data)
	{
		if (TypeIs<DragDataFiles>(data))
		{
			if (m_Folder->CanHaveAssets)
				return DragDropEffect::Copy;
		}
		else
		{
			if (_dragOverItems->GetHasValidDrag())
			{
				return DragDropEffect::Move;
			}
		}

		return DragDropEffect::None;
	}

	bool ContentTreeNode::ValidateDragItem(ContentItem* item)
	{
		// Reject itself and any parent
		return item != m_Folder && !item->Find(m_Folder);
	}

	DragDropEffect ContentTreeNode::OnDragEnterHeader(DragData* data)
	{
		if (_dragOverItems == nullptr)
			_dragOverItems = MakeRef<DragItems>(CreateFunc<ContentTreeNode, &ContentTreeNode::ValidateDragItem>(this));

		_dragOverItems->OnDragEnter(data);
		return GetDragEffect(data);
	}
	
	DragDropEffect ContentTreeNode::OnDragMoveHeader(DragData* data)
	{
		return GetDragEffect(data);
	}
	
	void ContentTreeNode::OnDragLeaveHeader()
	{
		_dragOverItems->OnDragLeave();
		TreeNode::OnDragLeaveHeader();
	}
	
	DragDropEffect ContentTreeNode::OnDragDropHeader(DragData* data)
	{
		DragDropEffect result = DragDropEffect::None;

		// Check if drop element or files
		DragDataFiles* files;
		if (TypeTryCast<DragDataFiles>(data, files))
		{
			// Import files
			// Editor.Instance.ContentImporting.Import(files->Files, _folder);
			ENGINE_UNREACHABLE_CODE();
			result = DragDropEffect::Copy;

			Expand();
		}
		else if (_dragOverItems->GetHasValidDrag())
		{
			// Move items
			EditorApp::Ins().databaseModule->Move(_dragOverItems->Objects, m_Folder);
			result = DragDropEffect::Move;

			Expand();
		}

		_dragOverItems->OnDragDrop();

		return result;
	}
	
	void ContentTreeNode::BeginDragDrop()
	{
		DragData* data = DragItems::GetDragData(m_Folder);
		DoDragDrop(data);
	}
	
	void ContentTreeNode::OnLongPress()
	{
		Select();

		StartRenaming();
	}

	ContentTreeNode::ContentTreeNode(ContentTreeNode* parent, ContentFolderType type, StringView path)
		: TreeNode(false, EditorIcons::FolderClosed32, EditorIcons::FolderOpen32)
	{
		m_Folder = New<ContentFolder>(type, path, this);
		Text = m_Folder->ShortName;
		if (parent != nullptr)
		{
			GetFolder()->ParentFolder = parent->GetFolder();
			Parent = parent;
		}
		IconColor = Style::Current->Foreground;
	}

	RootContentTreeNode::RootContentTreeNode() : ContentTreeNode(nullptr, String::Empty)
	{
	}

	StringView RootContentTreeNode::GetNavButtonLabel()
	{
		return SE_TEXT(" /");
	}
} // SE