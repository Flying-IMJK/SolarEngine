#include "Control.h"

#include "ContainerControl.h"
#include "RootControl.h"
#include "Style.h"
#include "WindowRootControl.h"
#include "Panels/Panel.h"
#include "Runtime/Graphics/GraphicWindow.h"
#include "Runtime/Render/2D/Render2D.h"

namespace SE
{

	Control::AnchorPresetData Control::anchorPresetsData[16] =
	{
	    AnchorPresetData(AnchorPresets::TopLeft,					Float2(0, 0),		Float2(0, 0)),
	    AnchorPresetData(AnchorPresets::TopCenter,					Float2(0.5f, 0),	Float2(0.5f, 0)),
	    AnchorPresetData(AnchorPresets::TopRight,					Float2(1, 0),		Float2(1, 0)),

	    AnchorPresetData(AnchorPresets::MiddleLeft,					Float2(0, 0.5f),	Float2(0, 0.5f)),
	    AnchorPresetData(AnchorPresets::MiddleCenter,				Float2(0.5f, 0.5f),	Float2(0.5f, 0.5f)),
	    AnchorPresetData(AnchorPresets::MiddleRight,				Float2(1, 0.5f),	Float2(1, 0.5f)),

	    AnchorPresetData(AnchorPresets::BottomLeft,					Float2(0, 1),		Float2(0, 1)),
	    AnchorPresetData(AnchorPresets::BottomCenter,				Float2(0.5f, 1),	Float2(0.5f, 1)),
	    AnchorPresetData(AnchorPresets::BottomRight,				Float2(1, 1),		Float2(1, 1)),

	    AnchorPresetData(AnchorPresets::HorizontalStretchTop,		Float2(0, 0),		Float2(1, 0)),
	    AnchorPresetData(AnchorPresets::HorizontalStretchMiddle,	Float2(0, 0.5f),	Float2(1, 0.5f)),
	    AnchorPresetData(AnchorPresets::HorizontalStretchBottom,	Float2(0, 1),		Float2(1, 1)),

	    AnchorPresetData(AnchorPresets::VerticalStretchLeft,		Float2(0, 0),		Float2(0, 1)),
	    AnchorPresetData(AnchorPresets::VerticalStretchCenter,		Float2(0.5f, 0),	Float2(0.5f, 1)),
	    AnchorPresetData(AnchorPresets::VerticalStretchRight,		Float2(1, 0),		Float2(1, 1)),

	    AnchorPresetData(AnchorPresets::StretchAll,					Float2(0, 0),		Float2(1, 1)),
	};



	ContainerControl* Control::__GetParent()
	{
		return m_Parent;
	}

	void Control::__SetParent(ContainerControl* value)
	{
		if (m_Parent == value)
			return;

		Defocus();

		Float2 oldParentSize;
		if (m_Parent != nullptr)
		{
			oldParentSize = m_Parent->Size;
			m_Parent->RemoveChildInternal(this);
		}
		else
		{
			oldParentSize = Float2::Zero;
			ClearState();
		}

		m_Parent = value;
		if (m_Parent != nullptr)
		{
			m_Parent->AddChildInternal(this);
		}

		CacheRootHandle();
		OnParentChangedInternal();

		// Check if parent size has been changed
		if (m_Parent != nullptr && oldParentSize != m_Parent->Size)
		{
			OnParentResized();
		}
	}

	bool Control::HasParent()
	{
		return m_Parent != nullptr;
	}

	int Control::__GetIndexInParent()
	{
		if (m_Parent == nullptr)
		{
			return -1;
		}
		return m_Parent->GetChildIndex(this);
	}

	void Control::__SetIndexInParent(int index)
	{
		m_Parent->ChangeChildIndex(this, index);
	}

