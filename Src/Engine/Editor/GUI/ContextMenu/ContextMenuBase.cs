#nullable disable annotations

using System.Collections.Generic;
using SE.GUI;

namespace SE.Editor.GUI
{
    public enum ContextMenuDirection
    {
        RightDown,
        RightUp,
        LeftDown,
        LeftUp,
    }

    public class ContextMenuBase : ContainerControl
    {
        private ContextMenuDirection m_Direction;
        private ContextMenuBase m_ParentCM;
        private ContextMenuBase m_ChildCM;
        private bool m_IsSubMenu;
        private SE.Window m_Window;
        private Control m_PreviouslyFocused;

        /// <summary>
        /// Gets a value indicating whether use automatic popup direction fix based on the screen dimensions.
        /// </summary>
        protected virtual bool UseAutomaticDirectionFix()
        {
            return true;
        }

        /// <summary>
        /// Returns true if context menu is opened
        /// </summary>
        public bool IsOpened => Parent != null;

        /// <summary>
        /// Gets the popup direction.
        /// </summary>
        public ContextMenuDirection Direction => m_Direction;

        /// <summary>
        /// Gets a value indicating whether any child context menu has been opened.
        /// </summary>
        public bool HasChildCMOpened => m_ChildCM != null;

        /// <summary>
        /// Gets the topmost context menu.
        /// </summary>
        public ContextMenuBase TopmostCM => GetTopmostCM();

        /// <summary>
        /// Gets a value indicating whether this context menu is a sub-menu. Sub menus are treated like child context
        /// menus of the other menu (eg. hierarchy).
        /// </summary>
        public bool IsSubMenu => m_IsSubMenu;

        /// <summary>
        /// External dialog popups opened within the context window (eg. color picker) that should preserve context menu
        /// visibility (prevent from closing context menu).
        /// </summary>
        public List<SE.Window> ExternalPopups = new List<SE.Window>();

        /// <summary>
        /// Shows the empty menu popup o na screen.
        /// </summary>
        /// <param name="control">The target control.</param>
        /// <param name="area">The target control area to cover.</param>
        /// <returns>Created popup.</returns>
        public static ContextMenuBase ShowEmptyMenu(Control control, Rectangle area)
        {
            // Calculate the control size in the window space to handle scaled controls
            Float2 upperLeft = control.PointToWindow(area.UpperLeft);
            Float2 bottomRight = control.PointToWindow(area.BottomRight);
            Float2 size = bottomRight - upperLeft;

            ContextMenuBase popup = new ContextMenuBase();
            popup.Size = size;
            popup.Show(control, area.Location + new Float2(0, (size.Y - popup.Height) * 0.5f));
            return popup;
        }

