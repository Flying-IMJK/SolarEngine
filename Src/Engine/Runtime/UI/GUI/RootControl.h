#pragma once

#include "ContainerControl.h"

namespace SE
{
    class SE_API_RUNTIME RootControl : public ContainerControl
    {
        SE_CLASS(RootControl, ContainerControl)
    private:
        static ContainerControl* _gameRoot;
        // static CanvasContainer _canvasContainer = new CanvasContainer();
    public:

        RootControl();

        /// <summary>
        /// Gets the main GUI control (it can be window or editor overriden control). Use it to plug-in custom GUI controls.
        /// </summary>
        static ContainerControl* GetGameRoot() { return _gameRoot; }
        static void SetGameRoot(ContainerControl* value)
        {
            _gameRoot = value;
            // _canvasContainer.Parent = _gameRoot;
        }

        /// <summary>
        /// Gets the canvas controls root container.
        /// </summary>
        // static CanvasContainer CanvasRoot => _canvasContainer;

        /// <summary>
        /// Gets or sets the current focused control
        /// </summary>
        virtual Control* GetFocusedControl() = 0;
        virtual void SetFocusedControl(Control* value) = 0;

        /// <summary>
        /// Gets the tracking mouse offset.
        /// </summary>
        virtual Float2 GetTrackingMouseOffset() = 0;

        /// <summary>
        /// Gets or sets the position of the mouse in the window space coordinates.
        /// </summary>
        virtual Float2 GetMousePosition() = 0;
        virtual void SetMousePosition(Float2 value) = 0;

        /// <summary>
        /// The update callbacks collection. Controls can register for this to get the update event for logic handling.
        /// </summary>
        List<UpdateDelegate*> UpdateCallbacks = List<UpdateDelegate*>(1024);

        /// <summary>
        /// The update callbacks to add before invoking the update.
        /// </summary>
        List<UpdateDelegate*> UpdateCallbacksToAdd;

        /// <summary>
        /// The update callbacks to remove before invoking the update.
        /// </summary>
        List<UpdateDelegate*> UpdateCallbacksToRemove;

        #pragma region Navigation

        /// <summary>
        /// The custom callback function for UI navigation. Can be used to override the default behaviour.
        /// </summary>
        Delegate<NavDirection> CustomNavigation;

        /// <summary>
        /// Performs the UI navigation.
        /// </summary>
        /// <param name="direction">The navigation direction.</param>
        void Navigate(NavDirection direction);

        /// <summary>
        /// Submits the currently focused control.
        /// </summary>
        void SubmitFocused();

        #pragma endregion

        /// <inheritdoc />
        void Update(float deltaTime) override;

        /// <inheritdoc />
        bool RayCast(Float2& location, Control*& hit) override;

        /// <inheritdoc />
        bool ContainsPoint(Float2& location, bool precise = false) override;

        /// <summary>
        /// Starts the mouse tracking. Used by the scrollbars, splitters, etc.
        /// </summary>
        /// <param name="control">The target control that want to track mouse. It will receive OnMouseMove event.</param>
        /// <param name="useMouseScreenOffset">If set to <c>true</c> will use mouse screen offset.</param>
        virtual void StartTrackingMouse(Control* control, bool useMouseScreenOffset) = 0;

        /// <summary>
        /// Ends the mouse tracking.
        /// </summary>
        virtual void EndTrackingMouse() = 0;

        /// <summary>
        /// Gets keyboard key state.
        /// </summary>
        /// <param name="key">Key to check.</param>
        /// <returns>True while the user holds down the key identified by id.</returns>
        virtual bool GetKey(KeyboardKeys key) = 0;

        /// <summary>
        /// Gets keyboard key down state.
        /// </summary>
        /// <param name="key">Key to check.</param>
        /// <returns>True during the frame the user starts pressing down the key.</returns>
        virtual bool GetKeyDown(KeyboardKeys key) = 0;

        /// <summary>
        /// Gets keyboard key up state.
        /// </summary>
        /// <param name="key">Key to check.</param>
        /// <returns>True during the frame the user releases the button.</returns>
        virtual bool GetKeyUp(KeyboardKeys key) = 0;

        /// <summary>
        /// Gets mouse button state.
        /// </summary>
        /// <param name="button">Mouse button to check.</param>
        /// <returns>True while the user holds down the button.</returns>
        virtual bool GetMouseButton(MouseButton button) = 0;

        /// <summary>
        /// Gets mouse button down state.
        /// </summary>
        /// <param name="button">Mouse button to check.</param>
        /// <returns>True during the frame the user starts pressing down the button.</returns>
        virtual bool GetMouseButtonDown(MouseButton button) = 0;

        /// <summary>
        /// Gets mouse button up state.
        /// </summary>
        /// <param name="button">Mouse button to check.</param>
        /// <returns>True during the frame the user releases the button.</returns>
        virtual bool GetMouseButtonUp(MouseButton button) = 0;

    protected:
        /// <inheritdoc />
        RootControl* __GetRoot() override { return this; }
    };
} // SE