	AnchorPresets Control::__GetAnchorPreset()
	{
		AnchorPresets result = AnchorPresets::Custom;
		for (int i = 0; i < ARRAY_SIZE(anchorPresetsData); i++)
		{
			if (Float2::NearEqual(m_AnchorMin, anchorPresetsData[i].Min) &&
				Float2::NearEqual(m_AnchorMax, anchorPresetsData[i].Max))
			{
				result = anchorPresetsData[i].Preset;
				break;
			}
		}
		return result;
	}

	bool Control::__GetEnabled()
	{
		return _isEnabled;
	}

	void Control::__SetEnabled(bool value)
	{
		if (_isEnabled != value)
		{
			_isEnabled = value;
			if (!_isEnabled)
				ClearState();
		}
	}

	bool Control::EnabledInHierarchy()
	{
		if (!_isEnabled)
			return false;
		if (m_Parent != nullptr)
			return m_Parent->EnabledInHierarchy();
		return true;
	}

	bool Control::__GetVisible()
	{
		return _isVisible;
	}

	void Control::__SetVisible(bool value)
	{
		if (_isVisible != value)
		{
			_isVisible = value;
			if (!_isVisible)
				ClearState();

			OnVisibleChanged();
			if (m_Parent != nullptr)
			{
				m_Parent->PerformLayout();
			}
		}
	}

	bool Control::__GetIsDisposing()
	{
		return m_IsDisposing;
	}

	RootControl* Control::__GetRoot()
	{
		return m_Root;
	}

	bool Control::__GetIsMouseOver()
	{
		return _isMouseOver;
	}

	bool Control::__GetIsFocused()
	{
		return m_IsFocused;
	}

	bool Control::__GetIsNavFocused()
	{
		return m_IsNavFocused;
	}
	String& Control::__GetName()
	{
		return m_Name;
	}

	bool Control::VisibleInHierarchy()
	{
		if (!_isVisible)
			return false;
		if (m_Parent != nullptr)
			return m_Parent->VisibleInHierarchy();
		return true;
	}

	WindowRootControl* Control::RootWindow()
	{
		if (m_Parent != nullptr)
		{
			return m_Parent->RootWindow();
		}
		return nullptr;
	}

	float Control::GetDpiScale()
	{
		WindowRootControl* rootWindow = RootWindow();
		if (rootWindow != nullptr)
		{
			GraphicWindow* window = rootWindow->Window();
			if (window != nullptr)
			{
				return window->GetDpiScale();
			}
		}

		return Platform::GetDpiScale();
	}

	Float2 Control::ScreenPos()
	{
		RootControl* parentWin = Root;
		if (parentWin == nullptr)
		{
			ENGINE_ASSERT("Missing parent window.");
		}
		Float2 clientPos = PointToWindow(Float2::Zero);
		return parentWin->PointToScreen(clientPos);
	}

	CursorType Control::__GetCursor()
	{
		if (m_Parent != nullptr)
		{
			return m_Parent->__GetCursor();
		}
		return CursorType::Default;
	}

	void Control::__SetCursor(CursorType type)
	{
		if (m_Parent != nullptr)
		{
			m_Parent->__SetCursor(type);
		}
	}

	Control::Control() : Control(0, 0, 0, 0)
	{
	}

	Control::Control(float x, float y, float width, float height) : Control(Rectangle(x, y, width, height))
	{
	}

	Control::Control(Float2 location, Float2 size) : Control(Rectangle(location, size))
	{
	}

	Control::Control(Rectangle bounds)
	{
		m_Bounds = bounds;
		m_Offsets = Margin(bounds.GetX(), bounds.GetWidth(), bounds.GetY(), bounds.GetHeight());
		UpdateTransform();
		m_OnUpdateTooltip.Bind(CreateFunc<Control, &ScrollBar::OnUpdateTooltip>(this));
	}

	void Control::Dispose()
	{
		if (m_IsDisposing)
			return;

		// Call event
		OnDestroy();

		// Unlink
		m_Parent = nullptr;
		m_OnUpdateTooltip.UnbindAll();
	}

