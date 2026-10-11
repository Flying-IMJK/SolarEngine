#nullable disable annotations

using System;
using System.Collections.Generic;

namespace SE.GUI
{
    /// <summary>
    /// Root of a GUI tree.
    /// </summary>
    public abstract class RootControl : ContainerControl
    {
        private static ContainerControl s_GameRoot;

        public RootControl() : base(0, 0, 100, 60)
        {
            AutoFocus = false;
        }

        /// <summary>
        /// Gets the main GUI control. Use it to plug in custom GUI controls.
        /// </summary>
        public static ContainerControl GetGameRoot()
        {
            return s_GameRoot;
        }

        /// <summary>
        /// Sets the main GUI control.
        /// </summary>
        public static void SetGameRoot(ContainerControl value)
        {
            s_GameRoot = value;
        }

        /// <summary>
        /// Gets or sets the currently focused control.
        /// </summary>
        public abstract Control GetFocusedControl();

        public abstract void SetFocusedControl(Control value);

        /// <summary>
        /// Gets the tracking mouse offset.
        /// </summary>
        public abstract Float2 GetTrackingMouseOffset();

        /// <summary>
        /// Gets or sets the position of the mouse in the window space coordinates.
        /// </summary>
        public abstract Float2 GetMousePosition();

        public abstract void SetMousePosition(Float2 value);

        /// <summary>
        /// The update callbacks collection. Controls can register for logic updates.
        /// </summary>
        public List<Action<float>> UpdateCallbacks = new(1024);

        /// <summary>
        /// The update callbacks to add before invoking the update.
        /// </summary>
        public List<Action<float>> UpdateCallbacksToAdd = new();

        /// <summary>
        /// The update callbacks to remove before invoking the update.
        /// </summary>
        public List<Action<float>> UpdateCallbacksToRemove = new();

        #region Navigation

        /// <summary>
        /// The custom callback for UI navigation. Overrides the default navigation behaviour.
        /// </summary>
        public Action<NavDirection> CustomNavigation;

        /// <summary>
        /// Performs UI navigation.
        /// </summary>
        public void Navigate(NavDirection direction)
        {
            if (direction == NavDirection.None)
            {
                return;
            }

            if (CustomNavigation != null)
            {
                // Custom
                CustomNavigation(direction);
                return;
            }

            List<Control> visited = new();

            Control focused = GetFocusedControl();
            if (focused == null)
            {
                // Nothing is focused so go to the first control
                focused = OnNavigate(direction, Float2.Zero, this, visited);
                if (focused != null)
                {
                    focused.NavigationFocus();
                }
                return;
            }

            Control target = focused.GetNavTarget(direction);
            if (target != null)
            {
                // Explicitly specified focus target
                target.NavigationFocus();
                return;
            }

            // Automatic navigation routine
            target = focused.OnNavigate(direction, focused.GetNavOrigin(direction), this, visited);
            if (target != null)
            {
                target.NavigationFocus();
            }
        }

        /// <summary>
        /// Submits the currently focused control.
        /// </summary>
        public void SubmitFocused()
        {
            Control focusedControl = GetFocusedControl();
            if (focusedControl != null)
            {
                focusedControl.OnSubmit();
            }
        }

        #endregion

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            // Flush requests
            for (int i = 0; i < UpdateCallbacksToAdd.Count; i++)
            {
                UpdateCallbacks.Add(UpdateCallbacksToAdd[i]);
            }
            UpdateCallbacksToAdd.Clear();
            for (int i = 0; i < UpdateCallbacksToRemove.Count; i++)
            {
                // Native List<UpdateDelegate*>::Remove compares pointer identity and moves the last item into the gap.
                for (int j = 0; j < UpdateCallbacks.Count; j++)
                {
                    if ((object)UpdateCallbacks[j] == (object)UpdateCallbacksToRemove[i])
                    {
                        int lastIndex = UpdateCallbacks.Count - 1;
                        if (lastIndex != 0)
                        {
                            UpdateCallbacks[j] = UpdateCallbacks[lastIndex];
                        }
                        UpdateCallbacks.RemoveAt(lastIndex);
                        break;
                    }
                }
            }
            UpdateCallbacksToRemove.Clear();

            // Perform the UI update
            for (int i = 0; i < UpdateCallbacks.Count; i++)
            {
                UpdateCallbacks[i](deltaTime);
            }
        }

        /// <inheritdoc />
        public override bool RayCast(ref Float2 location, out Control hit)
        {
            // Ignore self
            return RayCastChildren(location, out hit);
        }

        /// <inheritdoc />
        public override bool ContainsPoint(Float2 location, bool precise = false)
        {
            if (precise) // Ignore as utility-only element
            {
                return false;
            }
            return base.ContainsPoint(location, precise);
        }

        /// <summary>
        /// Starts the mouse tracking. Used by the scrollbars, splitters, etc.
        /// </summary>
        /// <param name="control">The target control that want to track mouse. It will receive OnMouseMove event.</param>
        /// <param name="useMouseScreenOffset">If set to <c>true</c> will use mouse screen offset.</param>
        public abstract void StartTrackingMouse(Control control, bool useMouseScreenOffset);

        /// <summary>
        /// Ends the mouse tracking.
        /// </summary>
        public abstract void EndTrackingMouse();

        /// <summary>
        /// Gets keyboard key state.
        /// </summary>
        /// <param name="key">Key to check.</param>
        /// <returns>True while the user holds down the key identified by id.</returns>
        public abstract bool GetKey(KeyboardKeys key);

        /// <summary>
        /// Gets keyboard key down state.
        /// </summary>
        /// <param name="key">Key to check.</param>
        /// <returns>True during the frame the user starts pressing down the key.</returns>
        public abstract bool GetKeyDown(KeyboardKeys key);

        /// <summary>
        /// Gets keyboard key up state.
        /// </summary>
        /// <param name="key">Key to check.</param>
        /// <returns>True during the frame the user releases the button.</returns>
        public abstract bool GetKeyUp(KeyboardKeys key);

        /// <summary>
        /// Gets mouse button state.
        /// </summary>
        /// <param name="button">Mouse button to check.</param>
        /// <returns>True while the user holds down the button.</returns>
        public abstract bool GetMouseButton(MouseButton button);

        /// <summary>
        /// Gets mouse button down state.
        /// </summary>
        /// <param name="button">Mouse button to check.</param>
        /// <returns>True during the frame the user starts pressing down the button.</returns>
        public abstract bool GetMouseButtonDown(MouseButton button);

        /// <summary>
        /// Gets mouse button up state.
        /// </summary>
        /// <param name="button">Mouse button to check.</param>
        /// <returns>True during the frame the user releases the button.</returns>
        public abstract bool GetMouseButtonUp(MouseButton button);

        /// <inheritdoc />
        public override RootControl Root
        {
            get
            {
                return this;
            }
        }
    }
}
