using System;
using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Managed content-browser window. The browser presentation is C# owned;
    /// asset import and database operations remain behind generated/native APIs.
    /// </summary>
    public sealed class ContentWindow : EditorWindow
    {
        private readonly ManagedContentBrowser m_Browser;

        public ContentWindow(Editor editor)
            : base(editor, "Content", "Content")
        {
            m_Browser = AddChild(new ManagedContentBrowser(Environment.CurrentDirectory));
        }

        public ManagedContentBrowser Browser => m_Browser;

        public void Refresh()
        {
            m_Browser.Refresh();
        }

        protected override void OnLayoutChildren()
        {
            base.OnLayoutChildren();
            m_Browser.SetBounds(0.0f, 0.0f, Width, Height);
        }
    }
}
