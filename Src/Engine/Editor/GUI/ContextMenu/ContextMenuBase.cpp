//
// Created by 10303 on 25-8-6.
//

#include "ContextMenuBase.h"

#include "ContextMenu.h"
#include "Runtime/Graphics/GraphicWindow.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/Style.h"
#include "Runtime/UI/GUI/WindowRootControl.h"

namespace SE::Editor
{
	bool ContextMenuBase::UseAutomaticDirectionFix()
	{
		return true;
	}

	ContextMenuBase::ContextMenuBase() : ContainerControl(0, 0, 120, 32)
	{
		_direction = ContextMenuDirection::RightDown;
		Visible = false;

		_isSubMenu = true;
	}
	ContextMenuBase* ContextMenuBase::ShowEmptyMenu(Control* control, Rectangle area)
	{
		// Calculate the control size in the window space to handle scaled controls
		Float2 upperLeft = control->PointToWindow(area.GetUpperLeft());
		Float2 bottomRight = control->PointToWindow(area.GetBottomRight());
		Float2 size = bottomRight - upperLeft;

		ContextMenuBase* popup = New<ContextMenuBase>();
		popup->Size = size;
		popup->Show(control, area.Location + Float2(0, (size.y - popup->Height) * 0.5f));
		return popup;
	}
	
	void ContextMenuBase::Show(Control* parent, Float2 location)
	{
		ENGINE_ASSERT(parent != nullptr);

		// Ensure to be closed
		Hide();

		// Peek parent control window
		auto parentWin = parent->RootWindow();
		if (parentWin == nullptr)
			return;

		// Check if show menu inside the other menu - then link as a child to prevent closing the calling menu window on lost focus
		if (_parentCM == nullptr && parentWin->ChildrenCount() == 1)
		{
			ContextMenuBase* parentCM;
			if (TypeTryCast<ContextMenuBase>(parentWin->Children()[0], parentCM))
			{
				parentCM->ShowChild(this, parentCM->PointFromScreen(parent->PointToScreen(location)), false);
				return;
			}
		}

		// Unlock and perform controls update
		Location = Float2::Zero;
		UnlockChildrenRecursive();
		PerformLayout();

		// Calculate popup direction and initial location (fit on a single monitor)
		float dpiScale = parentWin->GetDpiScale();
		Float2 dpiSize = Size.operator->() * dpiScale;
		Float2 locationWS = parent->PointToWindow(location);
		Float2 locationSS = parentWin->PointToScreen(locationWS);
		Rectangle monitorBounds = Platform::GetMonitorBounds(locationSS);
		Float2 rightBottomLocationSS = locationSS + dpiSize;
		bool isUp = false, isLeft = false;
		if (UseAutomaticDirectionFix())
		{
			ContextMenu* parentMenu = TypeTryCast<ContextMenu>(parent);
			if (monitorBounds.GetBottom() < rightBottomLocationSS.y)
			{
				isUp = true;
				locationSS.y -= dpiSize.y;
				if (parentMenu != nullptr && parentMenu->_childCM != nullptr)
					locationSS.y += 30.0f * dpiScale;
			}
			if (parentMenu == nullptr)
			{
				if (monitorBounds.GetRight() < rightBottomLocationSS.x)
				{
					isLeft = true;
					locationSS.x -= dpiSize.x;
				}
			}
			else if (monitorBounds.GetRight() < rightBottomLocationSS.x ||
				(_parentCM != nullptr && (_parentCM->Direction == ContextMenuDirection::LeftDown || _parentCM->Direction == ContextMenuDirection::LeftUp)))
			{
				isLeft = true;
				if (IsSubMenu && _parentCM != nullptr)
					locationSS.x -= _parentCM->Width + dpiSize.x;
				else
					locationSS.x -= dpiSize.x;
			}
		}

		// Update direction flag
		if (isUp)
			_direction = isLeft ? ContextMenuDirection::LeftUp : ContextMenuDirection::RightUp;
		else
			_direction = isLeft ? ContextMenuDirection::LeftDown : ContextMenuDirection::RightDown;

		// Create window
		CreateWindowSettings desc = DefaultWindowSettings();
		desc.Position = locationSS;
		desc.StartPosition = WindowStartPosition::Manual;
		desc.Size = dpiSize;
		desc.Fullscreen = false;
		desc.HasBorder = false;
		desc.SupportsTransparency = false;
		desc.ShowInTaskbar = false;
		desc.ActivateWhenFirstShown = true;
		desc.AllowInput = true;
		desc.AllowMinimize = false;
		desc.AllowMaximize = false;
		desc.AllowDragAndDrop = false;
		desc.IsTopmost = true;
		desc.IsRegularWindow = false;
		desc.HasSizingFrame = false;
		OnWindowCreating(desc);
		_window = New<GraphicWindow>(desc);
		_window->GetFocusEvent.BindUnique<ContextMenuBase, &ContextMenuBase::OnWindowGotFocus>(this);
		_window->LostFocusEvent.BindUnique<ContextMenuBase, &ContextMenuBase::OnWindowLostFocus>(this);

		// Attach to the window
		_parentCM = TypeTryCast<ContextMenuBase>(parent);;
		Parent = _window->GetGUI();

		// Show
		Visible = true;
		if (_window == nullptr)
			return;
		_window->Show();
		PerformLayout();
		_previouslyFocused = parentWin->GetFocusedControl();
		Focus();
		OnShow();
	}

