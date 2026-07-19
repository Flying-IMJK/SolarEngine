
#include "ComboBox.h"

#include "ContextMenu/ContextMenu.h"
#include "ContextMenu/ContextMenuButton.h"
#include "Runtime/Core/Types/Collections/ListExtensions.h"
#include "Runtime/Core/Types/Collections/Sorting.h"
#include "Runtime/Graphics/GraphicWindow.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/Style.h"
#include "Runtime/UI/GUI/WindowRootControl.h"
#include "Runtime/UI/GUI/Brushes/IBrush.h"
#include "Runtime/UI/GUI/Brushes/SpriteBrush.h"

namespace SE::Editor
{
	ComboBox::ComboBox() : ComboBox(0, 0)
	{
	}
	ComboBox::ComboBox(float x, float y, float width) : Control(x, y, width, DefaultHeight)
	{
		MaximumItemsInViewCount = 20;

		Style* style = Style::Current;
		Font = FontReference(style->FontMedium);
		TextColor = style->Foreground;
		BackgroundColor = style->BackgroundNormal;
		BackgroundColorHighlighted = BackgroundColor;
		BackgroundColorSelected = BackgroundColor;
		BorderColor = style->BorderNormal;
		BorderColorHighlighted = style->BorderSelected;
		BorderColorSelected = BorderColorHighlighted;
		ArrowImage = New<SpriteBrush>(style->ArrowDown);
		ArrowColor = style->Foreground * 0.6f;
		ArrowColorSelected = style->BackgroundSelected;
		ArrowColorHighlighted = style->Foreground;
	}

	void ComboBox::ClearItems()
	{
		SelectedIndex = -1;
		items.Clear();
	}

	void ComboBox::AddItem(String& item)
	{
		items.Add(item);
	}

	void ComboBox::AddItems(List<String>& items)
	{
		items.Add(items);
	}

	void ComboBox::SetItems(List<String>& items)
	{
		SelectedIndex = -1;
		items.Clear();
		items.Add(items);
	}

	bool ComboBox::IsSelected(String& item)
	{
		return IsSelected(items.Find(item));
	}

	bool ComboBox::IsSelected(int index)
	{
		return index != -1 && _selectedIndices.Contains(index);
	}
	
	void ComboBox::OnDestroy()
	{
		if (_popupMenu != nullptr)
		{
			_popupMenu->Hide();
			_popupMenu->Dispose();
			_popupMenu = nullptr;
		}

		if (IsDisposing)
			return;
		_selectedIndices.Clear();
		items.Clear();

		Control::OnDestroy();
	}
	
	void ComboBox::Draw()
	{
		// Cache data
		Rectangle clientRect = Rectangle(Float2::Zero, Size);
		float margin = clientRect.GetHeight() * 0.2f;
		float boxSize = clientRect.GetHeight() - margin * 2;
		bool isOpened = IsPopupOpened;
		bool enabled = EnabledInHierarchy();
		Color backgroundColor = BackgroundColor;
		Color borderColor = BorderColor;
		Color arrowColor = ArrowColor;
		if (!enabled)
		{
			backgroundColor *= 0.5f;
			arrowColor *= 0.7f;
		}
		else if (isOpened || _mouseDown)
		{
			backgroundColor = BackgroundColorSelected;
			borderColor = BorderColorSelected;
			arrowColor = ArrowColorSelected;
		}
		else if (IsMouseOver || IsNavFocused)
		{
			backgroundColor = BackgroundColorHighlighted;
			borderColor = BorderColorHighlighted;
			arrowColor = ArrowColorHighlighted;
		}

		// Background
		Render2D::FillRectangle(clientRect, backgroundColor);
		Render2D::DrawRectangle(clientRect.MakeExpanded(-2.0f), borderColor);

		// Check if has selected item
		if (_selectedIndices.Count() > 0)
		{
			String text = _selectedIndices.Count() == 1 ? (_selectedIndices[0] >= 0 && _selectedIndices[0] < items.Count() ? items[_selectedIndices[0]] : SE_TEXT("")) : SE_TEXT("Multiple Values");

			// Draw text of the selected item
			float textScale = Height / DefaultHeight;
			Rectangle textRect = Rectangle(margin, 0, clientRect.GetWidth() - boxSize - 2.0f * margin, clientRect.GetHeight());
			Render2D::PushClip(textRect);
			Color textColor = TextColor;
			Render2D::RenderText(Font.GetFont(), text, textRect, enabled ? textColor : textColor * 0.5f, TextAlignment::Near, TextAlignment::Center, TextWrapping::NoWrap, 1.0f, textScale);
			Render2D::PopClip();
		}

		// Arrow
		if (ArrowImage != nullptr)
		{
			ArrowImage->Draw(Rectangle(clientRect.GetWidth() - margin - boxSize, margin, boxSize, boxSize), arrowColor);
		}
	}
	
	void ComboBox::OnLostFocus()
	{
		Control::OnLostFocus();

		// Clear flags
		_mouseDown = false;
		_blockPopup = false;
	}
	
	void ComboBox::OnMouseLeave()
	{
		// Clear flags
		_mouseDown = false;
		_blockPopup = false;

		Control::OnMouseLeave();
	}
	
