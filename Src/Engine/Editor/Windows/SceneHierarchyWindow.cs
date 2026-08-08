using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Displays the managed SceneModule hierarchy and synchronizes its selection.
    /// </summary>
    public sealed class SceneHierarchyWindow : SceneEditorWindow
    {
        private readonly SceneHierarchyView m_HierarchyView;

        public SceneHierarchyWindow(Editor editor)
            : base(editor, "SceneHierarchy", "Scene Hierarchy", ScrollBars.None)
        {
            m_HierarchyView = AddChild(new SceneHierarchyView(editor.Scene));
        }

        public SceneHierarchyView HierarchyView => m_HierarchyView;

        protected override void OnLayoutChildren()
        {
            base.OnLayoutChildren();
            m_HierarchyView.SetBounds(0.0f, 0.0f, Width, Height);
        }
    }
}
