

#include "ToolStrip.h"

#include "ToolStripButton.h"
#include "ToolStripSeparator.h"
#include "Editor/Windows/EditorWindow.h"
#include "Runtime/UI/GUI/Style.h"

namespace SE::Editor
{

	ToolStrip::ToolStrip(float height, float y)
	{
		AutoFocus = false;
		AnchorPreset = AnchorPresets::HorizontalStretchTop;
		BackgroundColor = Style::Current->LightBackground;
		Offsets = Margin(0, 0, y, height);
	}
	
	ToolStripButton* ToolStrip::AddButton(SpriteHandle sprite, Function<void()> onClick)
	{
		ToolStripButton* button = New<ToolStripButton>(ItemsHeight, sprite);
		button->Parent = this;

		if (onClick.IsBinded())
			button->Clicked.BindUnique(onClick);
		return button;
	}
	
	ToolStripButton* ToolStrip::AddButton(SpriteHandle sprite, String text, Function<void()> onClick)
	{
		ToolStripButton* button = New<ToolStripButton>(ItemsHeight, sprite);
		button->Text = text;
		button->Parent = this;

		if (onClick.IsBinded())
			button->Clicked.BindUnique(onClick);
		return button;
	}

	ToolStripButton* ToolStrip::AddButton(String text, Function<void()> onClick)
	{
		ToolStripButton* button = New<ToolStripButton>(ItemsHeight, SpriteHandle::Invalid);
		button->Text = text;
		button->Parent = this;

		if (onClick.IsBinded())
			button->Clicked.BindUnique(onClick);
		return button;
	}

	ToolStripSeparator* ToolStrip::AddSeparator()
	{
		return AddChild(New<ToolStripSeparator>(ItemsHeight));
	}

	void ToolStrip::OnChildResized(Control* control)
	{
		ContainerControl::OnChildResized(control);

		PerformLayout();
	}
	
	bool ToolStrip::OnKeyDown(KeyboardKeys key)
	{
		if (ContainerControl::OnKeyDown(key))
			return true;

		// Fallback to the owning window for shortcuts
		EditorWindow* editorWindow = nullptr;
		ContainerControl* c = Parent;
		while (c != nullptr && editorWindow == nullptr)
		{
			editorWindow = TypeTryCast<EditorWindow>(c);
			c = c->Parent;
		}
		return false;
		/*var editor = Editor.Instance;
		if (editorWindow == null)
			editorWindow = editor.Windows.EditWin; // Fallback to main editor window
		return editorWindow.InputActions.Process(editor, this, key);*/
	}

	void ToolStrip::PerformLayoutBeforeChildren()
	{
		// Arrange controls
		float x = DefaultMarginH;
		float h = ItemsHeight;
		for (int i = 0; i < m_Children.Count(); i++)
		{
			Control* c = m_Children[i];
			if (c->Visible)
			{
				float w = c->Width;
				c->Bounds = Rectangle(x, DefaultMarginV, w, h);
				x += w + DefaultMarginH;
			}
		}
	}

	ToolStripButton* ToolStrip::__GetLastButton()
	{
		for (int i = m_Children.Count() - 1; i >= 0; i--)
		{
			ToolStripButton* button = TypeTryCast<ToolStripButton>(m_Children[i]);
			if (button != nullptr)
				return button;
		}
		return nullptr;
	}

	int ToolStrip::__GetButtonsCount()
	{
		int result = 0;
		for (int i = 0; i < m_Children.Count(); i++)
		{
			if (TypeAs<ToolStripButton>(m_Children[i]))
				result++;
		}
		return result;
	}

	float ToolStrip::__GetItemsHeight()
	{
		return Height - 2 * DefaultMarginV;
	}
} // SE