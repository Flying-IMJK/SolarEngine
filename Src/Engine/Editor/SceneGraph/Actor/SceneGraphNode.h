#pragma once

#include "Editor/SceneGraph/ActorGraphNode.h"

namespace SE
{
	class Scene;
}

namespace SE::Editor
{

	class SceneGraphNode : public ActorGraphNode
	{
		SE_DEFINE_CLASS_DEFAULT(SceneGraphNode, ActorGraphNode)
	private:
		 bool _isEdited;

	public:
		/// <summary>
		/// Gets or sets a value indicating whether this scene is edited.
		/// </summary>
		PRO(IsEdited, SceneGraphNode, bool, __GetIsEdited, __SetIsEdited);

		/// <summary>
		/// Gets the scene.
		/// </summary>
		PRO_GET(scene, SceneGraphNode, Scene*, __GetScene);

		/// <summary>
		/// Initializes a new instance of the <see cref="SceneNode"/> class.
		/// </summary>
		/// <param name="scene">The scene.</param>
		SceneGraphNode(Scene* scene);

	private:

		bool __GetIsEdited() { return _isEdited; }
		void __SetIsEdited(bool value);
		Scene* __GetScene();
	};

} // SE

