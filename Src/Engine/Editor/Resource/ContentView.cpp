
#include "ContentView.h"

#include "Core/Input/Input.h"
#include "Core/Types/Collections/Sorting.h"
#include "Core/Platform/Clipboard.h"
#include "Core/Types/Collections/ListExtensions.h"
#include "Editor/EditorApp.h"
#include "Editor/Modules/AssetImportingModule.h"
#include "Editor/Modules/WindowsModule.h"
#include "Editor/Windows/ContentWindow.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/Dragdata.h"
#include "Runtime/UI/GUI/RootControl.h"
#include "Runtime/UI/GUI/Style.h"
#include "Runtime/UI/GUI/Panels/Panel.h"

namespace SE::Editor
{
	ContentView::ContentView()
	{
		// Setup input actions
		/*InputActions = new InputActionsContainer(new[]
		{
			new InputActionsContainer.Binding(options => options.Delete, () =>
			{
				if (HasSelection)
					OnDelete?.Invoke(_selection);
			}),
			new InputActionsContainer.Binding(options => options.SelectAll, SelectAll),
			new InputActionsContainer.Binding(options => options.DeselectAll, DeselectAll),
			new InputActionsContainer.Binding(options => options.Rename, () =>
			{
				if (HasSelection && _selection[0].CanRename)
				{
					if (_selection.Count > 1)
						Select(_selection[0]);
					OnRename?.Invoke(_selection[0]);
				}
			}),
			new InputActionsContainer.Binding(options => options.Copy, Copy),
			new InputActionsContainer.Binding(options => options.Paste, Paste),
			new InputActionsContainer.Binding(options => options.Duplicate, Duplicate),
		});*/
	}

	void ContentView::ClearItems()
	{
		// Lock layout
		bool wasLayoutLocked = GetIsLayoutLocked();
		SetIsLayoutLocked(true);

		// Deselect items first
		ClearSelection();

		// Remove references and unlink items
		for (int i = 0; i < _items.Count(); i++)
		{
			ContentItem* item = _items[i];
			item->Parent = nullptr;
			item->RemoveReference(this);
		}
		_items.Clear();

		// Unload and perform UI layout
		SetIsLayoutLocked(wasLayoutLocked);
		PerformLayout();
	}

	void ContentView::ShowItems(List<ContentItem*>& items, SortType sortType, bool additive, bool keepSelection)
	{
		// Check if show nothing or not change view
		if (items.Count() == 0)
		{
			// Deselect items if need to
			if (!additive)
				ClearItems();
			return;
		}

		// Lock layout
		bool wasLayoutLocked = GetIsLayoutLocked();
		SetIsLayoutLocked(true);
		List<ContentItem*> selection = !additive && keepSelection ? _selection : List<ContentItem*>();

		// Deselect items if need to
		if (!additive)
			ClearItems();

		// Add references and link items
		for (int i = 0; i < items.Count(); i++)
		{
			ContentItem* item = items[i];
			if (item->Visible && !_items.Contains(item))
			{
				item->Parent = this;
				item->AddReference(this);
				_items.Add(item);
			}
		}
		if (selection.Count() > 0)
		{
			_selection.Clear();
			_selection.Add(selection);
		}

		// Sort items depending on sortMethod parameter
		auto sort = CreateFunc([sortType](Control* const & control,  Control* const & control1)
		{
			if (control == nullptr || control1 == nullptr)
				return false;
			if (sortType == SortType::AlphabeticReverse)
			{
				if (control->Compare(control1) > 0)
					return false;
				if (control->Compare(control1) == 0)
					return false;
				return true;
			}
			return control->Compare(control1) > 0;
		});
		Sorting::QuickSort(m_Children, sort);

		// Unload and perform UI layout
		SetIsLayoutLocked(wasLayoutLocked);
		PerformLayout();
	}

	bool ContentView::IsSelected(ContentItem* item)
	{
		return _selection.Contains(item);
	}

	void ContentView::ClearSelection()
	{
		if (_selection.Count() == 0)
			return;

		_selection.Clear();
	}

