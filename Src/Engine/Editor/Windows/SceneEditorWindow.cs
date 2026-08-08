using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Base type for windows that consume the managed scene and selection model.
    /// </summary>
    public abstract class SceneEditorWindow : EditorWindow
    {
        protected SceneEditorWindow(Editor editor, string id, string title, ScrollBars scrollBars = ScrollBars.None)
            : base(editor, id, title, scrollBars)
        {
        }
    }
}
