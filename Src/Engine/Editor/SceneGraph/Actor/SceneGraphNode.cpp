
#include "SceneGraphNode.h"

#include "Editor/GUI/Tree/ActorTreeNode.h"
#include "Runtime/Level/Scene/Scene.h"

namespace SE::Editor
{
	SceneGraphNode::SceneGraphNode(Scene* scene) : ActorGraphNode(scene)
	{
	}

	void SceneGraphNode::__SetIsEdited(bool value)
	{
		if (_isEdited != value)
		{
			_isEdited = value;
			_treeNode->UpdateText();
		}
	}
	Scene* SceneGraphNode::__GetScene()
	{
		return (Scene*)_actor;
	}
} // SE