
#include "LabelElement.h"

#include "Core/Input/Input.h"
#include "Core/Platform/Clipboard.h"
#include "Editor/GUI/ContextMenu/ContextMenu.h"
#include "Editor/GUI/ContextMenu/ContextMenuButton.h"
#include "Runtime/UI/GUI/Common/ClickableLabel.h"

namespace SE::Editor
{
	LabelElement::LabelElement()
	{
		Label = New<ClickableLabel>();
		Label->Size = Float2(100, 18);
		Label->HorizontalAlignment = TextAlignment::Near;
		// TODO: auto height for label
	}

	LabelElement* LabelElement::AddCopyContextMenu(Delegate<ContextMenu*> customOptions)
	{
		Label->RightClick.BindUnique<LabelElement, &LabelElement::OnRightClick>(this);
		m_CustomContextualOptions = customOptions;
		return this;
	}

	Control* LabelElement::GetControl()
	{
		return Label;
	}

	void LabelElement::OnRightClick()
	{
		ContextMenu* menu = New<ContextMenu>();
		menu->AddButton(SE_TEXT("Copy text"))->Clicked.BindUnique<LabelElement, &LabelElement::OnCopyText>(this);
		if (m_CustomContextualOptions.IsBinded())
		{
			m_CustomContextualOptions(menu);
		}

		menu->Show(Label, Label->PointFromScreen(Input::GetMouseScreenPosition()));
	}

	void LabelElement::OnCopyText()
	{
		Clipboard::SetText(Label->Text.Get());
	}
} // SE