
#include "ContentNavigationButton.h"

#include "Editor/EditorIcons.h"
#include "Editor/GUI/ContextMenu/ContextMenuButton.h"
#include "Editor/GUI/Drag/DragItems.hpp"
#include "Editor/Modules/WindowsModule.h"
#include "Editor/Windows/ContentWindow.h"
#include "Tree/ContentTreeNode.h"
#include "Items/ContentItem.h"
#include "Items/ContentFolder.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/Style.h"

namespace SE::Editor
{
	ContentNavigationButton::ContentNavigationButton() : NavigationButton(0, 0, 0)
	{
		TargetNode = nullptr;
		Text = String::Empty;
	}

	ContentNavigationButton::ContentNavigationButton(ContentTreeNode* targetNode, float x, float y, float height) : NavigationButton(x, y, height)
	{
		TargetNode = targetNode;
		Text = targetNode->GetNavButtonLabel();
	}

	DragDropEffect ContentNavigationButton::OnDragEnter(const Float2& location, DragData* data)
	{
		NavigationButton::OnDragEnter(location, data);

		if (_dragOverItems == nullptr)
			_dragOverItems = New<DragItems>(CreateFunc<ContentNavigationButton, &ContentNavigationButton::ValidateDragItem>(this));
		_dragOverItems->OnDragEnter(data);
		DragDropEffect result = GetDragEffect(data);
		m_ValidDragOver = result != DragDropEffect::None;
		return result;
	}

	DragDropEffect ContentNavigationButton::OnDragMove(const Float2& location, DragData* data)
	{
		NavigationButton::OnDragMove(location, data);

		return GetDragEffect(data);
	}

	void ContentNavigationButton::OnDragLeave()
	{
		NavigationButton::OnDragLeave();

		_dragOverItems->OnDragLeave();
		m_ValidDragOver = false;
	}

	DragDropEffect ContentNavigationButton::OnDragDrop(const Float2& location, DragData* data)
	{
		DragDropEffect result = DragDropEffect::None;
		NavigationButton::OnDragDrop(location, data);

		// Check if drop element or files
		DragDataFiles* files;
		if (TypeTryCast<DragDataFiles>(data, files))
		{
			// Import files
			ENGINE_UNREACHABLE_CODE()
			// Editor.Instance.ContentImporting.Import(files->Files, TargetNode->GetFolder());
			result = DragDropEffect::Copy;
		}
		else if (_dragOverItems->GetHasValidDrag())
		{
			// Move items
			EditorApp::Ins().databaseModule->Move(_dragOverItems->Objects, TargetNode->GetFolder());
			result = DragDropEffect::Move;
		}

		_dragOverItems->OnDragDrop();
		m_ValidDragOver = false;

		return result;
	}

	void ContentNavigationButton::OnClick()
	{
		// Navigate
		EditorApp::Ins().windowsModule->ContentWin->Navigate(TargetNode);

		NavigationButton::OnClick();
	}

	DragDropEffect ContentNavigationButton::GetDragEffect(DragData* data)
	{
		if (TypeIs<DragDataFiles>(data))
		{
			if (TargetNode->GetCanHaveAssets())
				return DragDropEffect::Copy;
		}
		else
		{
			if (_dragOverItems->GetHasValidDrag())
				return DragDropEffect::Move;
		}

		return DragDropEffect::None;
	}

	bool ContentNavigationButton::ValidateDragItem(ContentItem* item)
	{
		// Reject itself and any parent
		return item != TargetNode->GetFolder() && !item->Find(TargetNode->GetFolder()) && !TargetNode->IsRoot;
	}

	ContentNavigationSeparator::ContentNavigationSeparator(ContentNavigationButton* target, float x, float y, float height)
	{
		Target = target;
		Bounds = Rectangle(x, y, 16, height);
		Offsets = Margin(m_Bounds.GetX(), m_Bounds.GetWidth(), m_Bounds.GetY(), m_Bounds.GetHeight());
		UpdateTransform();

		MaximumItemsInViewCount = 20;
		Style* style = Style::Current;
		BackgroundColor = style->BackgroundNormal;
		BackgroundColorHighlighted = BackgroundColor;
		BackgroundColorSelected = BackgroundColor;
	}

	void ContentNavigationSeparator::Draw()
	{
		Style* style = Style::Current;
		Rectangle rect = Rectangle(Float2::Zero, Size);
		Color color = IsDragOver() ? Colors::Transparent : (_mouseDown ? style->BackgroundSelected : (IsMouseOver ? style->BackgroundHighlighted : Colors::Transparent));
		Render2D::FillRectangle(rect, color);
		Render2D::DrawSprite(EditorIcons::ArrowRight12, Rectangle(rect.Location.x, rect.GetY() + rect.Size.y * 0.25f, rect.Size.x, rect.Size.x), EnabledInHierarchy() ? style->Foreground : style->ForegroundDisabled);
	}

	ContextMenu* ContentNavigationSeparator::OnCreatePopup()
	{
		// Update items
		ClearItems();
		for (Control* child : Target->TargetNode->Children())
		{
			ContentTreeNode* node;
			if (TypeTryCast<ContentTreeNode>(child, node))
			{
				if (node->GetFolder()->VisibleInHierarchy()) // Respect the filter set by ContentFilterConfig.Filter(...)
					AddItem(node->GetFolder()->ShortName);
			}
		}

		return ComboBox::OnCreatePopup();
	}

	void ContentNavigationSeparator::OnLayoutMenuButton(ContextMenuButton* button, int index, bool construct)
	{
		button->Icon = EditorIcons::FolderClosed32;
		if (tooltips.Count() > index)
			button->TooltipText = tooltips[index];
	}

	void ContentNavigationSeparator::OnItemClicked(int index)
	{
		ComboBox::OnItemClicked(index);

		String& item = items[index];
		for (Control* child : Target->TargetNode->Children())
		{
			ContentTreeNode* node;
			if (TypeTryCast<ContentTreeNode>(child, node) && node->GetFolder()->ShortName == item)
			{
				EditorApp::Ins().windowsModule->ContentWin->Navigate(node);
				return;
			}
		}
	}
} // SE