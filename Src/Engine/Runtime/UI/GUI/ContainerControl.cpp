

#include "ContainerControl.h"

#include "WindowRootControl.h"
#include "Core/Types/Collections/Sorting.h"
#include "Core/Types/Delegate.h"
#include "Runtime/Render/2D/Render2D.h"

namespace SE
{
	ContainerControl::ContainerControl()
	{
		m_IsLayoutLocked = true;
	}

	void ContainerControl::LockChildrenRecursive()
	{
		SetIsLayoutLocked(true);
		for (int i = 0; i < m_Children.Count(); i++)
		{
			ContainerControl* child = TypeTryCast<ContainerControl>(m_Children[i]);
			if (child != nullptr)
			{
				child->LockChildrenRecursive();
			}
		}
	}

	void ContainerControl::UnlockChildrenRecursive()
	{
		SetIsLayoutLocked(false);
		for (int i = 0; i < m_Children.Count(); i++)
		{
			ContainerControl* child = TypeTryCast<ContainerControl>(m_Children[i]);
			if (child != nullptr)
			{
				child->UnlockChildrenRecursive();
			}
		}
	}

	void ContainerControl::RemoveChildren()
	{
		bool wasLayoutLocked = GetIsLayoutLocked();
		SetIsLayoutLocked(true);

		// Delete children
		while (m_Children.Count() > 0)
		{
			m_Children[0]->Parent = nullptr;
		}

		SetIsLayoutLocked(wasLayoutLocked);
		PerformLayout();
	}

	void ContainerControl::DisposeChildren()
	{
		bool wasLayoutLocked = GetIsLayoutLocked();
		SetIsLayoutLocked(true);

		// Delete children
		while (m_Children.Count() > 0)
		{
			Control* c = m_Children.Last();
			c->Dispose();
			Delete(c);

			m_Children.RemoveLast();
		}

		SetIsLayoutLocked(wasLayoutLocked);
		PerformLayout();
	}

	void ContainerControl::RemoveChild(Control* child) const
	{
		ENGINE_ASSERT(child != nullptr);
		ENGINE_ASSERT_M(child->Parent != this, SE_TEXT("Argument child cannot be removed, if current container is not its parent."));

		// Unlink
		child->Parent = nullptr;
	}

	int ContainerControl::GetChildIndex(const Control* child) const
	{
		return m_Children.Find(child);
	}

	void ContainerControl::ChangeChildIndex(Control* child, int newIndex)
	{
		int oldIndex = m_Children.Find(child);
		if (oldIndex == newIndex || oldIndex == -1)
		{
			return;
		}
		m_Children.RemoveAt(oldIndex);

		if (newIndex < 0 || newIndex >= m_Children.Count())
		{
			// Append at the end
			m_Children.Add(child);
		}
		else
		{
			// Change order
			m_Children.Insert(newIndex, child);
		}

		PerformLayout();
	}

	int ContainerControl::GetChildIndexAt(Float2 point)
	{
		int result = -1;
		Float2 childLocation;
		for (int i = m_Children.Count() - 1; i >= 0; i--)
		{
			Control* child = m_Children[i];
			if (IntersectsChildContent(child, point, childLocation))
			{
				result = i;
				break;
			}
		}
		return result;
	}

	Control* ContainerControl::GetChildAt(Float2 point)
	{
		Control* result = nullptr;
		Float2 childLocation;
		for (int i = m_Children.Count() - 1; i >= 0; i--)
		{
			Control* child = m_Children[i];
			if (IntersectsChildContent(child, point, childLocation))
			{
				result = child;
				break;
			}
		}
		return result;
	}

	Control* ContainerControl::GetChildAt(Float2 point, const Function<bool(Control*)>& isValid)
	{
		Control* result = nullptr;
		Float2 childLocation;
		for (int i = m_Children.Count() - 1; i >= 0; i--)
		{
			Control* child = m_Children[i];
			if (isValid(child) && IntersectsChildContent(child, point, childLocation))
			{
				result = child;
				break;
			}
		}
		return result;
	}

