// Adapted from Flax Editor. Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using SE.Editor.SceneGraph;

namespace SE.Editor.Gizmo
{
    public interface IGizmoOwner
    {
        EditorGizmoViewport Viewport { get; }
        GizmosCollection Gizmos { get; }
        GizmoRenderer Renderer { get; }
        SceneRenderTask RenderTask { get; }
        RootGraphNode SceneGraphRoot { get; }
        Ray MouseRay { get; }
        Float2 MousePosition { get; }
        bool IsControlDown { get; }
        bool UseSnapping { get; }

        // UndoStack integration is migrated but intentionally disabled.
        // void BeginTransform();
        // void EndTransform();

        void Select(List<ScenesGraphNode> nodes);
    }
}
