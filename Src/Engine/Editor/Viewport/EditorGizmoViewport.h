#pragma once
#include "EditorViewport.h"

namespace SE::Editor
{
	class RootGraphNode;
	class FPSCamera;

	class EditorGizmoViewport : public EditorViewport
	{
	private:
		RootGraphNode* SceneGraphRoot;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="EditorGizmoViewport"/> class.
		/// </summary>
		/// <param name="task">The task.</param>
		/// <param name="undo">The undo.</param>
		/// <param name="sceneGraphRoot">The scene graph root.</param>
		EditorGizmoViewport(SceneRenderTask* task/*, Undo undo*/, RootGraphNode* sceneGraphRoot);
	};

} // SE