	Control* ContainerControl::GetChildAtRecursive(Float2 point)
	{
		Control* result = nullptr;
		Float2 childLocation;
		for (int i = m_Children.Count() - 1; i >= 0; i--)
		{
			Control* child = m_Children[i];
			if (child->Visible && IntersectsChildContent(child, point, childLocation))
			{
				ContainerControl* containerControl = TypeTryCast<ContainerControl>(child);
				Control* childAtRecursive = nullptr;
				if (containerControl != nullptr)
				{
					childAtRecursive = containerControl->GetChildAtRecursive(childLocation);
				}

				if (childAtRecursive != nullptr && childAtRecursive->Visible)
				{
					child = childAtRecursive;
				}
				result = child;
				break;
			}
		}
		return result;
	}

	Rectangle ContainerControl::GetClientArea()
	{
		return GetDesireClientArea();
	}

	void ContainerControl::SortChildren()
	{
		Sorting::QuickSort(m_Children);
		PerformLayout();
	}

	void ContainerControl::SortChildrenRecursive()
	{
		SortChildren();

		for (int i = 0; i < m_Children.Count(); i++)
		{
			ContainerControl* child = TypeTryCast<ContainerControl>(m_Children[i]);
			if (child != nullptr)
			{
				child->SortChildrenRecursive();
			}
		}
	}

	void ContainerControl::OnChildrenChanged()
	{
		// Check if control isn't during disposing state
		if (!__GetIsDisposing())
		{
			// Arrange child controls
			PerformLayout();
		}
	}

	void ContainerControl::CacheRootHandle()
	{
		Control::CacheRootHandle();

		for (int i = 0; i < m_Children.Count(); i++)
		{
			m_Children[i]->CacheRootHandle();
		}
	}

	void ContainerControl::AddChildInternal(Control* child)
	{
		ENGINE_ASSERT_M(child != nullptr, SE_TEXT("Invalid control."));
		const ContainerControl* parent = Parent;
		ENGINE_ASSERT(parent != child);

		// Add child
		m_Children.Add(child);

		OnChildrenChanged();
	}

	void ContainerControl::RemoveChildInternal(Control* child)
	{
		ENGINE_ASSERT_M(child != nullptr, SE_TEXT("Invalid control."));

		// Remove child
		m_Children.Remove(child);

		OnChildrenChanged();
	}

	Rectangle ContainerControl::GetDesireClientArea()
	{
		return Rectangle(Float2::Zero, Size);
	}

	bool ContainerControl::IntersectsChildContent(Control* child, Float2 location, Float2& childSpaceLocation)
	{
		return child->IntersectsContent(location, childSpaceLocation);
	}

	Control* ContainerControl::OnNavigate(NavDirection direction, Float2 location, Control* caller, List<Control*>& visited)
    {
        // Try to focus itself first (only if navigation focus can enter this container)
        if (AutoFocus && !ContainsFocus())
            return this;

        // Try to focus children
        if (m_Children.Count() != 0 && !visited.Contains(this))
        {
            visited.Add(this);

            // Perform automatic navigation based on the layout
            Control* result = NavigationRaycast(direction, location, visited);
            Float2 rightMostLocation = location;
            if (result == nullptr && (direction == NavDirection::Next || direction == NavDirection::Previous))
            {
                // Try wrap the navigation over the layout based on the direction
                result = NavigationWrap(direction, location, visited, rightMostLocation);
            }
            if (result != nullptr)
            {
                // HACK: only the 'previous' direction needs the rightMostLocation so i used a ternary conditional operator.
                // The rightMostLocation can probably become a 'desired raycast origin' that gets calculated correctly in the NavigationWrap method.
                Float2 useLocation = direction == NavDirection::Previous ? rightMostLocation : location;
                result = result->OnNavigate(direction, result->PointFromParent(useLocation), this, visited);
                if (result != nullptr)
                    return result;
            }
        }

        // Try to focus itself
        if (AutoFocus && !IsFocused || caller == this)
            return this;

        // Route navigation to parent
        ContainerControl* parent = Parent;
        if (AutoFocus && Visible)
        {
            // Focusable container controls use own nav origin instead of the provided one
            location = GetNavOrigin(direction);
        }

		if (parent != nullptr)
		{
			return parent->OnNavigate(direction, PointToParent(location), caller, visited);
		}

        return nullptr;
    }

