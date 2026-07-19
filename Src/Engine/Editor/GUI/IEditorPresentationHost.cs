// Managed editor GUI feature implementation.
using SE.Editor;

namespace SE.Editor.GUI
{
    public interface IEditorPresentationHost
    {
        void Initialize(EditorApplication editor);
        void AddWindow(EditorWindow window);
        void RemoveWindow(EditorWindow window);
        void FocusWindow(EditorWindow window);
        void ShowWindow(EditorWindow window);
        void HideWindow(EditorWindow window);
        void Update();
        void Render();
        void Shutdown();
    }
}
