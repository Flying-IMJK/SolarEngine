//
// Created by 10303 on 25-8-7.
//

#include "ContextMenu.h"

#include "ContextMenuButton.h"
#include "ContextMenuChildMenu.h"
#include "ContextMenuItem.h"
#include "ContextMenuSeparator.h"
#include "Core/Types/Collections/ListExtensions.h"
#include "Core/Types/Collections/Sorting.h"

namespace SE::Editor
{
	ContextMenu::ItemsPanel::ItemsPanel(ContextMenu* menu) : _menu(menu)
	{
	}

	void ContextMenu::ItemsPanel::Arrange()
	{
		Panel::Arrange();

		// Arrange controls
		Margin margin = _menu->_itemsMargin;
		float y = 0;
		float width = Width - margin.GetWidth();
		for (int i = 0; i < m_Children.Count(); i++)
		{
			Control* c = m_Children[i];
			if (!c->Visible)
			{
				continue;
			}

			ContextMenuItem* item = TypeTryCast<ContextMenuItem>(c);
			if (item != nullptr)
			{
				float height = item->Height;
				item->Bounds = Rectangle(margin.Left, y, width, height);
				y += height + margin.GetHeight();
			}
		}
	}

	ContextMenu::ContextMenu()
	{
		MinimumWidth = 10;
		MaximumItemsInViewCount = 20;

		_panel = New<ItemsPanel>(this);
		_panel->ClipChildren = false;
		_panel->Parent = this;
	}

	void ContextMenu::SortButtons(bool force)
	{
		if (!_autosort && !force)
			return;

		List<Control*>& children = _panel->Children();
		auto compare = CreateFunc([]( Control* const &control, Control* const &control1)
		{
			bool v = false;
			ContextMenuButton const* cmb = TypeTryCast<ContextMenuButton>(control);
			ContextMenuButton const* cmb1 = TypeTryCast<ContextMenuButton>(control1);
			if (cmb != nullptr && cmb1 != nullptr)
				v = StringUtils::Compare(cmb->Text.Get(), cmb1->Text.Get()) > 0;
			if (cmb == nullptr)
				v = true;

			return v;
		});
		Sorting::QuickSort(children.Get(), children.Count(), compare);
	}

	void ContextMenu::DisposeAllItems()
	{
		for (int i = _panel->ChildrenCount() - 1; _panel->ChildrenCount() > 0 && i >= 0; i--)
		{
			Control* c = _panel->Children()[i];
			if (TypeAs<ContextMenuItem>(c))
			{
				c->Dispose();
			}
		}
	}

	ContextMenuButton* ContextMenu::AddButton(String text)
	{
		ContextMenuButton* item = New<ContextMenuButton>(this, text);
		item->Parent = _panel;

		SortButtons();
		return item;
	}

	ContextMenuButton* ContextMenu::AddButton(String text, String shortKeys)
	{
		ContextMenuButton* item = New<ContextMenuButton>(this, text, shortKeys);
		item->Parent = _panel;
		SortButtons();
		return item;
	}

	ContextMenuButton* ContextMenu::AddButton(String text, Function<void()> clicked)
	{
		ContextMenuButton* item = New<ContextMenuButton>(this, text);
		item->Parent = _panel;

		item->Clicked.BindUnique(clicked);
		SortButtons();
		return item;
	}

	ContextMenuButton* ContextMenu::AddButton(String text, String shortKeys, Function<void()> clicked)
	{
		ContextMenuButton* item = New<ContextMenuButton>(this, text, shortKeys);
		item->Parent = _panel;
		item->Clicked.BindUnique(clicked);
		SortButtons();
		return item;
	}

	ContextMenuChildMenu* ContextMenu::GetChildMenu(String text)
	{
		for (int i = 0; i < _panel->ChildrenCount(); i++)
		{
			ContextMenuChildMenu* menu = TypeTryCast<ContextMenuChildMenu>(_panel->Children()[i]);
			if (menu != nullptr && menu->Text == text)
				return menu;
		}
		return nullptr;
	}

	ContextMenuChildMenu* ContextMenu::GetOrAddChildMenu(String text)
	{
		ContextMenuChildMenu* item = GetChildMenu(text);
		if (item == nullptr)
		{
			item = New<ContextMenuChildMenu>(this, text);
			item->Parent = _panel;
		}
		return item;
	}

	ContextMenuChildMenu* ContextMenu::AddChildMenu(String text)
	{
		ContextMenuChildMenu* item = New<ContextMenuChildMenu>(this, text);
		item->Parent = _panel;
		return item;
	}

	ContextMenuSeparator* ContextMenu::AddSeparator()
	{
		ContextMenuSeparator* item = New<ContextMenuSeparator>(this);
		item->Parent = _panel;
		return item;
	}

	ContextMenuButton* ContextMenu::AddButton(String text, Function<void(ContextMenuButton*)> clicked)
	{
		ContextMenuButton* item = New<ContextMenuButton>(this, text);
		item->Parent = _panel;
		item->ButtonClicked.BindUnique(clicked);
		SortButtons();
		return item;
	}

	void ContextMenu::OnButtonClicked(ContextMenuButton* button)
	{
		ButtonClicked(button);
	}

	void ContextMenu::Show(Control* parent, Float2 location)
	{
		// Remove last separator to make context menu look better
		int lastIndex = _panel->ChildrenCount() - 1;
		if (lastIndex >= 0)
		{
			ContextMenuSeparator* separator = TypeTryCast<ContextMenuSeparator>(parent);
			if (separator != nullptr)
			{
				separator->Dispose();
			}
		}


		ContextMenuBase::Show(parent, location);
	}