	bool ContainerControl::CanNavigateChild(Control* child)
	{
		return !child->IsFocused && child->Enabled && child->Visible && CanGetAutoFocus(child);
	}

	Control* ContainerControl::NavigationWrap(NavDirection direction, Float2 location, List<Control*> visited, Float2& rightMostLocation)
	{
		// This searches form a child that calls this navigation event (see Control.OnNavigate) to determinate the layout wrapping size based on that child size
		WindowRootControl* currentChild = reinterpret_cast<WindowRootControl*>(RootWindow()->GetFocusedControl());
		visited.Add(this);
		if (currentChild != nullptr)
		{
			Float2 layoutSize = currentChild->Size;
			Float2 predictedLocation = Float2::Minimum;
			switch (direction)
			{
			case NavDirection::Next:
				predictedLocation = Float2(0, location.y + layoutSize.y);
				break;
			case NavDirection::Previous:
				predictedLocation = Float2(Size.operator->().x, location.y - layoutSize.y);
				break;
			}
			if (Rectangle(Float2::Zero, Size).Contains(predictedLocation))
			{
				Control* result = NavigationRaycast(direction, predictedLocation, visited);
				if (result != nullptr)
				{
					rightMostLocation = predictedLocation;
					return result;
				}
			}
		}
		rightMostLocation = location;
		ContainerControl* parent = Parent;
		if (parent != nullptr)
		{
			return parent->NavigationWrap(direction, PointToParent(location), visited, rightMostLocation);
		}
		return nullptr;
	}

	bool ContainerControl::CanGetAutoFocus(Control* c)
	{
		if (c->AutoFocus)
			return true;

		ContainerControl* cc = TypeTryCast<ContainerControl>(c);
		if (cc != nullptr)
		{
			for (int i = 0; i < cc->Children().Count(); i++)
			{
				if (cc->CanNavigateChild(cc->GetChildAt(i)))
				{
					return true;
				}
			}
		}
		return false;
	}

	Control* ContainerControl::NavigationRaycast(NavDirection direction, Float2 location, List<Control*> visited)
	{
		Float2 uiDir1 = Float2::Zero, uiDir2 = Float2::Zero;
		switch (direction)
		{
		case NavDirection::Up:
			uiDir1 = uiDir2 = Float2(0, -1);
			break;
		case NavDirection::Down:
			uiDir1 = uiDir2 = Float2(0, 1);
			break;
		case NavDirection::Left:
			uiDir1 = uiDir2 = Float2(-1, 0);
			break;
		case NavDirection::Right:
			uiDir1 = uiDir2 = Float2(1, 0);
			break;
		case NavDirection::Next:
			uiDir1 = Float2(1, 0);
			uiDir2 = Float2(0, 1);
			break;
		case NavDirection::Previous:
			uiDir1 = Float2(-1, 0);
			uiDir2 = Float2(0, -1);
			break;
		}

		Control* result = nullptr;
		float minDistance = Max_float;;
		for (int i = 0; i < m_Children.Count(); i++)
		{
			Control* child = m_Children[i];
			if (!CanNavigateChild(child) || visited.Contains(child))
				continue;
			Float2 childNavLocation = child->Center;
			Rectangle childBounds = child->Bounds;
			Float2 childNavDirection = Float2::Normalize(childNavLocation - location);
			Float2 childNavCoherence1 = Float2::Dot(uiDir1, childNavDirection);
			Float2 childNavCoherence2 = Float2::Dot(uiDir2, childNavDirection);
			float distance = Rectangle::Distance(childBounds, location);
			if (childNavCoherence1 > Math::EPSILON && childNavCoherence2 > Math::EPSILON && distance < minDistance)
			{
				minDistance = distance;
				result = child;
			}
		}
		return result;
	}

