#include "PropertyNameLabel.h"
#include "../CustomEditor.h"
#include "../CustomEditorPresenter.h"
#include "Core/Logging/Logger.h"

namespace SE::Editor
{
	// ============================================================================
	// PropertyNameLabel
	// ============================================================================

	PropertyNameLabel::PropertyNameLabel(const String& name)
	{
		// TODO: Set text property when Label functionality is available
		// Text = name;
		// HorizontalAlignment = TextAlignment::Near;
		// VerticalAlignment = TextAlignment::Center;
		// Margin = Margin(4, 0, 0, 0);
		// ClipText = true;
		
		m_Name = name;
		HighlightStripColor = Colors::Transparent;
	}

	void PropertyNameLabel::LinkEditor(CustomEditor* editor)
	{
		if (LinkedEditor == nullptr && editor != nullptr)
		{
			LinkedEditor = editor;
			editor->LinkedLabel = this;
		}
	}

	void PropertyNameLabel::Draw()
	{
		// Call base draw
		Control::Draw();

		// Draw highlight strip on the left side
		if (HighlightStripColor.A > 0.0f)
		{
			// TODO: Implement when Render2D is available
			// Render2D::FillRectangle(Rectangle(0, 0, 2, Height), HighlightStripColor);
		}
	}

	void PropertyNameLabel::OnMouseLeave()
	{
		m_mouseDown = false;
		Control::OnMouseLeave();
	}

	bool PropertyNameLabel::OnMouseDown(const Float2& location, MouseButton button)
	{
		if (button == MouseButton::Right)
		{
			m_mouseDown = true;
		}

		return Control::OnMouseDown(location, button);
	}

	bool PropertyNameLabel::OnMouseUp(const Float2& location, MouseButton button)
	{
		if (Control::OnMouseUp(location, button))
			return true;

		if (m_mouseDown && button == MouseButton::Right)
		{
			m_mouseDown = false;

			// Skip if not extended
			if (LinkedEditor == nullptr && !SetupContextMenu.IsBound())
				return false;

			ShowContextMenu(location);
			return true;
		}

		return false;
	}

	void PropertyNameLabel::OnLostFocus()
	{
		m_mouseDown = false;
		Control::OnLostFocus();
	}

	void PropertyNameLabel::ShowContextMenu(const Float2& location)
	{
		// TODO: Implement context menu when ContextMenu class is available
		// For now, just log
		LOG_INFO("PropertyNameLabel::ShowContextMenu - Context menu requested at ({}, {})", location.X, location.Y);

		// Create context menu
		// ContextMenu* menu = New<ContextMenu>();

		if (LinkedEditor != nullptr)
		{
			// TODO: Add menu items when ContextMenu is available
			/*
			auto features = LinkedEditor->GetPresenter()->Features;
			
			if ((features & (FeatureFlags::UseDefault | FeatureFlags::UsePrefab)) != FeatureFlags::None)
			{
				if ((features & FeatureFlags::UsePrefab) != FeatureFlags::None)
				{
					auto* revertToPrefab = menu->AddButton("Revert to Prefab", [this]() {
						LinkedEditor->RevertToReferenceValue();
					});
					revertToPrefab->Enabled = LinkedEditor->CanRevertReferenceValue();
				}
				
				if ((features & FeatureFlags::UseDefault) != FeatureFlags::None)
				{
					auto* resetToDefault = menu->AddButton("Reset to default", [this]() {
						LinkedEditor->RevertToDefaultValue();
					});
					resetToDefault->Enabled = LinkedEditor->CanRevertDefaultValue();
				}
				
				menu->AddSeparator();
			}
			
			menu->AddButton("Copy", [this]() {
				LinkedEditor->Copy();
			});
			
			auto* paste = menu->AddButton("Paste", [this]() {
				LinkedEditor->Paste();
			});
			paste->Enabled = LinkedEditor->CanPaste();
			*/
		}

		// Fire custom setup event
		// SetupContextMenu.Invoke(this, menu, LinkedEditor);

		// Show menu
		// menu->Show(this, location);
	}

