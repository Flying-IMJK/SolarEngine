

#include "EditorGizmoViewport.h"

#include "Cameras/FPSCamera.h"

namespace SE::Editor
{
	EditorGizmoViewport::EditorGizmoViewport(SceneRenderTask* task, RootGraphNode* sceneGraphRoot): EditorViewport(task, New<FPSCamera>(), true)
	{
		// Undo = undo;
		SceneGraphRoot = sceneGraphRoot;
		// Gizmos = new GizmosCollection(this);

		// SetUpdate(_update, OnUpdate);
	}
} // SE