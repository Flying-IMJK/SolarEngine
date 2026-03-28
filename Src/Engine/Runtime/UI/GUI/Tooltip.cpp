

#include "Tooltip.h"

#include "RootControl.h"
#include "Style.h"
#include "WindowRootControl.h"
#include "Core/Input/Input.h"
#include "Core/Input/Mouse.h"
#include "Runtime/Graphics/GraphicWindow.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/Render/2D/TextLayoutOptions.h"

namespace SE
{
	void Tooltip::UpdateWindowSize()
	{
		if (m_Window)
		{
			m_Window->SetClientSize(Size.operator->() * m_Window->GetDpiScale());
		}
	}
	
	void Tooltip::WrapPosition(Float2 locationSS, float flipOffset)
	{
		if (m_ShowTarget == nullptr || m_ShowTarget->RootWindow() == nullptr)
			return;

		// Calculate popup direction
		float dpiScale = m_ShowTarget->RootWindow()->GetDpiScale();
		Float2 dpiSize = Size.operator->() * dpiScale;
		Rectangle monitorBounds = Platform::GetMonitorBounds(locationSS);
		Float2 rightBottomMonitorBounds = monitorBounds.GetBottomRight();
		Float2 rightBottomLocationSS = locationSS + dpiSize;

		// Prioritize tooltip placement within parent window, fall back to virtual desktop
		if (rightBottomMonitorBounds.y < rightBottomLocationSS.y)
		{
			// Direction: up
			locationSS.y -= dpiSize.y + flipOffset;
		}
		if (rightBottomMonitorBounds.x < rightBottomLocationSS.x)
		{
			// Direction: left
			locationSS.x -= dpiSize.x + flipOffset * 2;
		}
	}

	Tooltip::Tooltip() : ContainerControl(0, 0, 300, 24)
	{
		Visible = false;;
		AutoFocus = false;
	}

	void Tooltip::Show(Control* target, Float2 location, Rectangle targetArea)
	{
		ENGINE_ASSERT(target != nullptr);

		// Ensure to be closed
		Hide();

		// Block showing tooltips when application is not focused
		if (!Platform::GetHasFocus())
			return;

		// Unlock and perform controls update
		UnlockChildrenRecursive();
		PerformLayout();

		// Calculate popup direction and initial location
		RootControl* parentWin = target->Root;
		if (parentWin == nullptr)
		{
			return;
		}
		float dpiScale = target->RootWindow()->GetDpiScale();
		Float2 dpiSize = Size.operator->() * dpiScale;
		Float2 locationWS = target->PointToWindow(location);
		Float2 locationSS = parentWin->PointToScreen(locationWS);
		m_ShowTarget = target;
		WrapPosition(locationSS);

		// Create window
		CreateWindowSettings desc = DefaultWindowSettings();
		desc.StartPosition = WindowStartPosition::Manual;
		desc.Position = locationSS;
		desc.Size = dpiSize;
		desc.Fullscreen = false;
		desc.HasBorder = false;
		desc.SupportsTransparency = false;
		desc.ShowInTaskbar = false;
		desc.ActivateWhenFirstShown = false;
		desc.AllowInput = false;
		desc.AllowMinimize = false;
		desc.AllowMaximize = false;
		desc.AllowDragAndDrop = false;
		desc.IsTopmost = true;
		desc.IsRegularWindow = false;
		desc.HasSizingFrame = false;
		desc.ShowAfterFirstPaint = true;
		m_Window = New<GraphicWindow>(desc);
		ENGINE_ASSERT_M(m_Window != nullptr, SE_TEXT("Failed to create tooltip window."));

		// Attach to the window and focus
		Parent = m_Window->GetGUI();
		Visible = true;;
		m_Window->Show();
		m_ShowTarget->OnTooltipShown(this);
	}
	
	void Tooltip::Hide()
	{
		if (!Visible)
			return;

		// Unlink
		SetIsLayoutLocked(true);
		Parent = nullptr;

		// Close window
		if (m_Window)
		{
			GraphicWindow* win = m_Window;
			m_Window = nullptr;
			win->Close();
		}

		// Hide
		Visible = false;;
	}
	