	void ContainerControl::UpdateContainsFocus()
	{
		// Get current state and update all children
		bool result = Control::ContainsFocus();

		for (int i = 0; i < m_Children.Count(); i++)
		{
			Control* control = m_Children[i];
			ContainerControl* containerControl;
			if (TypeTryCast<ContainerControl>(control, containerControl))
			{
				containerControl->UpdateContainsFocus();
			}
			if (control->ContainsFocus())
				result = true;
		}

		// Check if state has been changed
		if (result != _containsFocus)
		{
			_containsFocus = result;
			if (result)
			{
				OnStartContainsFocus();
			}
			else
			{
				OnEndContainsFocus();
			}
		}
	}

	void ContainerControl::UpdateChildrenBounds()
	{
		for (int i = 0; i < m_Children.Count(); i++)
		{
			m_Children[i]->UpdateBounds();
		}
	}

	void ContainerControl::OnDestroy()
	{
		// Steal focus from children
		if (ContainsFocus())
		{
			Focus();
		}

		// Disable layout
		if (!GetIsLayoutLocked())
		{
			LockChildrenRecursive();
		}

		Control::OnDestroy();

		// Pass event further
		for (int i = 0; i < m_Children.Count(); i++)
		{
			m_Children[i]->OnDestroy();
		}
		m_Children.Clear();
	}

	bool ContainerControl::IsTouchOver()
	{
		if (Control::IsTouchOver())
			return true;
		for (int i = 0; i < m_Children.Count() && m_Children.Count() > 0; i++)
		{
			if (m_Children[i]->IsTouchOver())
				return true;
		}
		return false;
	}

	void ContainerControl::Update(float deltaTime)
	{
		Control::Update(deltaTime);

		// Update all enabled child controls
		for (int i = 0; i < m_Children.Count(); i++)
		{
			if (m_Children[i]->Enabled)
			{
				m_Children[i]->Update(deltaTime);
			}
		}
	}

	void ContainerControl::ClearState()
	{
		Control::ClearState();

		// Clear state for any nested controls
		for (int i = 0; i < m_Children.Count(); i++)
		{
			Control* child = m_Children[i];
			//if (child->GetEnabled() && child->GetEnabled())
			child->ClearState();
		}
	}

	void ContainerControl::Draw()
	{
		DrawSelf();
		if (ClipChildren)
		{
			Rectangle clientArea = GetDesireClientArea();
			Render2D::PushClip(clientArea);
			DrawChildren();
			Render2D::PopClip();
		}
		else
		{
			DrawChildren();
		}
	}

	void ContainerControl::DrawChildren()
	{
		// Draw all visible child controls
		if (CullChildren)
		{
			Rectangle globalClipping;
			Matrix3x3 globalTransform;
			Matrix3x3 globalChildTransform;
			Render2D::PeekClip(globalClipping);
			Render2D::PeekTransform(globalTransform);
			for (int i = 0; i < m_Children.Count(); i++)
			{
				Control* child = m_Children[i];
				if (child->Visible)
				{
					Matrix3x3::Multiply(child->GetCachedTransform(), globalTransform, globalChildTransform);
					Rectangle childGlobalRect = Rectangle(globalChildTransform.M31, globalChildTransform.M32, child->Width * globalChildTransform.M11, child->Height * globalChildTransform.M22);
					if (globalClipping.Intersects(childGlobalRect))
					{
						Render2D::PushTransform(child->GetCachedTransform());
						child->Draw();
						Render2D::PopTransform();
					}
				}
			}
		}
		else
		{
			for (int i = 0; i < m_Children.Count(); i++)
			{
				Control* child = m_Children[i];
				if (child->Visible)
				{
					Render2D::PushTransform(child->GetCachedTransform());
					child->Draw();
					Render2D::PopTransform();
				}
			}
		}
	}

	void ContainerControl::PerformLayout(bool force)
	{
		if (GetIsLayoutLocked() && !force)
			return;

		bool wasLocked = GetIsLayoutLocked();
		if (!wasLocked)
		{
			LockChildrenRecursive();
		}

		PerformLayoutBeforeChildren();

		for (int i = 0; i < m_Children.Count(); i++)
		{
			m_Children[i]->PerformLayout(true);
		}

		PerformLayoutAfterChildren();

		if (!wasLocked)
		{
			UnlockChildrenRecursive();
		}
	}

