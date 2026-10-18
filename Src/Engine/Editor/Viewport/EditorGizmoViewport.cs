
using SE.Editor.SceneGraph;

namespace SE.Editor
{
    /// <summary>
    /// Editor viewport with scene gizmos.
    /// </summary>
    public class EditorGizmoViewport : EditorViewport
    {
        private RootGraphNode SceneGraphRoot;

        /// <summary>
        /// Initializes a new instance of the <see cref="EditorGizmoViewport"/> class.
        /// </summary>
        /// <param name="task">The task.</param>
        /// <param name="sceneGraphRoot">The scene graph root.</param>
        public EditorGizmoViewport(SceneRenderTask task, RootGraphNode sceneGraphRoot)
            : base(task, new FPSCamera(), true)
        {
            // Undo = undo;
            SceneGraphRoot = sceneGraphRoot;
            // Gizmos = new GizmosCollection(this);
            // SetUpdate(_update, OnUpdate);
        }
    }
}