	void Control::Draw()
	{
		// Paint Background
		if (BackgroundColor.a > 0.0f)
		{
			Render2D::FillRectangle(Rectangle(Float2::Zero, Size), BackgroundColor);
		}
	}

	void Control::ClearState()
	{
		Defocus();
		if (_isMouseOver)
		{
			OnMouseLeave();
		}
		if (_isDragOver)
		{
			OnDragLeave();
		}
		while (_touchOvers.Count() != 0)
		{
			OnTouchLeave(_touchOvers[0]);
		}
	}


	void Control::Defocus()
	{
		if (ContainsFocus())
		{
			Focus(nullptr);
		}
	}

	void Control::StartMouseCapture(bool useMouseScreenOffset)
	{
		RootControl* parent = Root;
		if (parent != nullptr)
		{
			parent->StartTrackingMouse(this, useMouseScreenOffset);
		}
	}
	void Control::EndMouseCapture()
	{
		RootControl* parent = Root;
		if (parent != nullptr)
		{
			parent->EndTrackingMouse();
		}
	}

	bool Control::Focus(Control* c)
	{
		return m_Parent != nullptr && m_Parent->Focus(c);
	}

	Control* Control::GetNavTarget(NavDirection direction)
	{
		switch (direction)
		{
		case NavDirection::Up: return NavTargetUp;
		case NavDirection::Down: return NavTargetDown;
		case NavDirection::Left: return NavTargetLeft;
		case NavDirection::Right: return NavTargetRight;
		default: return nullptr;
		}
	}

	Float2 Control::GetNavOrigin(NavDirection direction)
	{
		Float2 size = Size;
		switch (direction)
		{
		case NavDirection::Up: return Float2(size.x * 0.5f, 0);
		case NavDirection::Down: return Float2(size.x * 0.5f, size.y);
		case NavDirection::Left: return Float2(0, size.y * 0.5f);
		case NavDirection::Right: return Float2(size.y, size.y * 0.5f);
		case NavDirection::Next: return Float2::Zero;
		case NavDirection::Previous: return size;
		default: return size * 0.5f;
		}
	}

	Control* Control::OnNavigate(NavDirection direction, Float2 location, Control* caller, List<Control*>& visited)
	{
		if (caller == m_Parent && AutoFocus && __GetVisible())
			return this;

		if (m_Parent == nullptr)
		{
			return nullptr;
		}

		return m_Parent->OnNavigate(direction, PointToParent(GetNavOrigin(direction)), caller, visited);
	}

	void Control::NavigationFocus()
	{
		Focus();
		if (IsFocused)
		{
			m_IsNavFocused = true;

			// Ensure to be in a view
			auto parent = Parent;
			while (parent != nullptr)
			{
				Panel* panel;
				if (TypeTryCast<Panel>(parent, panel)&& ((panel->vScrollBar != nullptr && panel->vScrollBar->Enabled) || (panel->hScrollBar != nullptr && panel->hScrollBar->Enabled)))
				{
					panel->ScrollViewTo(this);
					break;
				}
				parent = parent->Parent;
			}
		}
	}

	void Control::OnMouseEnter(Float2 location)
	{
		// Set flag
		_isMouseOver = true;

		// Update tooltip
		if (ShowTooltip() && OnTestTooltipOverControl(location))
		{
			// Tooltip.OnMouseEnterControl(this);
			SetUpdate(&m_TooltipUpdate, &m_OnUpdateTooltip);
		}
	}

	void Control::OnMouseMove(Float2 location)
	{
		// Update tooltip
		if (ShowTooltip() && OnTestTooltipOverControl(location))
		{
			if (m_TooltipUpdate.IsBinded())
			{
				// Tooltip.OnMouseEnterControl(this);
				SetUpdate(&m_TooltipUpdate, &m_OnUpdateTooltip);
			}
		}
		else if (m_TooltipUpdate.IsBinded())
		{
			SetUpdate(&m_TooltipUpdate, nullptr);
			// Tooltip.OnMouseLeaveControl(this);
		}
	}