	void ContentView::Select(List<ContentItem*>& items, bool additive)
	{
		// Check if nothing to select
		if (items.Count() == 0)
		{
			// Deselect items if need to
			if (!additive)
				ClearSelection();
			return;
		}

		// Lock layout
		bool wasLayoutLocked = GetIsLayoutLocked();
		SetIsLayoutLocked(true);

		// Select items
		if (additive)
		{
			for (int i = 0; i < items.Count(); i++)
			{
				if (!_selection.Contains(items[i]))
					_selection.Add(items[i]);
			}
		}
		else
		{
			_selection.Clear();
			_selection.Add(items);
		}

		// Unload and perform UI layout
		SetIsLayoutLocked(wasLayoutLocked);
		PerformLayout();
	}

	void ContentView::Select(ContentItem* item, bool additive)
	{
		// Lock layout
		bool wasLayoutLocked = GetIsLayoutLocked();
		SetIsLayoutLocked(true);

		// Select item
		if (additive)
		{
			if (!_selection.Contains(item))
				_selection.Add(item);
		}
		else
		{
			_selection.Clear();
			_selection.Add(item);
		}

		// Unload and perform UI layout
		SetIsLayoutLocked(wasLayoutLocked);
		PerformLayout();
	}

	void ContentView::SelectAll()
	{
		BulkSelectUpdate(true);
	}

	void ContentView::DeselectAll()
	{
		BulkSelectUpdate(false);
	}

	void ContentView::Deselect(ContentItem* item)
	{
		ENGINE_ASSERT(item == nullptr)

		// Lock layout
		bool wasLayoutLocked = GetIsLayoutLocked();
		SetIsLayoutLocked(true);

		// Deselect item
		if (_selection.Contains(item))
			_selection.Remove(item);

		// Unload and perform UI layout
		SetIsLayoutLocked(wasLayoutLocked);
		PerformLayout();
	}

	void ContentView::Duplicate()
	{
		OnDuplicate(_selection);
	}

	void ContentView::Copy()
	{
		if (_selection.Count() == 0)
			return;
		List<String> files;
		for (ContentItem* item : _selection)
		{
			files.Add(item->Path);
		}
		Clipboard::SetFiles(files);
	}

	bool ContentView::CanPaste()
	{
		List<String> files = Clipboard::GetFiles();
		return files.Count() > 0;
	}

	void ContentView::Paste()
	{
		List<String> files = Clipboard::GetFiles();
		if (files.Count() == 0)
			return;

		OnPaste(files);
	}

	void ContentView::SelectFirstItem()
	{
		if (_items.Count() > 0)
		{
			_items[0]->Focus();
			Select(_items[0]);
		}
		else
		{
			Focus();
		}
	}

	void ContentView::RefreshThumbnails()
	{
		for (int i = 0; i < _items.Count(); i++)
		{
			_items[i]->RefreshThumbnail();
		}
	}

	void ContentView::OnItemClick(ContentItem* item)
	{
		bool isSelected = _selection.Contains(item);

		// Add/remove from selection
		if (Root->GetKey(KeyboardKeys::Control))
		{
			if (isSelected)
				Deselect(item);
			else
				Select(item, true);
		}
		// Range select
		else if (_selection.Count() != 0 && Root->GetKey(KeyboardKeys::Shift))
		{
			int min = Max_int32;
			int max = Min_int32;
			for (ContentItem* itemTemp : _selection)
			{
				min = Math::Min(min, itemTemp->IndexInParent.operator->());
				max = Math::Max(max, itemTemp->IndexInParent.operator->());
			}

			min = Math::Max(Math::Min(min, item->IndexInParent.operator->()), 0);
			max = Math::Min(Math::Max(max, item->IndexInParent.operator->()), m_Children.Count() - 1);
			List<ContentItem*> selection = _selection;
			for (int i = min; i <= max; i++)
			{
				ContentItem* cc = TypeTryCast<ContentItem>(m_Children[i]);
				if (cc != nullptr && !selection.Contains(cc))
				{
					selection.Add(cc);
				}
			}
			Select(selection);
		}
		// Select
		else
		{
			Select(item);
		}
	}