	bool ContainerControl::RayCast(Float2& location, Control*& hit)
	{
		if (RayCastChildren(location, hit))
			return true;
		return Control::RayCast(location, hit);
	}

	bool ContainerControl::RayCastChildren(Float2 location, Control*& hit)
	{
		Float2 childLocation;
		for (int i = m_Children.Count() - 1; i >= 0 && m_Children.Count() > 0; i--)
		{
			Control* child = m_Children[i];
			if (child->Visible)
			{
				IntersectsChildContent(child, location, childLocation);
				if (child->RayCast(childLocation, hit))
					return true;
			}
		}
		hit = nullptr;
		return false;
	}

	void ContainerControl::OnMouseEnter(Float2 location)
	{
		Float2 childLocation;
		// Check all children collisions with mouse and fire events for them
		for (int i = m_Children.Count() - 1; i >= 0 && m_Children.Count() > 0; i--)
		{
			Control* child = m_Children[i];
			if (child->Visible && child->Enabled)
			{
				if (IntersectsChildContent(child, location, childLocation))
				{
					// Enter
					child->OnMouseEnter(childLocation);
				}
			}
		}

		Control::OnMouseEnter(location);
	}

	void ContainerControl::OnMouseMove(Float2 location)
	{
		Float2 childLocation;
		// Check all children collisions with mouse and fire events for them
		for (int i = m_Children.Count() - 1; i >= 0 && m_Children.Count() > 0; i--)
		{
			Control* child = m_Children[i];
			if (child != nullptr && child->Visible && child->Enabled)
			{
				if (IntersectsChildContent(child, location, childLocation))
				{
					if (child->IsMouseOver)
					{
						// Move
						child->OnMouseMove(childLocation);
					}
					else
					{
						// Enter
						child->OnMouseEnter(childLocation);
					}
				}
				else if (child->IsMouseOver)
				{
					// Leave
					child->OnMouseLeave();
				}
			}
		}

		Control::OnMouseMove(location);
	}

	void ContainerControl::OnMouseLeave()
	{
		// Check all children collisions with mouse and fire events for them
		for (int i = 0; i < m_Children.Count() && m_Children.Count() > 0; i++)
		{
			Control* child = m_Children[i];
			if (child->Visible && child->Enabled && child->IsMouseOver)
			{
				// Leave
				child->OnMouseLeave();
			}
		}

		Control::OnMouseLeave();
	}

