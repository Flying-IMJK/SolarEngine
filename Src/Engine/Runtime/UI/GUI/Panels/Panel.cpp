
#include "Panel.h"

#include "Core/Input/Input.h"
#include "Runtime/Render/2D/Render2D.h"

namespace SE
{
	Float2 Panel::GetViewBottom() const
	{
		return _viewOffset + Size;
	}

	void Panel::SetScrollBars(ScrollBars value)
	{
		if (_scrollBars == value)
			return;

		_scrollBars = value;

		if (((int)value & (int)ScrollBars::Vertical) == (int)ScrollBars::Vertical)
		{
			if (vScrollBar == nullptr)
				vScrollBar = GetChild<VScrollBar>();
			if (vScrollBar == nullptr)
			{
				vScrollBar = New<VScrollBar>(this, Width - _scrollBarsSize, Height);
				vScrollBar->AnchorPreset = AnchorPresets::TopLeft;

				//VScrollBar.X += VScrollBar.Width;
				vScrollBar->ValueChanged.BindUnique([this]()
				{
					SetViewOffset(Orientation::Vertical, vScrollBar->GetValue());
				});
			}
			if (vScrollBar != nullptr)
			{
				vScrollBar->TrackColor = _scrollbarTrackColor;
				vScrollBar->ThumbColor = _scrollbarThumbColor;
				vScrollBar->ThumbSelectedColor = _scrollbarThumbSelectedColor;
			}
		}
		else if (vScrollBar != nullptr)
		{
			vScrollBar->Dispose();
			vScrollBar = nullptr;
		}

		if (((int)value & (int)ScrollBars::Horizontal) == (int)ScrollBars::Horizontal)
		{
			if (hScrollBar == nullptr)
				hScrollBar = GetChild<HScrollBar>();
			if (hScrollBar == nullptr)
			{
				hScrollBar = New<HScrollBar>(this, Height - _scrollBarsSize, Width);
				hScrollBar->AnchorPreset = AnchorPresets::TopLeft;

				//HScrollBar.Y += HScrollBar.Height;
				//HScrollBar.Offsets += new Margin(0, 0, HScrollBar.Height * 0.5f, 0);
				hScrollBar->ValueChanged.BindUnique([this]()
				{
					SetViewOffset(Orientation::Horizontal, hScrollBar->GetValue());
				});
			}
			if (hScrollBar != nullptr)
			{
				hScrollBar->TrackColor = _scrollbarTrackColor;
				hScrollBar->ThumbColor = _scrollbarThumbColor;
				hScrollBar->ThumbSelectedColor = _scrollbarThumbSelectedColor;
			}
		}
		else if (hScrollBar != nullptr)
		{
			hScrollBar->Dispose();
			hScrollBar = nullptr;
		}

		PerformLayout();
	}

	void Panel::SetScrollBarsSize(float value)
	{
		if (Math::IsNearEqual(_scrollBarsSize, value))
			return;
		_scrollBarsSize = value;
		PerformLayout();
	}

	void Panel::SetAlwaysShowScrollbars(bool value)
	{
		if (_alwaysShowScrollbars != value)
		{
			_alwaysShowScrollbars = value;
			switch (_scrollBars)
			{
			case ScrollBars::None:
				break;
			case ScrollBars::Horizontal:
				hScrollBar->Visible = value;;
				break;
			case ScrollBars::Vertical:
				vScrollBar->Visible = value;;
				break;
			case ScrollBars::Both:
				hScrollBar->Visible = value;;
				vScrollBar->Visible = value;;
				break;
			default: break;
			}
			PerformLayout();
		}
	}

	void Panel::SetScrollMargin(Margin value)
	{
		if (_scrollMargin != value)
		{
			_scrollMargin = value;
			PerformLayout();
		}
	}

	void Panel::SetScrollbarTrackColor(Color value)
	{
		_scrollbarTrackColor = value;
		if (vScrollBar != nullptr)
			vScrollBar->TrackColor = _scrollbarTrackColor;
		if (hScrollBar != nullptr)
			hScrollBar->TrackColor = _scrollbarTrackColor;
	}

	void Panel::SetScrollbarThumbColor(Color value)
	{
		_scrollbarThumbColor = value;
		if (vScrollBar != nullptr)
			vScrollBar->ThumbColor = _scrollbarThumbColor;
		if (hScrollBar != nullptr)
			hScrollBar->ThumbColor = _scrollbarThumbColor;
	}