	void ContentView::OnItemDoubleClick(ContentItem* item)
	{
		OnOpen(item);
	}

	void ContentView::OnItemDeleted(ContentItem* item)
	{
		_selection.Remove(item);
		_items.Remove(item);
	}

	void ContentView::OnItemRenamed(ContentItem* item)
	{

	}

	void ContentView::OnItemReimported(ContentItem* item)
	{
	}

	void ContentView::OnItemDispose(ContentItem* item)
	{
		_selection.Remove(item);
		_items.Remove(item);
	}

	void ContentView::Draw()
	{
		ContainerControl::Draw();

		Style* style = Style::Current;

		// Check if drag is over
		if (IsDragOver() && _validDragOver)
		{
			Rectangle bounds = Rectangle(Float2::One, Float2::One * 2.0f - Size);
			Render2D::FillRectangle(bounds, style->Selection);
			Render2D::DrawRectangle(bounds, style->SelectionBorder);
		}

		// Check if it's an empty thing
		if (_items.Count() == 0)
		{
			Render2D::RenderText(style->FontSmall, IsSearching ? SE_TEXT("No results") : SE_TEXT("Empty"),
				Rectangle(Float2::Zero, Size), style->ForegroundDisabled, TextAlignment::Center, TextAlignment::Center);
		}

		// Selection
		if (_isRubberBandSpanning)
		{
			Render2D::FillRectangle(_rubberBandRectangle, style->Selection);
			Render2D::DrawRectangle(_rubberBandRectangle, style->SelectionBorder);
		}
	}

	bool ContentView::OnMouseDown(Float2 location, MouseButton button)
	{
		if (ContainerControl::OnMouseDown(location, button))
			return true;

		if (button == MouseButton::Left)
		{
			_mousePressLocation = location;
			_rubberBandRectangle = Rectangle(_mousePressLocation.x, _mousePressLocation.y, 0.0, 0.0);
			_isRubberBandSpanning = true;
			StartMouseCapture();
			return true;
		}

		return AutoFocus && Focus(this);
	}

	void ContentView::OnMouseMove(Float2 location)
	{
		if (_isRubberBandSpanning)
		{
			_rubberBandRectangle.SetWidth(location.x - _mousePressLocation.x);
			_rubberBandRectangle.SetHeight(location.y - _mousePressLocation.y);
		}

		ContainerControl::OnMouseMove(location);
	}

	bool ContentView::OnMouseUp(Float2 location, MouseButton button)
	{
		if (_isRubberBandSpanning)
		{
			_isRubberBandSpanning = false;
			EndMouseCapture();
			if (_rubberBandRectangle.GetWidth() < 0 || _rubberBandRectangle.GetHeight() < 0)
			{
				// make sure we have a well-formed rectangle i.e. size is positive and X/Y is upper left corner
				Float2 size = _rubberBandRectangle.Size;
				_rubberBandRectangle.SetX(Math::Min(_rubberBandRectangle.GetX(), _rubberBandRectangle.GetX() + _rubberBandRectangle.GetWidth()));
				_rubberBandRectangle.SetY(Math::Min(_rubberBandRectangle.GetY(), _rubberBandRectangle.GetY() + _rubberBandRectangle.GetHeight()));
				size.x = Math::Abs(size.x);
				size.y = Math::Abs(size.y);
				_rubberBandRectangle.Size = size;
			}
			List<ContentItem*> itemsInRectangle;

			ListExtensions::Where(_items, Function<bool(ContentItem* const &t)>([this](ContentItem* const &t)
			{
				return _rubberBandRectangle.Intersects(t->Bounds);
			}), itemsInRectangle);

			Select(itemsInRectangle, Input::GetKey(KeyboardKeys::Shift) || Input::GetKey(KeyboardKeys::Control));
			return true;
		}

		return ContainerControl::OnMouseUp(location, button);
	}

	bool ContentView::OnMouseWheel(Float2 location, float delta)
	{
		// Check if pressing control key
		if (Root->GetKey(KeyboardKeys::Control))
		{
			// Zoom
			ViewScale = ViewScale + delta * 0.05f;

			// Handled
			return true;
		}

		return ContainerControl::OnMouseWheel(location, delta);
	}