	void Control::OnMouseLeave()
	{
		// Clear flag
		_isMouseOver = false;

		// Update tooltip
		if (m_TooltipUpdate.IsBinded())
		{
			SetUpdate(&m_TooltipUpdate, nullptr);
			// Tooltip.OnMouseLeaveControl(this);
		}
	}

	bool Control::IsTouchOver()
	{
		return _touchOvers.Count() != 0;
	}

	bool Control::IsTouchPointerOver(int pointerId)
	{
		return _touchOvers.Contains(pointerId);
	}

	void Control::OnTouchEnter(Float2 location, int pointerId)
	{
		_touchOvers.Add(pointerId);
	}

	void Control::OnTouchLeave(int pointerId)
	{
		_touchOvers.Remove(pointerId);
		if (_touchOvers.Count() == 0)
			OnTouchLeave();
	}

	void Control::DoDragDrop(DragData* data)
	{
		// Hide tooltip
		// Tooltip?.Hide();
		RootControl* root = Root;
		if (root != nullptr)
		{
			root->DoDragDrop(data);
		}
	}

	Tooltip* Control::GetTooltip()
	{
		if (Tooltip)
		{
			return Tooltip;
		}
		return Style::Current->SharedTooltip;
	}

	bool Control::ShowTooltip()
	{
		return TooltipText.Length() > 0;
	}

	Control* Control::LinkTooltip(String text, ::SE::Tooltip* customTooltip)
	{
		TooltipText = text;
		Tooltip = customTooltip;
		return this;
	}

	void Control::UnlinkTooltip()
	{
		TooltipText = String::Empty;
		Tooltip = nullptr;
	}

	bool Control::OnShowTooltip(String text, Float2& location, Rectangle& area)
	{
		text = TooltipText;
		location = Float2(0.5f, 1.0f) * Size;
		area = Rectangle(Float2::Zero, Size);
		return ShowTooltip();
	}

	bool Control::OnTestTooltipOverControl(Float2 location)
	{
		return ContainsPoint(location) && ShowTooltip();
	}

	bool Control::RayCast(Float2& location, Control*& hit)
	{
		if (ContainsPoint(location, true))
		{
			hit = this;
			return true;
		}
		hit = nullptr;
		return false;
	}

	bool Control::IntersectsContent(Float2& locationParent, Float2& location)
	{
		location = PointFromParent(locationParent);
		return ContainsPoint(location);
	}

	bool Control::ContainsPoint(Float2& location, bool precise)
	{
		return location.x >= 0 &&
			   location.y >= 0 &&
			   location.x <= m_Bounds.Size.x &&
			   location.y <= m_Bounds.Size.y;
	}

	Float2 Control::PointToParent(ContainerControl* parent, Float2 location)
	{
		if (parent == nullptr)
		{
			ENGINE_UNREACHABLE_CODE()
		}

		Control* c = this;
		while (c != nullptr)
		{
			location = c->PointToParent(location);
			c = c->Parent;
			if (c == parent)
				break;
		}
		return location;
	}

	Float2 Control::PointToParent(Float2 location)
	{
		Float2 result;
		Matrix3x3::Transform2DPoint(location, m_CachedTransform, result);
		return result;
	}

	Float2 Control::PointFromParent(Float2 locationParent)
	{
		Float2 result;
		Matrix3x3::Transform2DPoint(locationParent, m_CachedTransformInv, result);
		return result;
	}

	Float2 Control::PointFromParent(ContainerControl* parent, Float2 location)
	{
		if (parent == nullptr)
		{
			ENGINE_UNREACHABLE_CODE()
		}

		auto path = List<Control*>();

		Control* c = this;
		while (c != nullptr && c != parent)
		{
			path.Add(c);
			c = c->Parent;
		}
		for (int i = path.Count() - 1; i >= 0; i--)
		{
			location = path[i]->PointFromParent(location);
		}
		return location;
	}