	// ============================================================================
	// ClickablePropertyNameLabel
	// ============================================================================

	ClickablePropertyNameLabel::ClickablePropertyNameLabel(const String& name)
		: PropertyNameLabel(name)
	{
	}

	bool ClickablePropertyNameLabel::OnMouseUp(const Float2& location, MouseButton button)
	{
		// Fire events
		if (button == MouseButton::Left)
		{
			if (MouseLeftClick.IsBound())
			{
				MouseLeftClick.Invoke(this, location);
				return true;
			}
		}
		else if (button == MouseButton::Right)
		{
			if (MouseRightClick.IsBound())
			{
				MouseRightClick.Invoke(this, location);
				return true;
			}
		}

		return PropertyNameLabel::OnMouseUp(location, button);
	}

	bool ClickablePropertyNameLabel::OnMouseDoubleClick(const Float2& location, MouseButton button)
	{
		// Fire events
		if (button == MouseButton::Left)
		{
			if (MouseLeftDoubleClick.IsBound())
			{
				MouseLeftDoubleClick.Invoke(this, location);
				return true;
			}
		}
		else if (button == MouseButton::Right)
		{
			if (MouseRightDoubleClick.IsBound())
			{
				MouseRightDoubleClick.Invoke(this, location);
				return true;
			}
		}

		return PropertyNameLabel::OnMouseDoubleClick(location, button);
	}

	// ============================================================================
	// DraggablePropertyNameLabel
	// ============================================================================

	DraggablePropertyNameLabel::DraggablePropertyNameLabel(const String& name)
		: PropertyNameLabel(name)
	{
	}

	bool DraggablePropertyNameLabel::OnMouseDown(const Float2& location, MouseButton button)
	{
		if (button == MouseButton::Left)
		{
			m_isDragging = true;
			m_dragStartPos = location;
			
			if (DragStart.IsBound())
			{
				DragStart.Invoke(this, location);
			}
		}

		return PropertyNameLabel::OnMouseDown(location, button);
	}

	void DraggablePropertyNameLabel::OnMouseMove(const Float2& mousePos)
	{
		if (m_isDragging && DragMove.IsBound())
		{
			DragMove.Invoke(this, mousePos);
		}

		Control::OnMouseMove(mousePos);
	}

	bool DraggablePropertyNameLabel::OnMouseUp(const Float2& location, MouseButton button)
	{
		if (button == MouseButton::Left && m_isDragging)
		{
			m_isDragging = false;
			
			if (DragEnd.IsBound())
			{
				DragEnd.Invoke(this);
			}
			
			return true;
		}

		return PropertyNameLabel::OnMouseUp(location, button);
	}

	// ============================================================================
	// CheckablePropertyNameLabel
	// ============================================================================

	CheckablePropertyNameLabel::CheckablePropertyNameLabel(const String& name)
		: PropertyNameLabel(name)
	{
	}

	void CheckablePropertyNameLabel::Draw()
	{
		PropertyNameLabel::Draw();

		// TODO: Draw checkbox when Render2D is available
		/*
		Rectangle checkboxRect(4, (Height - 16) / 2, 16, 16);
		
		// Draw checkbox background
		Render2D::FillRectangle(checkboxRect, Checked ? Color::Green : Color::Gray);
		
		// Draw checkbox border
		Render2D::DrawRectangle(checkboxRect, Color::Black);
		
		// Draw checkmark if checked
		if (Checked)
		{
			// Draw checkmark
		}
		*/
	}

	bool CheckablePropertyNameLabel::OnMouseUp(const Float2& location, MouseButton button)
	{
		if (button == MouseButton::Left)
		{
			// Toggle checked state
			Checked = !Checked;
			
			if (CheckedChanged.IsBound())
			{
				CheckedChanged.Invoke(this, Checked);
			}
			
			return true;
		}

		return PropertyNameLabel::OnMouseUp(location, button);
	}

} // namespace SE::Editor