	void Panel::SetScrollbarThumbSelectedColor(Color value)
	{
		_scrollbarThumbSelectedColor = value;
		if (vScrollBar != nullptr)
			vScrollBar->ThumbSelectedColor = _scrollbarThumbSelectedColor;
		if (hScrollBar != nullptr)
			hScrollBar->ThumbSelectedColor = _scrollbarThumbSelectedColor;
	}

	Panel::Panel()
	{
	}

	Panel::Panel(ScrollBars scrollBars, bool autoFocus)
	{
		AutoFocus = autoFocus;
		// var style = Style.Current;
		// _scrollbarTrackColor = style.BackgroundHighlighted;
		// _scrollbarThumbColor = style.BackgroundNormal;
		// _scrollbarThumbSelectedColor = style.BackgroundSelected;
		this->SetScrollBars(scrollBars);
	}

	void Panel::SetViewOffset(Float2 value)
	{
		bool wasLocked = GetIsLayoutLocked();
		SetIsLayoutLocked(true);

		if (hScrollBar != nullptr)
			hScrollBar->SetValue(-value.x);
		if (vScrollBar != nullptr)
			vScrollBar->SetValue(-value.y);

		SetIsLayoutLocked(wasLocked);
		ScrollableControl::SetViewOffset(value);
	}

	void Panel::FastScroll()
	{
		if (hScrollBar != nullptr)
		{
			hScrollBar->FastScroll();
		}

		if (vScrollBar != nullptr)
		{
			vScrollBar->FastScroll();
		}
	}

	void Panel::ScrollViewTo(Control* c, bool fastScroll)
	{
		ENGINE_ASSERT(c != nullptr);

		Float2 location = c->Location;
		Float2 size = c->Size;
		while (c->HasParent() && c->Parent != this)
		{
			c = c->Parent;
			location = c->PointToParent(location);
		}

		if (c->HasParent())
		{
			ScrollViewTo(Rectangle(location, size), fastScroll);
		}
	}

	void Panel::ScrollViewTo(Float2 location, bool fastScroll)
	{
		ScrollViewTo(Rectangle(location, Float2::Zero), fastScroll);
	}

	void Panel::ScrollViewTo(Rectangle bounds, bool fastScroll)
	{
		bool wasLocked = GetIsLayoutLocked();
		SetIsLayoutLocked(true);

		if (hScrollBar != nullptr && hScrollBar->Enabled)
			hScrollBar->ScrollViewTo(bounds.GetLeft(), bounds.GetRight(), fastScroll);
		if (vScrollBar != nullptr && vScrollBar->Enabled)
			vScrollBar->ScrollViewTo(bounds.GetTop(), bounds.GetBottom(), fastScroll);

		SetIsLayoutLocked(wasLocked);
		PerformLayout();
	}

	void Panel::SetViewOffset(Orientation orientation, float value)
	{
		if (orientation == Orientation::Vertical)
			_viewOffset.y = -value;
		else
			_viewOffset.x = -value;
		OnViewOffsetChanged();
		PerformLayout();
	}

	bool Panel::OnMouseDown(Float2 location, MouseButton button)
	{
		if (ScrollableControl::OnMouseDown(location, button))
			return true;
		return AutoFocus && Focus(this);
	}

	bool Panel::OnMouseWheel(Float2 location, float delta)
	{
		// Base
		if (ScrollableControl::OnMouseWheel(location, delta))
			return true;

		if (Input::GetKey(KeyboardKeys::Shift))
		{
			if (hScrollBar != nullptr && hScrollBar->Enabled && hScrollBar->OnMouseWheel(hScrollBar->PointFromParent(location), delta))
				return true;
		}

		// Roll back to scroll bars
		if (vScrollBar != nullptr && vScrollBar->Enabled && vScrollBar->OnMouseWheel(vScrollBar->PointFromParent(location), delta))
			return true;

		// No event handled
		return false;
	}

	void Panel::RemoveChildren()
	{
		// Keep scroll bars alive
		if (vScrollBar != nullptr)
			m_Children.Remove(vScrollBar);
		if (hScrollBar != nullptr)
			m_Children.Remove(hScrollBar);

		ScrollableControl::RemoveChildren();

		// Restore scrollbars
		if (vScrollBar != nullptr)
			m_Children.Add(vScrollBar);
		if (hScrollBar != nullptr)
			m_Children.Add(hScrollBar);
		PerformLayout();
	}