	void ContextMenuBase::Hide()
	{
		if (!Visible)
			return;

		// Lock update
		SetIsLayoutLocked(true);

		// Close child
		HideChild();

		// Unlink from window
		Parent = nullptr;

		// Close window
		if (_window != nullptr)
		{
			GraphicWindow* win = _window;
			_window = nullptr;
			win->Close();
		}

		// Unlink from parent
		if (_parentCM != nullptr)
		{
			_parentCM->_childCM = nullptr;
			_parentCM = nullptr;
		}

		// Return focus
		if (_previouslyFocused != nullptr)
		{
			if (_previouslyFocused->RootWindow() != nullptr)
			{
				_previouslyFocused->RootWindow()->Focus();
			}

			_previouslyFocused->Focus();
			_previouslyFocused = nullptr;
		}

		// Hide
		Visible = false;
		OnHide();
	}

	void ContextMenuBase::ShowChild(ContextMenuBase* child, Float2 location, bool isSubMenu)
	{
		// Hide current child
		HideChild();

		// Set child
		_childCM = child;
		_childCM->_parentCM = this;
		_childCM->_isSubMenu = isSubMenu;

		// Show child
		_childCM->Show(this, location);
	}

	void ContextMenuBase::HideChild()
	{
		if (_childCM != nullptr)
		{
			_childCM->Hide();
			_childCM = nullptr;
		}
	}

	void ContextMenuBase::Draw()
	{
		// Draw background
		Style* style = Style::Current;
		Rectangle bounds = Rectangle(Float2::Zero, Size);
		Render2D::FillRectangle(bounds, style->Background);
		Render2D::DrawRectangle(bounds, Color::Lerp(style->BackgroundSelected, style->Background, 0.6f));

		ContainerControl::Draw();
	}

	bool ContextMenuBase::OnMouseDown(Float2 location, MouseButton button)
	{
		ContainerControl::OnMouseDown(location, button);
		return true;
	}

	bool ContextMenuBase::OnMouseUp(Float2 location, MouseButton button)
	{
		ContainerControl::OnMouseUp(location, button);
		return true;
	}

	bool ContextMenuBase::OnKeyDown(KeyboardKeys key)
	{
		if (ContainerControl::OnKeyDown(key))
			return true;

		switch (key)
		{
		case KeyboardKeys::Escape:
			Hide();
			return true;
		}
		return false;
	}

	void ContextMenuBase::OnDestroy()
	{
		// Ensure to be hidden
		Hide();

		ContainerControl::OnDestroy();
	}

	void ContextMenuBase::UpdateWindowSize()
	{
		if (_window != nullptr)
		{
			_window->SetClientSize(Size.operator->() * _window->GetDpiScale());
		}
	}

	void ContextMenuBase::OnWindowLostFocus()
	{
		// Skip for parent menus (child should handle lost of focus)
		if (_childCM != nullptr)
			return;

		if (_parentCM != nullptr)
		{
			if (IsMouseOver)
				return;

			for (GraphicWindow* externalPopup : ExternalPopups)
			{
				if (externalPopup != nullptr && externalPopup->IsFocused())
					return;
			}

			// Check if mouse is over any of the parents
			ContextMenuBase* focusCM = nullptr;
			ContextMenuBase* cm = _parentCM;
			while (cm != nullptr)
			{
				if (cm->IsMouseOver)
					focusCM = cm;
				cm = cm->_parentCM;
			}

			if (focusCM != nullptr)
			{
				// Focus on the clicked parent and hide any open sub-menus
				focusCM->HideChild();
				if (focusCM->_window != nullptr)
				{
					focusCM->_window->Show();
				}
			}
			else
			{
				// User clicked outside the context menus, hide the whole context menu tree
				TopmostCM->Hide();
			}
		}
		else if (!IsMouseOver)
		{
			Hide();
		}
	}

	ContextMenuBase* ContextMenuBase::__GetTopmostCM()
	{
		ContextMenuBase* cm = this;
		while (cm->_parentCM != nullptr && cm->_isSubMenu)
		{
			cm = cm->_parentCM;
		}
		return cm;
	}

	bool ContextMenuBase::__GetIsMouseOver()
	{
		bool result = false;
		for (int i = 0; i < m_Children.Count(); i++)
		{
			Control* c = m_Children[i];
			if (c->Visible && c->IsMouseOver)
			{
				result = true;
				break;
			}
		}
		return result;
	}
} // SE