	Float2 Control::PointToWindow(Float2 location)
	{
		location = PointToParent(location);
		if (m_Parent != nullptr)
		{
			location = m_Parent->PointToWindow(location);
		}
		return location;
	}

	Float2 Control::PointFromWindow(Float2 location)
	{
		if (m_Parent != nullptr)
		{
			location = m_Parent->PointFromWindow(location);
		}
		return PointFromParent(location);
	}

	Float2 Control::PointToScreen(Float2 location)
	{
		location = PointToParent(location);
		if (m_Parent != nullptr)
		{
			location = m_Parent->PointToScreen(location);
		}
		return location;
	}

	Float2 Control::PointFromScreen(Float2 location)
	{
		if (m_Parent != nullptr)
		{
			location = m_Parent->PointFromScreen(location);
		}
		return PointFromParent(location);
	}

	void Control::OnLocationChanged()
	{
		LocationChanged(this);
	}

	void Control::OnSizeChanged()
	{
		SizeChanged(this);
		if (m_Parent != nullptr)
		{
			m_Parent->OnChildResized(this);
		}
	}

	void Control::SetScaleInternal(Float2 scale)
	{
		m_Scale = scale;
		UpdateTransform();
		if (m_Parent != nullptr)
		{
			m_Parent->OnChildResized(this);
		}
	}

	void Control::SetPivotInternal(Float2 pivot)
	{
		m_Pivot = pivot;
		UpdateTransform();
		if (m_Parent != nullptr)
		{
			m_Parent->OnChildResized(this);
		}
	}

	void Control::SetShearInternal(Float2 shear)
	{
		m_Shear = shear;
		UpdateTransform();
		if (m_Parent != nullptr)
		{
			m_Parent->OnChildResized(this);
		}
	}

	void Control::SetRotationInternal(float rotation)
	{
		m_Rotation = rotation;
		UpdateTransform();
		if (m_Parent != nullptr)
		{
			m_Parent->OnChildResized(this);
		}
	}

	void Control::OnVisibleChanged()
	{
		// Clear state when control gets hidden
		if (!_isVisible && _isMouseOver)
		{
			OnMouseLeave();
		}

		VisibleChanged(this);
	}

	void Control::OnParentChangedInternal()
	{
		ParentChanged(this);
	}

	void Control::CacheRootHandle()
	{
		if (m_Root != nullptr)
			RemoveUpdateCallbacks(m_Root);

		if (m_Parent != nullptr)
		{
			m_Root = m_Parent->Root;
		}

		if (m_Root != nullptr)
		{
			AddUpdateCallbacks(m_Root);
		}
	}

	void Control::AddUpdateCallbacks(RootControl* root)
	{
		if (m_TooltipUpdate.IsBinded())
		{
			root->UpdateCallbacksToAdd.Add(&m_TooltipUpdate);
		}
	}

	void Control::RemoveUpdateCallbacks(RootControl* root)
	{
		if (m_TooltipUpdate.IsBinded())
			root->UpdateCallbacksToRemove.Add(&m_TooltipUpdate);
	}

	void Control::SetUpdate(Delegate<float>* onUpdate, Delegate<float>* value)
	{
		if (onUpdate == value)
			return;
		if (m_Root != nullptr && onUpdate != nullptr)
		{
			m_Root->UpdateCallbacksToRemove.Add(onUpdate);
		}
		onUpdate = value;
		if (m_Root != nullptr && onUpdate != nullptr)
		{
			m_Root->UpdateCallbacksToAdd.Add(onUpdate);
		}
	}

	void Control::OnParentResized()
	{
		if (!m_AnchorMin.IsZero() || !m_AnchorMax.IsZero())
		{
			UpdateBounds();
		}
	}

	void Control::OnDestroy()
	{
		// Set disposing flag
		m_IsDisposing = true;
		Defocus();
		UnlinkTooltip();
		Tag = nullptr;
	}

	void Control::__SetAnchorPreset(AnchorPresets presets)
	{
		SetAnchorPreset(presets, false);
	}
}