	void Panel::DisposeChildren()
	{
		// Keep scrollbars alive
		if (vScrollBar != nullptr)
			m_Children.Remove(vScrollBar);
		if (hScrollBar != nullptr)
			m_Children.Remove(hScrollBar);

		ScrollableControl::DisposeChildren();

		// Restore scrollbars
		if (vScrollBar != nullptr)
			m_Children.Add(vScrollBar);
		if (hScrollBar != nullptr)
			m_Children.Add(hScrollBar);
		PerformLayout();
	}

	void Panel::OnChildResized(Control* control)
	{
		ScrollableControl::OnChildResized(control);

		if (control->IsScrollable)
		{
			PerformLayout();
		}
	}

	void Panel::Draw()
	{
		ScrollableControl::Draw();

		// Draw scrollbars manually (they are outside the clipping bounds)
		if (vScrollBar != nullptr && vScrollBar->Visible)
		{
			Render2D::PushTransform(vScrollBar->GetCachedTransform());
			vScrollBar->Draw();
			Render2D::PopTransform();
		}

		if (hScrollBar != nullptr && hScrollBar->Visible)
		{
			Render2D::PushTransform(hScrollBar->GetCachedTransform());
			hScrollBar->Draw();
			Render2D::PopTransform();
		}
	}

	bool Panel::ContainsPoint(Float2& location, bool precise)
	{
		if (precise && BackgroundColor.a <= 0.0f) // Go through transparency
			return false;
		return ScrollableControl::ContainsPoint(location, precise);
	}

	bool Panel::IntersectsChildContent(Control* child, Float2 location, Float2& childSpaceLocation)
	{
		// For not scroll bars we want to reject any collisions
		if (child != vScrollBar && child != hScrollBar)
		{
			// Check if has v scroll bar to reject points on it
			if (vScrollBar != nullptr && vScrollBar->Enabled)
			{
				Float2 pos = vScrollBar->PointFromParent(location);
				if (vScrollBar->ContainsPoint(pos))
				{
					childSpaceLocation = Float2::Zero;
					return false;
				}
			}

			// Check if has h scroll bar to reject points on it
			if (hScrollBar != nullptr && hScrollBar->Enabled)
			{
				Float2 pos = hScrollBar->PointFromParent(location);
				if (hScrollBar->ContainsPoint(pos))
				{
					childSpaceLocation = Float2::Zero;
					return false;
				}
			}
		}

		return ScrollableControl::IntersectsChildContent(child, location, childSpaceLocation);
	}

	void Panel::AddChildInternal(Control* child)
	{
		ScrollableControl::AddChildInternal(child);

		if (child->IsScrollable)
		{
			PerformLayout();
		}
	}

	void Panel::PerformLayout(bool force)
	{
		if (_layoutUpdateLock > 2)
			return;
		_layoutUpdateLock++;

		if (!GetIsLayoutLocked())
		{
			_layoutChanged = false;
		}

		ScrollableControl::PerformLayout(force);

		if (!GetIsLayoutLocked() && _layoutChanged)
		{
			_layoutChanged = false;
			PerformLayout(true);
		}

		_layoutUpdateLock--;
	}

	Rectangle Panel::GetDesireClientArea()
	{
		Rectangle rect = Rectangle(Float2::Zero, Size);

		if (vScrollBar != nullptr && vScrollBar->Visible)
		{
			rect.Size.x -= vScrollBar->Width;
		}

		if (hScrollBar != nullptr && hScrollBar->Visible)
		{
			rect.Size.y -= hScrollBar->Height;
		}

		return rect;
	}

	DragDropEffect Panel::OnDragMove(const Float2& location, DragData* data)
	{
		DragDropEffect result = ScrollableControl::OnDragMove(location, data);

		float width = Width;
		float height = Height;
		float MinSize = 70;
		float AreaSize = 25;
		float MoveScale = 4.0f;
		Float2 viewOffset = -_viewOffset;

		if (vScrollBar != nullptr && vScrollBar->Enabled && height > MinSize)
		{
			if (Rectangle(0, 0, width, AreaSize).Contains(location))
			{
				viewOffset.y -= MoveScale;
			}
			else if (Rectangle(0, height - AreaSize, width, AreaSize).Contains(location))
			{
				viewOffset.y += MoveScale;
			}

			viewOffset.y = Math::Clamp(viewOffset.y, vScrollBar->GetMinimum(), vScrollBar->GetMaximum());
			vScrollBar->SetValue(viewOffset.y);
		}

		if (hScrollBar != nullptr && hScrollBar->Enabled && width > MinSize)
		{
			if (Rectangle(0, 0, AreaSize, height).Contains(location))
			{
				viewOffset.x -= MoveScale;
			}
			else if (Rectangle(width - AreaSize, 0, AreaSize, height).Contains(location))
			{
				viewOffset.x += MoveScale;
			}

			viewOffset.x = Math::Clamp(viewOffset.x, hScrollBar->GetMinimum(), hScrollBar->GetMaximum());
			hScrollBar->SetValue(viewOffset.x);
		}

		viewOffset *= -1;

		if (viewOffset != _viewOffset)
		{
			_viewOffset = viewOffset;
			OnViewOffsetChanged();
			PerformLayout();
		}

		return result;
	}