	bool ContextMenu::ContainsPoint(Float2& location, bool precise)
	{
		if (ContextMenuBase::ContainsPoint(location, precise))
			return true;

		Float2 cLocation = location - Location;
		for (int i = 0; i < _panel->ChildrenCount(); i++)
		{
			if (_panel->Children()[i]->ContainsPoint(cLocation, precise))
				return true;
		}

		return false;
	}

	bool ContextMenu::OnCharInput(Char c)
	{
		if (ContextMenuBase::OnCharInput(c))
			return true;

		// Find the item that starts with that character
		if (StringUtils::IsAlnum(c) && _panel->vScrollBar != nullptr && _panel->vScrollBar->Visible)
		{
			int startIndex = 0;
			for (int i = 0; i < _panel->ChildrenCount(); i++)
			{
				ContextMenuButton* item = TypeTryCast<ContextMenuButton>(_panel->Children()[i]);
				if (item != nullptr && item->Visible && item->IsFocused)
				{
					// Start searching from the last hit item
					startIndex = i + 1;
					break;
				}
			}
			for (int i = startIndex; i < _panel->ChildrenCount(); i++)
			{
				ContextMenuButton* item = TypeTryCast<ContextMenuButton>(_panel->Children()[i]);
				if (item != nullptr && item->Visible)
				{
					bool startsWith = false;
					for (int j = 0; j < item->Text.Length(); j++)
					{
						Char k = item->Text[j];
						if (StringUtils::ToLower(k) == StringUtils::ToLower(c))
						{
							startsWith = true;
							break;
						}
						if (!StringUtils::IsWhitespace(k) && k != '>')
							break;
					}
					if (startsWith)
					{
						// Focus found item
						item->Focus();
						_panel->ScrollViewTo(item);
						return true;
					}
				}
			}
			if (startIndex > 0 && startIndex <= _panel->ChildrenCount())
			{
				// No more items found so start from the top if there are matching items
				_panel->Children()[startIndex - 1]->Defocus();
				return OnCharInput(c);
			}
		}

		return false;
	}

	bool ContextMenu::OnKeyDown(KeyboardKeys key)
	{
		if (ContextMenuBase::OnKeyDown(key))
			return true;

		switch (key)
		{
		case KeyboardKeys::ArrowDown:
			for (int i = 0; i < _panel->ChildrenCount(); i++)
			{
				ContextMenuButton* item = TypeTryCast<ContextMenuButton>(_panel->Children()[i]);

				if (item != nullptr && item->Visible && item->Enabled)
				{
					item->Focus();
					_panel->ScrollViewTo(item);
					return true;
				}
			}
			break;
		case KeyboardKeys::ArrowUp:
			for (int i = _panel->ChildrenCount() - 1; i >= 0; i--)
			{
				ContextMenuButton* item = TypeTryCast<ContextMenuButton>(_panel->Children()[i]);

				if (item != nullptr && item->Visible && item->Enabled)
				{
					item->Focus();
					_panel->ScrollViewTo(item);
					return true;
				}
			}
			break;
		}

		return false;
	}

	void ContextMenu::PerformLayoutAfterChildren()
	{
		Float2 prevSize = Size;

		// Calculate size of the context menu (items only)
		float maxWidth = 0;
		float height = _itemsAreaMargin.GetHeight();
		int itemsLeft = MaximumItemsInViewCount;
		int overflowItemCount = 0;
		int itemsCount = 0;
		for (int i = 0; i < _panel->ChildrenCount(); i++)
		{
			ContextMenuItem* item = TypeTryCast<ContextMenuItem>(_panel->Children()[i]);

			if (item != nullptr && item->Visible)
			{
				itemsCount++;
				if (itemsLeft > 0)
				{
					height += item->Height + _itemsMargin.GetHeight();
					itemsLeft--;
				}
				else
				{
					overflowItemCount++;
				}
				maxWidth = Math::Max(maxWidth, item->MinimumWidth.operator->());
			}
		}
		if (itemsCount != 0)
		{
			height -= _itemsMargin.GetHeight(); // Remove item margin from top and bottom
		}
		maxWidth = Math::Max(maxWidth + 20, MinimumWidth);

		// Move child arrows to accommodate scroll bar showing
		if (overflowItemCount > 0)
		{
			for (Control* child : _panel->Children())
			{
				ContextMenuButton* item = TypeTryCast<ContextMenuButton>(child);

				if (item != nullptr && item->Visible)
				{
					item->ExtraAdjustmentAmount = -_panel->vScrollBar->Width;
				}
			}
		}

		// Resize container
		Size = Float2(Math::Ceil(maxWidth), Math::Ceil(height));

		// Arrange items view panel
		Rectangle panelBounds = Rectangle(Float2::Zero, Size);
		_itemsAreaMargin.ShrinkRectangle(panelBounds);
		_panel->Bounds = panelBounds;

		// Check if is visible size get changed
		if (Visible && prevSize != Size)
		{
			// Update window dimensions
			UpdateWindowSize();
		}
	}

	void ContextMenu::__SetItemsAreaMargin(Margin &value)
	{
		_itemsAreaMargin = value;
		PerformLayout();
	}

	void ContextMenu::__SetItemsMargin(Margin &value)
	{
		_itemsMargin = value;
		PerformLayout();
	}

	List<ContextMenuItem*> ContextMenu::__GetItems()
	{
		List<ContextMenuItem*> list;
		for (Control* c : _panel->Children())
		{
			ContextMenuItem* menuItem;
			if (TypeTryCast<ContextMenuItem>(c, menuItem))
			{
				list.Add(menuItem);
			}
		}

		return list;
	}

	void ContextMenu::__SetAutoSort(bool value)
	{
		_autosort = value;
		if (_autosort)
		{
			SortButtons();
		}
	}
} // SE