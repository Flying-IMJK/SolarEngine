using SE.GUI;

namespace SE.Editor.GUI
{
    /// <summary>
    /// Managed scene editing window. The viewport surface is deliberately kept
    /// independent of the C++ editor viewport until SceneRenderTask receives a
    /// generated C# binding.
    /// </summary>
    public sealed class EditSceneWindow : SceneEditorWindow
    {
        private readonly Label m_StatusLabel;

        public EditSceneWindow(Editor editor)
            : base(editor, "EditScene", "Edit Scene", ScrollBars.None)
        {
            m_StatusLabel = AddChild(new Label());
            m_StatusLabel.AutoFocus = false;
            m_StatusLabel.HorizontalAlignment = TextAlignment.Center;
            m_StatusLabel.VerticalAlignment = TextAlignment.Center;
            UpdateStatus();
        }

        public override void OnUpdate()
        {
            UpdateStatus();
        }

        protected override void OnLayoutChildren()
        {
            base.OnLayoutChildren();
            m_StatusLabel.SetBounds(0.0f, 0.0f, Width, Height);
        }

        private void UpdateStatus()
        {
            int selectionCount = Editor.Scene.SelectionCount;
            m_StatusLabel.Text = selectionCount == 0
                ? "Scene viewport is waiting for the generated SceneRenderTask binding."
                : $"{selectionCount} scene object(s) selected. Scene viewport is waiting for the generated SceneRenderTask binding.";
        }
    }
}