	bool ContainerControl::OnMouseWheel(Float2 location, float delta)
	{
		Float2 childLocation;
		// Check all children collisions with mouse and fire events for them
		for (int i = m_Children.Count() - 1; i >= 0 && m_Children.Count() > 0; i--)
		{
			Control* child = m_Children[i];
			if (child->Visible && child->Enabled)
			{
				if (IntersectsChildContent(child, location, childLocation))
				{
					if (child->OnMouseWheel(childLocation, delta))
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	bool ContainerControl::OnMouseDown(Float2 location, MouseButton button)
	{
		Float2 childLocation;
		// Check all children collisions with mouse and fire events for them
		for (int i = m_Children.Count() - 1; i >= 0 && m_Children.Count() > 0; i--)
		{
			Control* child = m_Children[i];
			if (child->Visible && child->Enabled)
			{
				if (IntersectsChildContent(child, location, childLocation))
				{
					if (child->OnMouseDown(childLocation, button))
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	bool ContainerControl::OnMouseUp(Float2 location, MouseButton button)
	{
		Float2 childLocation;
		// Check all children collisions with mouse and fire events for them
		for (int i = m_Children.Count() - 1; i >= 0 && m_Children.Count() > 0; i--)
		{
			Control* child = m_Children[i];
			if (child->Visible && child->Enabled)
			{
				if (IntersectsChildContent(child, location, childLocation))
				{
					if (child->OnMouseUp(childLocation, button))
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	bool ContainerControl::OnMouseDoubleClick(Float2 location, MouseButton button)
	{
		Float2 childLocation;
		// Check all children collisions with mouse and fire events for them
		for (int i = m_Children.Count() - 1; i >= 0 && m_Children.Count() > 0; i--)
		{
			Control* child = m_Children[i];
			if (child->Visible && child->Enabled)
			{
				if (IntersectsChildContent(child, location, childLocation))
				{
					if (child->OnMouseDoubleClick(childLocation, button))
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	bool ContainerControl::IsTouchPointerOver(int pointerId)
	{
		if (Control::IsTouchPointerOver(pointerId))
			return true;

		for (int i = 0; i < m_Children.Count() && m_Children.Count() > 0; i++)
		{
			if (m_Children[i]->IsTouchPointerOver(pointerId))
				return true;
		}

		return false;
	}

	void ContainerControl::OnTouchEnter(Float2 location, int pointerId)
	{
		Float2 childLocation;
		for (int i = m_Children.Count() - 1; i >= 0 && m_Children.Count() > 0; i--)
		{
			Control* child = m_Children[i];
			if (child->Visible && child->Enabled && !child->IsTouchPointerOver(pointerId))
			{
				if (IntersectsChildContent(child, location, childLocation))
				{
					child->OnTouchEnter(childLocation, pointerId);
				}
			}
		}

		Control::OnTouchEnter(location, pointerId);
	}

	bool ContainerControl::OnTouchDown(Float2 location, int pointerId)
	{
		Float2 childLocation;
		for (int i = m_Children.Count() - 1; i >= 0 && m_Children.Count() > 0; i--)
		{
			Control* child = m_Children[i];
			if (child->Visible && child->Enabled)
			{
				if (IntersectsChildContent(child, location, childLocation))
				{
					if (!child->IsTouchPointerOver(pointerId))
					{
						child->OnTouchEnter(location, pointerId);
					}
					if (child->OnTouchDown(childLocation, pointerId))
					{
						return true;
					}
				}
			}
		}

		return Control::OnTouchDown(location, pointerId);
	}

	void ContainerControl::OnTouchMove(Float2 location, int pointerId)
	{
		Float2 childLocation;
		for (int i = m_Children.Count() - 1; i >= 0 && m_Children.Count() > 0; i--)
		{
			Control* child = m_Children[i];
			if (child->Visible && child->Enabled)
			{
				if (IntersectsChildContent(child, location, childLocation))
				{
					if (child->IsTouchPointerOver(pointerId))
					{
						child->OnTouchMove(childLocation, pointerId);
					}
					else
					{
						child->OnTouchEnter(childLocation, pointerId);
					}
				}
				else if (child->IsTouchPointerOver(pointerId))
				{
					child->OnTouchLeave(pointerId);
				}
			}
		}

		Control::OnTouchMove(location, pointerId);
	}

	bool ContainerControl::OnTouchUp(Float2 location, int pointerId)
	{
		Float2 childLocation;
		for (int i = m_Children.Count() - 1; i >= 0 && m_Children.Count() > 0; i--)
		{
			Control* child = m_Children[i];
			if (child->Visible && child->Enabled && child->IsTouchPointerOver(pointerId))
			{
				if (IntersectsChildContent(child, location, childLocation))
				{
					if (child->OnTouchUp(childLocation, pointerId))
					{
						return true;
					}
				}
			}
		}

		return Control::OnTouchUp(location, pointerId);
	}

	void ContainerControl::OnTouchLeave(int pointerId)
	{
		for (int i = 0; i < m_Children.Count() && m_Children.Count() > 0; i++)
		{
			Control* child = m_Children[i];
			if (child->Visible && child->Enabled && child->IsTouchPointerOver(pointerId))
			{
				child->OnTouchLeave(pointerId);
			}
		}

		Control::OnTouchLeave(pointerId);
	}

	void ContainerControl::OnTouchLeave()
	{
		Control::OnTouchLeave();
	}

	bool ContainerControl::OnCharInput(Char c)
	{
		for (int i = 0; i < m_Children.Count() && m_Children.Count() > 0; i++)
		{
			Control* child = m_Children[i];
			if (child->Enabled && child->ContainsFocus())
			{
				return child->OnCharInput(c);
			}
		}
		return false;
	}

	bool ContainerControl::OnKeyDown(KeyboardKeys key)
	{
		for (int i = 0; i < m_Children.Count() && m_Children.Count() > 0; i++)
		{
			Control* child = m_Children[i];
			if (child->Enabled && child->ContainsFocus())
			{
				return child->OnKeyDown(key);
			}
		}
		return false;
	}

	void ContainerControl::OnKeyUp(KeyboardKeys key)
	{
		for (int i = 0; i < m_Children.Count() && m_Children.Count() > 0; i++)
		{
			Control* child = m_Children[i];
			if (child->Enabled && child->ContainsFocus())
			{
				child->OnKeyUp(key);
				break;
			}
		}
	}

	DragDropEffect ContainerControl::OnDragEnter(const Float2& location, DragData* data)
	{
		// Base
		DragDropEffect result = Control::OnDragEnter(location, data);

		Float2 childLocation;
		// Check all children collisions with mouse and fire events for them
		for (int i = m_Children.Count() - 1; i >= 0 && m_Children.Count() > 0; i--)
		{
			Control* child = m_Children[i];
			if (child->Visible && child->Enabled)
			{
				if (IntersectsChildContent(child, location, childLocation))
				{
					// Enter
					result = child->OnDragEnter(childLocation, data);
					if (result != DragDropEffect::None)
						break;
				}
			}
		}

		return result;
	}

	DragDropEffect ContainerControl::OnDragMove(const Float2& location, DragData* data)
	{
		// Base
		DragDropEffect result = Control::OnDragMove(location, data);

		Float2 childLocation;
		// Check all children collisions with mouse and fire events for them
		for (int i = m_Children.Count() - 1; i >= 0 && m_Children.Count() > 0; i--)
		{
			Control* child = m_Children[i];
			if (child->Visible && child->Enabled)
			{
				if (IntersectsChildContent(child, location, childLocation))
				{
					if (child->IsDragOver())
					{
						// Move
						DragDropEffect tmpResult = child->OnDragMove(childLocation, data);
						if (tmpResult != DragDropEffect::None)
							result = tmpResult;
					}
					else
					{
						// Enter
						DragDropEffect tmpResult = child->OnDragEnter(childLocation, data);
						if (tmpResult != DragDropEffect::None)
							result = tmpResult;
					}
				}
				else if (child->IsDragOver())
				{
					// Leave
					child->OnDragLeave();
				}
			}
		}

		return result;
	}

	void ContainerControl::OnDragLeave()
	{
		// Base
		Control::OnDragLeave();

		// Check all children collisions with mouse and fire events for them
		for (int i = 0; i < m_Children.Count() && m_Children.Count() > 0; i++)
		{
			Control* child = m_Children[i];
			if (child->IsDragOver())
			{
				// Leave
				child->OnDragLeave();
			}
		}
	}

	DragDropEffect ContainerControl::OnDragDrop(const Float2& location, DragData* data)
	{
		// Base
		DragDropEffect result = Control::OnDragDrop(location, data);
		Float2 childLocation;
		// Check all children collisions with mouse and fire events for them
		for (int i = m_Children.Count() - 1; i >= 0 && m_Children.Count() > 0; i--)
		{
			Control* child = m_Children[i];
			if (child->Visible && child->Enabled)
			{
				if (IntersectsChildContent(child, location, childLocation))
				{
					// Enter
					result = child->OnDragDrop(childLocation, data);
					if (result != DragDropEffect::None)
						break;
				}
			}
		}

		return result;
	}

	void ContainerControl::OnSizeChanged()
	{
		// Lock updates to prevent additional layout calculations
		bool wasLayoutLocked = GetIsLayoutLocked();
		SetIsLayoutLocked(true);

		Control::OnSizeChanged();

		// Fire event
		for (int i = 0; i < m_Children.Count(); i++)
		{
			m_Children[i]->OnParentResized();
		}

		// Restore state
		SetIsLayoutLocked(wasLayoutLocked);

		// Arrange child controls
		PerformLayout();
	}
} // SE