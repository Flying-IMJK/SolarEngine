using System;
using SE.GUI;

namespace SE.Editor
{
    /// <summary>
    /// Base class for managed editor tool windows. It mirrors Flax EditorWindow:
    /// the window owns a focused editor feature and registers itself with the
    /// managed WindowsModule for lifecycle and docking management.
    /// </summary>
    public abstract class EditorWindow : GUI.DockWindow
    {
        protected EditorWindow(Editor editor, bool hideOnClose, ScrollBars scrollBars = ScrollBars.None)
            : base(editor.UI.MasterDockPanel, hideOnClose, scrollBars)
        {
            Editor = editor;
            AutoFocus = true;

            Editor.Windows.RegisterWindow(this);
        }

        /// <summary>
        /// Gets the managed editor root that owns this window.
        /// </summary>
        protected Editor Editor { get; }


        /// <summary>
        /// Determines whether this window is editing a given content item.
        /// </summary>
        public virtual bool IsEditingItem(ContentItem item)
        {
            return false;
        }

        /// <summary>
        /// Called before Editor will enter play mode.
        /// </summary>
        public virtual void OnPlayBeginning()
        {
        }

        /// <summary>
        /// Called when Editor is entering play mode.
        /// </summary>
        public virtual void OnPlayBegin()
        {
        }

        /// <summary>
        /// Called when Editor leaves the play mode.
        /// </summary>
        public virtual void OnPlayEnd()
        {
        }


        /// <summary>
        /// Called once after all default editor windows have been constructed.
        /// </summary>
        public virtual void OnInit()
        {
        }

        /// <summary>
        /// Called on every managed editor update.
        /// </summary>
        public virtual void OnUpdate()
        {
        }

        /// <summary>
        /// Called while the managed editor is shutting down.
        /// </summary>
        public virtual void OnExit()
        {
        }

        /// <summary>
        /// Called when Editor state gets changed.
        /// </summary>
        public virtual void OnEditorStateChanged()
        {
        }


        public override bool OnKeyDown(KeyboardKeys key)
        {
            /*// Prevent closing the editor window when using RMB + Ctrl + W to slow down the camera flight
		    if (Editor.Options.Options.Input.CloseTab.Process(this, key))
		    {
			    if (Root->GetMouseButton(MouseButton::Right))
				    return true;
		    }*/

            if (base.OnKeyDown(key))
            {
                return true;
            }

/*            switch (key)
            {
                case KeyboardKeys.Return:
                    if (CanUseNavigation() && Root != null && Root->GetFocusedControl() != null)
                    {
                        Root->SubmitFocused();
                        return true;
                    }
                    break;
                case KeyboardKeys.Tab:
                    if (CanUseNavigation() && Root != null)
                    {
                        bool shiftDown = Root->GetKey(KeyboardKeys::Shift);
                        Root.Navigate(shiftDown ? NavDirection.Previous : NavDirection.Next);
                        return true;
                    }
                    break;
            }*/
            return false;
        }

        protected override void OnDispose()
        {
            if (IsDisposing)
            {
                return;
            }

            OnExit();

            Editor.Windows.UnregisterWindow(this);
            base.OnDispose();
        }
    }
}