        /// <summary>
        /// Show context menu over given control.
        /// </summary>
        /// <param name="parent">Parent control to attach to it.</param>
        /// <param name="location">Popup menu origin location in parent control coordinates.</param>
        public virtual void Show(Control parent, Float2 location)
        {
            SE.Debug.Assert(parent != null, "parent != null");

            // Ensure to be closed
            Hide();

            // Peek parent control window
            WindowRootControl parentWin = parent.RootWindow();
            if (parentWin == null)
            {
                return;
            }

            // Check if show menu inside the other menu - then link as a child to prevent closing the calling menu
            // window on lost focus
            if (m_ParentCM == null && parentWin.ChildrenCount == 1)
            {
                if (parentWin.Children[0] is ContextMenuBase parentCM)
                {
                    parentCM.ShowChild(this, parentCM.PointFromScreen(parent.PointToScreen(location)), false);
                    return;
                }
            }

            // Unlock and perform controls update
            Location = Float2.Zero;
            UnlockChildrenRecursive();
            PerformLayout();

            // Calculate popup direction and initial location (fit on a single monitor)
            float dpiScale = parentWin.Window.DpiScale;
            Float2 dpiSize = Size * dpiScale;
            Float2 locationWS = parent.PointToWindow(location);
            Float2 locationSS = parentWin.PointToScreen(locationWS);
            Rectangle monitorBounds = Platform.GetMonitorBounds(locationSS);
            Float2 rightBottomLocationSS = locationSS + dpiSize;
            bool isUp = false;
            bool isLeft = false;
            if (UseAutomaticDirectionFix())
            {
                ContextMenu parentContextMenu = (parent as object) as ContextMenu;
                ContextMenuBase parentMenu = (parentContextMenu as object) as ContextMenuBase;
                if (monitorBounds.Bottom < rightBottomLocationSS.Y)
                {
                    isUp = true;
                    locationSS.Y -= dpiSize.Y;
                    if (parentMenu != null && parentMenu.m_ChildCM != null)
                    {
                        locationSS.Y += 30.0f * dpiScale;
                    }
                }

                if (parentMenu == null)
                {
                    if (monitorBounds.Right < rightBottomLocationSS.X)
                    {
                        isLeft = true;
                        locationSS.X -= dpiSize.X;
                    }
                }
                else if (monitorBounds.Right < rightBottomLocationSS.X ||
                         (m_ParentCM != null && (m_ParentCM.Direction == ContextMenuDirection.LeftDown || m_ParentCM
                             .Direction == ContextMenuDirection.LeftUp)))
                {
                    isLeft = true;
                    if (IsSubMenu && m_ParentCM != null)
                    {
                        locationSS.X -= m_ParentCM.Width + dpiSize.X;
                    }
                    else
                    {
                        locationSS.X -= dpiSize.X;
                    }
                }
            }

            // Update direction flag
            if (isUp)
            {
                m_Direction = isLeft ? ContextMenuDirection.LeftUp : ContextMenuDirection.RightUp;
            }
            else
            {
                m_Direction = isLeft ? ContextMenuDirection.LeftDown : ContextMenuDirection.RightDown;
            }

            // Create window
            CreateWindowSettings desc = SE.Window.CreateDefaultSettings();
            desc.Position = locationSS;
            desc.StartPosition = SE.WindowStartPosition.Manual;
            desc.Size = dpiSize;
            desc.Fullscreen = false;
            desc.HasBorder = false;
            desc.SupportsTransparency = false;
            desc.ShowInTaskbar = false;
            desc.ActivateWhenFirstShown = true;
            desc.AllowInput = true;
            desc.AllowMinimize = false;
            desc.AllowMaximize = false;
            desc.AllowDragAndDrop = false;
            desc.IsTopmost = true;
            desc.IsRegularWindow = false;
            desc.HasSizingFrame = false;
            OnWindowCreating(desc);
            m_Window = SE.Window.Create(desc);
            m_Window.GetFocus += OnWindowGotFocus;
            m_Window.LostFocus += OnWindowLostFocus;

            // Attach to the window
            m_ParentCM = parent as ContextMenuBase;
            Parent = m_Window.GUI;

            // Show
            Visible = true;
            if (m_Window == null)
            {
                return;
            }
            m_Window.Show();
            PerformLayout();
            m_PreviouslyFocused = parentWin.GetFocusedControl();
            Focus();
            OnShow();
        }

        /// <summary>
        /// Hide popup menu and all child menus.
        /// </summary>
        public virtual void Hide()
        {
            if (!Visible)
            {
                return;
            }

            // Lock update
            IsLayoutLocked = true;

            // Close child
            HideChild();

            // Unlink from window
            Parent = null;

            // Close window
            if (m_Window != null)
            {
                SE.Window win = m_Window;
                m_Window = null;
                win.Close();
            }

            // Unlink from parent
            if (m_ParentCM != null)
            {
                m_ParentCM.m_ChildCM = null;
                m_ParentCM = null;
            }

            // Return focus
            if (m_PreviouslyFocused != null)
            {
                if (m_PreviouslyFocused.RootWindow() != null)
                {
                    m_PreviouslyFocused.RootWindow().Focus();
                }

                m_PreviouslyFocused.Focus();
                m_PreviouslyFocused = null;
            }

            // Hide
            Visible = false;
            OnHide();
        }

