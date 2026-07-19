#pragma once
#include "Runtime/UI/GUI/ContainerControl.h"

namespace SE
{
    class GraphicWindow;
}

namespace SE::Editor
{

    /// <summary>
	/// Context menu popup directions.
	/// </summary>
	enum class ContextMenuDirection
	{
		/// <summary>
		/// The right down.
		/// </summary>
		RightDown,

		/// <summary>
		/// The right up.
		/// </summary>
		RightUp,

		/// <summary>
		/// The left down.
		/// </summary>
		LeftDown,

		/// <summary>
		/// The left up.
		/// </summary>
		LeftUp,
	};

    SE_CLASS(Reflect)
    class ContextMenuBase : public ContainerControl
    {
        SE_DEFINE_CLASS(ContextMenuBase, ContainerControl)
    private:
        ContextMenuDirection _direction;
        ContextMenuBase* _parentCM;
        ContextMenuBase* _childCM;
        bool _isSubMenu;
        GraphicWindow* _window;
        Control* _previouslyFocused;

        /// <summary>
        /// Gets a value indicating whether use automatic popup direction fix based on the screen dimensions.
        /// </summary>
    protected:
        virtual bool UseAutomaticDirectionFix();

    public:
        /// <summary>
        /// Returns true if context menu is opened
        /// </summary>
        PRO_GET(IsOpened, ContextMenuBase, bool, __GetIsOpened);

        /// <summary>
        /// Gets the popup direction.
        /// </summary>
        PRO_GET(Direction, ContextMenuBase, ContextMenuDirection, __GetDirection);

        /// <summary>
        /// Gets a value indicating whether any child context menu has been opened.
        /// </summary>
        PRO_GET(HasChildCMOpened, ContextMenuBase, bool, __GetHasChildCMOpened);

        /// <summary>
        /// Gets the topmost context menu.
        /// </summary>
        PRO_GET(TopmostCM, ContextMenuBase, ContextMenuBase*, __GetTopmostCM);

        /// <summary>
        /// Gets a value indicating whether this context menu is a sub-menu. Sub menus are treated like child context menus of the other menu (eg. hierarchy).
        /// </summary>
        PRO_GET(IsSubMenu, ContextMenuBase, bool, __GetIsSubMenu);

        /// <summary>
        /// External dialog popups opened within the context window (eg. color picker) that should preserve context menu visibility (prevent from closing context menu).
        /// </summary>
        List<GraphicWindow*> ExternalPopups;

        /// <summary>
        /// Shows the empty menu popup o na screen.
        /// </summary>
        /// <param name="control">The target control.</param>
        /// <param name="area">The target control area to cover.</param>
        /// <returns>Created popup.</returns>
        static ContextMenuBase* ShowEmptyMenu(Control* control, Rectangle area);

        /// <summary>
        /// Show context menu over given control.
        /// </summary>
        /// <param name="parent">Parent control to attach to it.</param>
        /// <param name="location">Popup menu origin location in parent control coordinates.</param>
        virtual void Show(Control* parent, Float2 location);

        /// <summary>
        /// Hide popup menu and all child menus.
        /// </summary>
        virtual void Hide();

        /// <summary>
        /// Shows new child context menu.
        /// </summary>
        /// <param name="child">The child menu.</param>
        /// <param name="location">The child menu initial location.</param>
        /// <param name="isSubMenu">True if context menu is a normal sub-menu, otherwise it is a custom menu popup linked as child.</param>
        void ShowChild(ContextMenuBase* child, Float2 location, bool isSubMenu = true);

        /// <summary>
        /// Hides child popup menu if any opened.
        /// </summary>
        void HideChild();

        /// <inheritdoc />
        void Draw() override;

        /// <inheritdoc />
        bool OnMouseDown(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnMouseUp(Float2 location, MouseButton button) override;

        /// <inheritdoc />
        bool OnKeyDown(KeyboardKeys key) override;

        /// <inheritdoc />
        void OnDestroy() override;

        ContextMenuBase();

    protected:
        /// <summary>
        /// Updates the size of the window to match context menu dimensions.
        /// </summary>
        void UpdateWindowSize();

        /// <summary>
        /// Called when context menu window setup is performed. Can be used to adjust the popup window options.
        /// </summary>
        /// <param name="settings">The settings.</param>
        virtual void OnWindowCreating(CreateWindowSettings settings)
        {
        }

        /// <summary>
        /// Called on context menu show.
        /// </summary>
        virtual void OnShow()
        {
            // Nothing to do
        }

        /// <summary>
        /// Called on context menu hide.
        /// </summary>
        virtual void OnHide()
        {
            // Nothing to do
        }

#if USE_IS_FOREGROUND
        /// <summary>
        /// Returns true if context menu is in foreground (eg. context window or any child window has user focus or user opened additional popup within this context).
        /// </summary>
        protected virtual bool IsForeground
        {
            get
            {
                // Any external popup is focused
                foreach (var externalPopup in ExternalPopups)
                {
                    if (externalPopup && externalPopup.IsForegroundWindow)
                        return true;
                }

                // Any context menu window is focused
                var anyForeground = false;
                var c = this;
                while (!anyForeground && c != null)
                {
                    if (c._window != null && c._window.IsForegroundWindow)
                        anyForeground = true;
                    c = c._childCM;
                }

                return anyForeground;
            }
        }

        private void OnWindowGotFocus()
        {
            var child = _childCM;
            if (child != null && _window && _window.IsForegroundWindow)
            {
                // Hide child if user clicked over parent (do it next frame to process other events before - eg. child windows focus loss)
                FlaxEngine.Scripting.InvokeOnUpdate(() =>
                {
                    if (child == _childCM)
                        HideChild();
                });
            }
        }

        private void OnWindowLostFocus()
        {
            // Skip for parent menus (child should handle lost of focus)
            if (_childCM != null)
                return;

            // Check if user stopped using that popup menu
            if (_parentCM != null)
            {
                // Focus parent if user clicked over the parent popup
                var mouse = _parentCM.PointFromScreen(FlaxEngine.Input.MouseScreenPosition);
                if (_parentCM.ContainsPoint(ref mouse))
                {
                    _parentCM._window.Focus();
                }
            }
        }
#else

    private:
        void OnWindowGotFocus()
        {
        }

        void OnWindowLostFocus();
#endif


#if USE_IS_FOREGROUND
        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            ContainerControl::Update(deltaTime);

            // Let root context menu to check if none of the popup windows
            if (_parentCM == null && !IsForeground)
            {
                Hide();
            }
        }
#endif

    private:
        bool __GetIsOpened()
        {
            return Parent != nullptr;
        }

        ContextMenuDirection __GetDirection()
        {
            return _direction;
        }

        bool __GetHasChildCMOpened()
        {
            return _childCM != nullptr;
        }

        ContextMenuBase* __GetTopmostCM();

        bool __GetIsSubMenu()
        {
            return _isSubMenu;
        }

        bool __GetIsMouseOver() override;

    };

} // SE

