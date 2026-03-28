
#include "RenamePopup.h"

#include "Core/Input/Input.h"
#include "Runtime/UI/GUI/RootControl.h"
#include "Runtime/UI/GUI/Style.h"
#include "Runtime/UI/GUI/WindowRootControl.h"
#include "Runtime/UI/GUI/Common/TextBox.h"

namespace SE::Editor
{
	RenamePopup::RenamePopup(String& value, Float2 size, bool isMultiline)
	{
		if (!isMultiline)
			size.y = TextBox::DefaultHeight;
		Size = size;

		InitialValue = value;

		_inputField = new TextBox(isMultiline, 0, 0, size.y);
		_inputField->TextChanged.Bind<RenamePopup, &RenamePopup::OnTextChanged>(this);
		_inputField->AnchorPreset = AnchorPresets::StretchAll;
		_inputField->Offsets = Margin::Zero;
		_inputField->Text = InitialValue;
		_inputField->Parent = this;
	}

	void RenamePopup::Update(float deltaTime)
	{
		Float2 mouseLocation = Root->GetMousePosition();
		if (!ContainsPoint(mouseLocation) && RootWindow()->ContainsFocus() && Text != InitialValue)
		{
			// rename item before closing if left mouse button in clicked
			if (Input::GetMouseButtonDown(MouseButton::Left))
				OnEnd();
		}

		ContextMenuBase::Update(deltaTime);
	}

	RenamePopup* RenamePopup::ShowPopup(Control* control, Rectangle area, String& value, bool isMultiline)
	{
		// Calculate the control size in the window space to handle scaled controls
		Float2 upperLeft = control->PointToWindow(area.GetUpperLeft());
		Float2 bottomRight = control->PointToWindow(area.GetBottomRight());
		Float2 size = bottomRight - upperLeft;

		RenamePopup* rename = New<RenamePopup>(value, size, isMultiline);
		Float2 pos = area.Location + Float2(0, (size.y - rename->Height) * 0.5f);
		rename->Show(control, pos);
		return rename;
	}

	bool RenamePopup::OnKeyDown(KeyboardKeys key)
	{
		// Enter
		if (key == KeyboardKeys::Return)
		{
			OnEnd();
			return true;
		}
		// Esc
		if (key == KeyboardKeys::Escape)
		{
			Hide();
			return true;
		}

		// Base
		return ContextMenuBase::OnKeyDown(key);
	}

	void RenamePopup::OnDestroy()
	{
		Renamed.Unbind();
		Closed.Unbind();
		Validate.Unbind();
		_inputField = nullptr;

		ContextMenuBase::OnDestroy();
	}

	void RenamePopup::OnShow()
	{
		_inputField->EndEditOnClick = false; // Ending edit is handled through popup
		_inputField->Focus();
		_inputField->SelectAll();

		ContextMenuBase::OnShow();
	}

	void RenamePopup::OnHide()
	{
		Closed(this);
		Closed.Unbind();

		ContextMenuBase::OnHide();

		// Remove itself
		Dispose();
	}

	void RenamePopup::OnEnd()
	{
		String& text = Text;
		if (text != InitialValue && GetIsInputValid())
		{
			Renamed(this);
		}

		Hide();
	}

	void RenamePopup::OnTextChanged()
	{
		if (!Validate.IsBinded())
			return;

		bool valid = GetIsInputValid();
		Style* style = Style::Current;
		if (valid)
		{
			_inputField->BorderColor = Colors::Transparent;
			_inputField->BorderSelectedColor = style->BackgroundSelected;
		}
		else
		{
			Color color = Color(1.0f, 0.0f, 0.02745f, 1.0f);
			_inputField->BorderColor = Color::Lerp(color, style->TextBoxBackground, 0.6f);
			_inputField->BorderSelectedColor = color;
		}
	}

	bool RenamePopup::GetIsInputValid()
	{
		return !_inputField->Text.operator->().IsEmpty() && (_inputField->Text == InitialValue || !Validate.IsBinded() || Validate(this, _inputField->Text));
	}

	String& RenamePopup::__GetText()
	{
		return _inputField->Text;
	}

	void RenamePopup::__SetText(String& value)
	{
		_inputField->Text = value;
	}
} // SE