	bool ContentView::OnKeyDown(KeyboardKeys key)
	{
		// Navigate backward
		if (key == KeyboardKeys::Backspace)
		{
			OnNavigateBack();
			return true;
		}

		/*if (InputActions.Process(Editor.Instance, this, key))
			return true;*/

		// Check if sth is selected
		if (HasSelection)
		{
			// Open
			if (key == KeyboardKeys::Return && _selection.Count() != 0)
			{
				for (ContentItem* e : _selection)
				{
					OnOpen(e);
				}
				return true;
			}

			// Movement with arrows
			{
				ContentItem* root = _selection[0];
				Float2 size = root->Size;
				Float2 offset = Float2::Minimum;
				ContentItem* item = nullptr;
				if (key == KeyboardKeys::ArrowUp)
				{
					offset = Float2(0, -size.y);
				}
				else if (key == KeyboardKeys::ArrowDown)
				{
					offset = Float2(0, size.y);
				}
				else if (key == KeyboardKeys::ArrowRight)
				{
					offset = Float2(size.x, 0);
				}
				else if (key == KeyboardKeys::ArrowLeft)
				{
					offset = Float2(-size.x, 0);
				}
				if (offset != Float2::Minimum)
				{
					item = TypeTryCast<ContentItem>(GetChildAt(size / 2.0f + offset + root->Location));;
				}
				if (item != nullptr)
				{
					OnItemClick(item);
					return true;
				}
			}
		}

		return ContainerControl::OnKeyDown(key);
	}
	
	bool ContentView::OnCharInput(Char c)
	{
		if (ContainerControl::OnCharInput(c))
			return true;

		if (StringUtils::IsAlnum(c) && _items.Count() != 0)
		{
			// Jump to the item starting with this character
			c = StringUtils::ToLower(c);
			for (int i = 0; i < _items.Count(); i++)
			{
				ContentItem* item = _items[i];
				String& name = item->ShortName;
				if (name.Length() > 0 && StringUtils::ToLower(name[0]) == c)
				{
					Select(item);
					Panel* panel = TypeTryCast<Panel>(Parent);
					if (panel != nullptr)
					{
						panel->ScrollViewTo(item, true);
					}
					break;
				}
			}
		}

		return false;
	}
	
	DragDropEffect ContentView::OnDragEnter(const Float2& location, DragData* data)
	{
		DragDropEffect result = ContainerControl::OnDragEnter(location, data);
		if (result != DragDropEffect::None)
			return result;


		// Check if drop file(s)
		if (TypeIs<DragDataFiles>(data))
		{
			_validDragOver = true;
			return DragDropEffect::Copy;
		}
		/*
		// Check if drop actor(s)
		if (_dragActors == null)
			_dragActors = new DragActors(ValidateDragActors);
		if (_dragActors.OnDragEnter(data))
		{
			_validDragOver = true;
			return DragDropEffect::Move;
		}*/

		return DragDropEffect::None;
	}
	
	DragDropEffect ContentView::OnDragMove(const Float2& location, DragData* data)
	{
		_validDragOver = false;
		DragDropEffect result = ContainerControl::OnDragMove(location, data);
		if (result != DragDropEffect::None)
			return result;

		if (TypeAs<DragDataFiles>(data))
		{
			_validDragOver = true;
			result = DragDropEffect::Copy;
		}
		/*else if (_dragActors != null && _dragActors.HasValidDrag)
		{
			_validDragOver = true;
			result = DragDropEffect::Move;
		}*/

		return result;
	}

