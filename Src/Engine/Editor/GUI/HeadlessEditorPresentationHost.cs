// Managed editor GUI feature implementation.
using SE.Editor;

namespace SE.Editor.GUI
{
    public sealed class HeadlessEditorPresentationHost : IEditorPresentationHost
    {
        public void Initialize(EditorApplication editor)
        {
            Debug.Log("C# Editor presentation host initialized without legacy SGUI backend");
        }

        public void AddWindow(EditorWindow window)
        {
        }

        public void RemoveWindow(EditorWindow window)
        {
        }

        public void FocusWindow(EditorWindow window)
        {
        }

        public void ShowWindow(EditorWindow window)
        {
        }

        public void HideWindow(EditorWindow window)
        {
        }

        public void Update()
        {
        }

        public void Render()
        {
        }

        public void Shutdown()
        {
        }
    }
}
