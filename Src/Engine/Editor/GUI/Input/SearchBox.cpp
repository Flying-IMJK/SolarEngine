
#include "SearchBox.h"

#include "Editor/EditorIcons.h"
#include "Runtime/UI/GUI/Style.h"
#include "Runtime/UI/GUI/Brushes/SpriteBrush.h"
#include "Runtime/UI/GUI/Common/Button.h"

namespace SE::Editor
{
	SearchBox::SearchBox() : TextBox(false, 0, 0), ClearSearchButton(nullptr)
	{
	}

	SearchBox::SearchBox(bool isMultiline, float x, float y, float width): TextBox(isMultiline, x, y, width)
	{
		WatermarkText = SE_TEXT("Search...");

		ClearSearchButton = New<Button>();
		ClearSearchButton->Parent = this;
		ClearSearchButton->Width = 14.0f;
		ClearSearchButton->Height = 14.0f;
		ClearSearchButton->AnchorPreset = AnchorPresets::TopRight;
		ClearSearchButton->Text = SE_TEXT("");
		ClearSearchButton->TooltipText = SE_TEXT("Cancel Search.");
		ClearSearchButton->BackgroundColor = TextColor;
		ClearSearchButton->BorderColor = Colors::Transparent;
		ClearSearchButton->BackgroundColorHighlighted = Style::Current->ForegroundGrey;
		ClearSearchButton->BorderColorHighlighted = Colors::Transparent;
		ClearSearchButton->BackgroundColorSelected = Style::Current->ForegroundGrey;
		ClearSearchButton->BorderColorSelected = Colors::Transparent;
		ClearSearchButton->BackgroundBrush = New<SpriteBrush>(EditorIcons::Cross16);
		ClearSearchButton->Visible = false,

		ClearSearchButton->LocalY = ClearSearchButton->LocalY + 2;
		ClearSearchButton->LocalX = ClearSearchButton->LocalX - 2;
		ClearSearchButton->Clicked.Bind([this]
		{
			Clear();
		});
		ClearSearchButton->HoverBegin.Bind([this]
		{
			_changeCursor = false;
			Cursor = CursorType::Default;
		});
		ClearSearchButton->HoverEnd.Bind([this]
		{
			_changeCursor = true;
		});

		TextChanged.Bind([this]
		{
			ClearSearchButton->Visible = !Text.operator->().IsEmpty();
		});
	}
} // SE