        /// <summary>
        /// Shows new child context menu.
        /// </summary>
        /// <param name="child">The child menu.</param>
        /// <param name="location">The child menu initial location.</param>
        /// <param name="isSubMenu"> True if context menu is a normal sub-menu, otherwise it is a custom menu popup
        /// linked as child.</param>
        public void ShowChild(ContextMenuBase child, Float2 location, bool isSubMenu = true)
        {
            // Hide current child
            HideChild();

            // Set child
            m_ChildCM = child;
            m_ChildCM.m_ParentCM = this;
            m_ChildCM.m_IsSubMenu = isSubMenu;

            // Show child
            m_ChildCM.Show(this, location);
        }

        /// <summary>
        /// Hides child popup menu if any opened.
        /// </summary>
        public void HideChild()
        {
            if (m_ChildCM != null)
            {
                m_ChildCM.Hide();
                m_ChildCM = null;
            }
        }

        /// <inheritdoc />
        public override void Draw()
        {
            // Draw background
            Style style = Style.Current;
            Rectangle bounds = new Rectangle(Float2.Zero, Size);
            Render2D.FillRectangle(bounds, style.Background);
            Render2D.DrawRectangle(bounds, Color.Lerp(style.BackgroundSelected, style.Background, 0.6f));

            base.Draw();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            base.OnMouseDown(location, button);
            return true;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            base.OnMouseUp(location, button);
            return true;
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (base.OnKeyDown(key))
            {
                return true;
            }

            switch (key)
            {
                case KeyboardKeys.Escape:
                    Hide();
                    return true;
            }
            return false;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            // Ensure to be hidden
            Hide();

            base.OnDestroy();
        }

        public ContextMenuBase()
            : base(0, 0, 120, 32)
        {
            m_Direction = ContextMenuDirection.RightDown;
            Visible = false;

            m_IsSubMenu = true;
        }

        /// <summary>
        /// Updates the size of the window to match context menu dimensions.
        /// </summary>
        protected void UpdateWindowSize()
        {
            if (m_Window != null)
            {
                m_Window.ClientSize = Size * m_Window.DpiScale;
            }
        }

        /// <summary>
        /// Called when context menu window setup is performed. Can be used to adjust the popup window options.
        /// </summary>
        /// <param name="settings">The settings.</param>
        protected virtual void OnWindowCreating(CreateWindowSettings settings)
        {
        }

        /// <summary>
        /// Called on context menu show.
        /// </summary>
        protected virtual void OnShow()
        {
            // Nothing to do
        }

        /// <summary>
        /// Called on context menu hide.
        /// </summary>
        protected virtual void OnHide()
        {
            // Nothing to do
        }

        private void OnWindowGotFocus()
        {
        }

        private void OnWindowLostFocus()
        {
            // Skip for parent menus (child should handle lost of focus)
            if (m_ChildCM != null)
            {
                return;
            }

            if (m_ParentCM != null)
            {
                if (IsMouseOver)
                {
                    return;
                }

                foreach (SE.Window externalPopup in ExternalPopups)
                {
                    if (externalPopup != null && externalPopup.IsFocused)
                    {
                        return;
                    }
                }

                // Check if mouse is over any of the parents
                ContextMenuBase focusCM = null;
                ContextMenuBase cm = m_ParentCM;
                while (cm != null)
                {
                    if (cm.IsMouseOver)
                    {
                        focusCM = cm;
                    }
                    cm = cm.m_ParentCM;
                }

                if (focusCM != null)
                {
                    // Focus on the clicked parent and hide any open sub-menus
                    focusCM.HideChild();
                    if (focusCM.m_Window != null)
                    {
                        focusCM.m_Window.Show();
                    }
                }
                else
                {
                    // User clicked outside the context menus, hide the whole context menu tree
                    TopmostCM.Hide();
                }
            }
            else if (!IsMouseOver)
            {
                Hide();
            }
        }

        private ContextMenuBase GetTopmostCM()
        {
            ContextMenuBase cm = this;
            while (cm.m_ParentCM != null && cm.m_IsSubMenu)
            {
                cm = cm.m_ParentCM;
            }
            return cm;
        }

        /// <inheritdoc />
        public override bool IsMouseOver
        {
            get
            {
                bool result = false;
                for (int i = 0; i < m_Children.Count; i++)
                {
                    Control child = m_Children[i];
                    if (child.Visible && child.IsMouseOver)
                    {
                        result = true;
                        break;
                    }
                }
                return result;
            }
        }
    }
}