	void Panel::PerformLayoutBeforeChildren()
	{
		// Arrange controls and get scroll bounds
		ArrangeAndGetBounds();

		// Update scroll bars
		Rectangle controlsBounds = _controlsBounds;
		Rectangle scrollBounds = controlsBounds;
		_scrollMargin.ExpandRectangle(scrollBounds);
		if (vScrollBar != nullptr)
		{
			float height = Height;
			bool vScrollEnabled = (controlsBounds.GetBottom() > height + 0.01f || controlsBounds.GetY() < 0.0f) && height > _scrollBarsSize;

			if (vScrollBar->Enabled != vScrollEnabled)
			{
				// Set scroll bar visibility
				vScrollBar->Enabled = vScrollEnabled;
				vScrollBar->Visible = vScrollEnabled || _alwaysShowScrollbars;;
				_layoutChanged = true;

				// Clear scroll state
				vScrollBar->Reset();
				_viewOffset.y = 0;
				OnViewOffsetChanged();

				// Get the new bounds after changing scroll
				ArrangeAndGetBounds();
			}

			if (vScrollEnabled)
			{
				vScrollBar->SetScrollRange(scrollBounds.GetTop(), Math::Max(Math::Max(0.0f, scrollBounds.GetTop()), scrollBounds.GetHeight() - height));
			}
			vScrollBar->Bounds = Rectangle(Width - _scrollBarsSize, 0, _scrollBarsSize, Height);
		}
		if (hScrollBar != nullptr)
		{
			float width = Width;
			bool hScrollEnabled = controlsBounds.GetRight() > width + 0.01f || controlsBounds.GetX() < 0.0f && width > _scrollBarsSize;

			if (hScrollBar->Enabled != hScrollEnabled)
			{
				// Set scroll bar visibility
				hScrollBar->Enabled = hScrollEnabled;
				hScrollBar->Visible = hScrollEnabled || _alwaysShowScrollbars;;
				_layoutChanged = true;

				// Clear scroll state
				hScrollBar->Reset();
				_viewOffset.x = 0;
				OnViewOffsetChanged();

				// Get the new bounds after changing scroll
				ArrangeAndGetBounds();
			}

			if (hScrollEnabled)
			{
				hScrollBar->SetScrollRange(scrollBounds.GetLeft(), Math::Max(Math::Max(0.0f, scrollBounds.GetLeft()), scrollBounds.GetWidth() - width));
			}
			hScrollBar->Bounds = Rectangle(0, Height - _scrollBarsSize, Width - (vScrollBar != nullptr && vScrollBar->Visible ? vScrollBar->Width : 0), _scrollBarsSize);
		}
	}
	
	void Panel::ArrangeAndGetBounds()
	{
		Arrange();

		// Calculate scroll area bounds
		Float2 totalMin = Float2::Zero;
		Float2 totalMax = Float2::Zero;
		for (int i = 0; i < m_Children.Count(); i++)
		{
			Control* c = m_Children[i];
			if (c->Visible && c->IsScrollable)
			{
				Float2 min = Float2::Zero;
				Float2 max = c->Size;
				Matrix3x3::Transform2DPoint(min, c->GetCachedTransform(), min);
				Matrix3x3::Transform2DPoint(max, c->GetCachedTransform(), max);
				Float2::Min(min, totalMin, totalMin);
				Float2::Max(max, totalMax, totalMax);
			}
		}

		// Cache result
		_controlsBounds = Rectangle(totalMin, totalMax - totalMin);
	}
	
	void Panel::Arrange()
	{
		ScrollableControl::PerformLayoutBeforeChildren();
	}
} // SE