
#include "RootControl.h"

#include "Core/Types/Delegate.h"

namespace SE
{
	ContainerControl* RootControl::_gameRoot;

	RootControl::RootControl() : ContainerControl(0, 0, 100, 60)
	{
		AutoFocus = false;
	}

	void RootControl::Navigate(NavDirection direction)
	{
		if (direction == NavDirection::None)
			return;

		if (CustomNavigation.IsBinded())
		{
			// Custom
			CustomNavigation(direction);
			return;
		}

		List<Control*> visited;

		Control* focused = GetFocusedControl();
		if (focused == nullptr)
		{
			// Nothing is focused so go to the first control
			focused = OnNavigate(direction, Float2::Zero, this, visited);
			if (focused != nullptr)
			{
				focused->NavigationFocus();
			}
			return;
		}

		Control* target = focused->GetNavTarget(direction);
		if (target != nullptr)
		{
			// Explicitly specified focus target
			target->NavigationFocus();
			return;
		}

		// Automatic navigation routine
		target = focused->OnNavigate(direction, focused->GetNavOrigin(direction), this, visited);
		if (target != nullptr)
		{
			target->NavigationFocus();
		}
	}

	void RootControl::SubmitFocused()
	{
		Control* focusedControl = GetFocusedControl();
		if (focusedControl != nullptr)
		{
			focusedControl->OnSubmit();
		}
	}

	void RootControl::Update(float deltaTime)
	{
		ContainerControl::Update(deltaTime);

		// Flush requests
		for (int i = 0; i < UpdateCallbacksToAdd.Count(); i++)
		{
			UpdateCallbacks.Add(UpdateCallbacksToAdd[i]);
		}
		UpdateCallbacksToAdd.Clear();
		for (int i = 0; i < UpdateCallbacksToRemove.Count(); i++)
		{
			UpdateCallbacks.Remove(UpdateCallbacksToRemove[i]);
		}
		UpdateCallbacksToRemove.Clear();

		// Perform the UI update
		for (int i = 0; i < UpdateCallbacks.Count(); i++)
		{
			UpdateCallbacks[i]->operator()(deltaTime);
		}
	}

	bool RootControl::RayCast(Float2& location, Control*& hit)
	{
		// Ignore self
		return RayCastChildren(location, hit);
	}

	bool RootControl::ContainsPoint(Float2& location, bool precise)
	{
		if (precise) // Ignore as utility-only element
		{
			return false;
		}
		return Control::ContainsPoint(location, precise);
	}
} // SE