	void Tooltip::OnMouseEnterControl(Control* target)
	{
		m_LastTarget = target;
		m_TimeToPopupLeft = TimeToShow;
	}
	
	void Tooltip::OnMouseOverControl(Control* target, float dt)
	{
		if (!Visible && m_TimeToPopupLeft > 0.0f)
		{
			m_LastTarget = target;
			m_TimeToPopupLeft -= dt;

			if (m_TimeToPopupLeft <= 0.0f)
			{
				Float2 location;
				Rectangle area;
				if (m_LastTarget->OnShowTooltip(m_CurrentText, location, area))
				{
					Show(m_LastTarget, location, area);
				}
			}
		}
	}
	
	void Tooltip::OnMouseLeaveControl(Control* target)
	{
		if (Visible)
			Hide();
		m_LastTarget = nullptr;
	}
	
	void Tooltip::Update(float deltaTime)
	{
		Float2 mousePos = Input::Mouse->GetPosition();
		Float2 location = m_ShowTarget->PointFromScreen(mousePos);
		if (!m_ShowTarget->OnTestTooltipOverControl(location))
		{
			// Auto hide if mouse leaves control area
			Hide();
		}
		else
		{
			// Position tooltip when mouse moves
			WrapPosition(mousePos, 10);
			if (m_Window)
				m_Window->SetPosition(mousePos + Float2(15, 10));
		}

		ContainerControl::Update(deltaTime);
	}
	
	void Tooltip::Draw()
	{
		Style* style = Style::Current;

		// Background
		Render2D::FillRectangle(Rectangle(Float2::Zero, Size), Color::Lerp(style->BackgroundSelected, style->Background, 0.6f));
		Render2D::FillRectangle(Rectangle(1.1f, 1.1f, Width - 2, Height - 2), style->Background);

		// Padding for text
		Rectangle textRect = GetClientArea();
		textRect.Location.x += 5;
		textRect.Size.x -= 10;

		TextLayoutOptions layout;
		layout.Bounds = textRect;
		layout.HorizontalAlignment = TextAlignment::Center;
		layout.VerticalAlignment = TextAlignment::Center;
		layout.TextWrapping = TextWrapping::WrapWords;
		layout.Scale = 1.0f;
		layout.BaseLinesGapScale = 1.0f;

		// Tooltip text
		Render2D::RenderText(style->FontMedium, m_CurrentText, style->Foreground, layout);
	}
	
	bool Tooltip::OnShowTooltip(String text, Float2& location, Rectangle& area)
	{
		ContainerControl::OnShowTooltip(text, location, area);

		// It's better not to show tooltip for a tooltip.
		// It would be kind of tooltipness.
		return false;
	}
	
	void Tooltip::OnDestroy()
	{
		Hide();

		ContainerControl::OnDestroy();
	}
	
	void Tooltip::PerformLayoutBeforeChildren()
	{
		ContainerControl::PerformLayoutBeforeChildren();

		Float2 prevSize = Size;
		Style* style = Style::Current;

		// Calculate size of the tooltip
		Float2 size = Float2::Zero;
		if (style != nullptr && style->FontMedium && m_CurrentText.Length() > 0)
		{
			TextLayoutOptions layout = TextLayoutOptions::Default();
			layout.Bounds = Rectangle(0, 0, MaxWidth, 10000000);
			layout.HorizontalAlignment = TextAlignment::Center;
			layout.VerticalAlignment = TextAlignment::Center;
			layout.TextWrapping = TextWrapping::WrapWords;
			auto items = style->FontMedium->ProcessText(m_CurrentText, layout);
			for (int i = 0; i < items.Count(); i++)
			{
				auto item = &items[i];
				size.x = Math::Max(size.x, item->Size.x + 8.0f);
				size.y += item->Size.y;
			}
			//size.X += style.FontMedium.MeasureText(_currentText).X;
		}
		Size = size + Float2(24.0f);

		// Check if is visible size get changed
		if (Visible && prevSize != Size)
		{
			UpdateWindowSize();
		}
	}
} // SE