	DragDropEffect ContentView::OnDragDrop(const Float2& location, DragData* data)
	{
		DragDropEffect result = ContainerControl::OnDragDrop(location, data);
		if (result != DragDropEffect::None)
			return result;

		// Check if drop file(s)
		DragDataFiles* files;
		if (TypeTryCast<DragDataFiles>(data, files))
		{
			// Import files
			ContentFolder* currentFolder = EditorApp::Ins().windowsModule->ContentWin->CurrentViewFolder;
			if (currentFolder != nullptr)
			{
				EditorApp::Ins().importingModule->Import(files->Files.Get(), files->Files.Count(), currentFolder);
			}
			result = DragDropEffect::Copy;
		}
		/*// Check if drop actor(s)
		else if (_dragActors != null && _dragActors.HasValidDrag)
		{
			// Import actors
			var currentFolder = Editor.Instance.Windows.ContentWin.CurrentViewFolder;
			if (currentFolder != null)
				ImportActors(_dragActors, currentFolder);

			_dragActors.OnDragDrop();
			result = DragDropEffect::Move;
		}*/

		// Clear cache
		_validDragOver = false;

		return result;
	}

	void ContentView::OnDragLeave()
	{
		_validDragOver = false;
		// _dragActors.OnDragLeave();

		ContainerControl::OnDragLeave();
	}

	void ContentView::OnDestroy()
	{
		if (IsDisposing)
			return;

		// Ensure to unlink all items
		ClearItems();

		ContainerControl::OnDestroy();
	}

	void ContentView::PerformLayoutBeforeChildren()
	{
		float width = GetClientArea().GetWidth();
		float x = 0, y = 1;
		float viewScale = _viewScale * 0.97f;

		switch (ViewType)
		{
		case ContentViewType::Tiles:
		{
			float defaultItemsWidth = ContentItem::DefaultWidth * viewScale;
			int itemsToFit = Math::FloorToInt(width / defaultItemsWidth) - 1;
			if (itemsToFit < 1)
			{
				itemsToFit = 1;
			}
			int xSpace = 4;
			float itemsWidth = width / Math::Max(itemsToFit, 1) - xSpace;
			float itemsHeight = itemsWidth / defaultItemsWidth * (ContentItem::DefaultHeight * viewScale);
			float flooredItemsWidth = Math::Floor(itemsWidth);
			float flooredItemsHeight = Math::Floor(itemsHeight);
			x = itemsToFit == 1 ? 1 : itemsWidth / itemsToFit + xSpace;
			for (int i = 0; i < m_Children.Count(); i++)
			{
				Control* c = m_Children[i];
				c->Bounds = Rectangle(Math::Floor(x), Math::Floor(y), flooredItemsWidth, flooredItemsHeight);

				x += (itemsWidth + xSpace) + (itemsWidth + xSpace) / itemsToFit;
				if (x + itemsWidth > width)
				{
					x = itemsToFit == 1 ? 1 : itemsWidth / itemsToFit + xSpace;
					y += itemsHeight + 7;
				}
			}
			if (x > 0)
			{
				y += itemsHeight;
			}

			break;
		}
		case ContentViewType::List:
		{
			float itemsHeight = 50.0f * viewScale;
			for (int i = 0; i < m_Children.Count(); i++)
			{
				Control* c = m_Children[i];
				c->Bounds = Rectangle(x, y, width, itemsHeight);
				y += itemsHeight + 1;
			}
			y += 40.0f;

			break;
		}
		}

		// Set maximum size and fit the parent container
		if (HasParent())
		{
			y = Math::Max(y, Parent->Height.operator->());
		}
		Height = y;

		ContainerControl::PerformLayoutBeforeChildren();
	}

	void ContentView::BulkSelectUpdate(bool select)
	{
		// Lock layout
		bool wasLayoutLocked = GetIsLayoutLocked();
		SetIsLayoutLocked(true);

		// Select items
		_selection.Clear();
		if (select)
		{
			_selection.Add(_items);
		}

		// Unload and perform UI layout
		SetIsLayoutLocked(wasLayoutLocked);
		PerformLayout();
	}

	void ContentView::__SetViewType(ContentViewType value)
	{
		if (_viewType != value)
		{
			_viewType = value;
			ViewTypeChanged();
			PerformLayout();
		}
	}

	void ContentView::__SetViewScale(float value)
	{
		value = Math::Clamp(value, 0.3f, 3.0f);
		if (!Math::IsNearEqual(value, _viewScale))
		{
			_viewScale = value;
			ViewScaleChanged();
			PerformLayout();
		}
	}
} // SE