	bool ComboBox::OnMouseDown(Float2 location, MouseButton button)
	{
		if (button == MouseButton::Left)
		{
			_mouseDown = true;
			Focus();
			return true;
		}

		return Control::OnMouseDown(location, button);
	}
	
	bool ComboBox::OnMouseUp(Float2 location, MouseButton button)
	{
		if (_mouseDown && !_blockPopup)
		{
			_mouseDown = false;
			ShowPopup();
		}
		else
		{
			_blockPopup = false;
		}

		return true;
	}

	void ComboBox::OnSubmit()
	{
		Control::OnSubmit();

		ShowPopup();
	}

	void ComboBox::OnSelectedIndexChanged()
	{
		if (tooltips.Count() == items.Count())
		{
			TooltipText = _selectedIndices.Count() == 1 ? tooltips[_selectedIndices[0]] : nullptr;
		}
		SelectedIndexChanged(this);
	}

	void ComboBox::ShowPopup()
	{
		// Ensure to have valid menu
		if (_popupMenu == nullptr)
		{
			_popupMenu = OnCreatePopup();
			_popupMenu->MaximumItemsInViewCount = MaximumItemsInViewCount;

			// Bind events
			_popupMenu->VisibleChanged.Bind([this](Control* cm)
			{
				RootControl* win = Root;
				_blockPopup = win != nullptr && Rectangle(Float2::Zero, Size).Contains(PointFromWindow(win->GetMousePosition()));
				if (!_blockPopup)
					Focus();
			});
			_popupMenu->ButtonClicked.Bind([this](ContextMenuButton* btn)
			{
				OnItemClicked(std::any_cast<int>(btn->Tag));
				if (SupportMultiSelect)
				{
					// Don't hide in multi-select, so user can edit multiple elements instead of just one
					UpdateButtons();
					if (_popupMenu != nullptr)
					{
						_popupMenu->PerformLayout();
					}

				}
				else
				{
					if (_popupMenu != nullptr)
					{
						_popupMenu->Hide();
					}
				}
			});
		}

		// Check if menu hs been already shown
		if (_popupMenu->Visible)
		{
			if (!SupportMultiSelect)
				_popupMenu->Hide();
			return;
		}

		PopupShowing(this);

		// Check if has any items
		if (items.Count() > 0)
		{
			UpdateButtons();

			// Show dropdown list
			_popupMenu->MinimumWidth = Width;
			_popupMenu->Show(this, Float2(1, Height));

			// Adjust menu position if it is not the down direction
			if (_popupMenu->Direction == ContextMenuDirection::RightUp)
			{
				Float2 position = _popupMenu->RootWindow()->Window()->GetPosition();
				_popupMenu->RootWindow()->Window()->SetPosition(Float2(position.x, position.y - Height));
			}
		}
	}

	void ComboBox::OnLayoutMenuButton(ContextMenuButton* button, int index, bool construct)
	{
		button->SetChecked(_selectedIndices.Contains(index));
		if (tooltips.Count() > index)
			button->TooltipText = tooltips[index];
	}

	ContextMenu* ComboBox::OnCreatePopup()
	{
		if (PopupCreate.IsBinded())
			return PopupCreate(this);
		return New<ContextMenu>();
	}

	void ComboBox::UpdateButtons()
	{
		if (_popupMenu->Items.operator->().Count() != items.Count())
		{
			List<ContextMenuItem*> itemControls = _popupMenu->Items;
			for (auto e : itemControls)
			{
				e->Dispose();
			}
			if (Sorted)
			{
				Sorting::QuickSort(items);
			}
			for (int i = 0; i < items.Count(); i++)
			{
				ContextMenuButton* btn = _popupMenu->AddButton(items[i]);
				OnLayoutMenuButton(btn, i, true);
				btn->Tag = i;
			}
		}
		else
		{
			List<ContextMenuItem*> itemControls = _popupMenu->Items;
			if (Sorted)
			{
				Sorting::QuickSort(items);
			}
			for (int i = 0; i < items.Count(); i++)
			{
				ContextMenuButton* btn;
				if (TypeTryCast<ContextMenuButton>(itemControls[i], btn))
				{
					btn->Text = items[i];
					OnLayoutMenuButton(btn, i, true);
				}
			}
		}
	}

	void ComboBox::__SetSelectedIndex(int value)
	{
		// Clamp index
		value = Math::Min(value, items.Count() - 1);

		// Check if index will change
		if (value != SelectedIndex)
		{
			// Select
			_selectedIndices.Clear();
			if (value != -1)
				_selectedIndices.Add(value);
			OnSelectedIndexChanged();
		}
	}

	void ComboBox::__SetSelection(List<int>& value)
	{
		ENGINE_ASSERT(!(!SupportMultiSelect && value.Count() > 1))

		for (int i = 0; i < value.Count(); i++)
		{
			int index = value[i];
			if (index < 0 || index >= items.Count())
			{
				ENGINE_ASSERT(false);
			}
		}

		Function<bool(const int& a, const int& b)> func = [](const int& a, const int& b)
		{
			return a == b;
		};
		if (!ListExtensions::SequenceEqual(_selectedIndices, value, func))
		{
			// Select
			_selectedIndices.Clear();
			_selectedIndices.Add(value);
			OnSelectedIndexChanged();
		}
	}

	bool ComboBox::__GetIsPopupOpened()
	{
		return _popupMenu != nullptr && _popupMenu->IsOpened;
	}
} // SE