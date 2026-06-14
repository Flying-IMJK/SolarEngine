#pragma once
#include "Editor/SceneGraph/ActorGraphNode.h"

namespace SE::Editor
{

	class DirectionalLightGraphNode : public ActorGraphNode
	{
		SE_DEFINE_CLASS_DEFAULT(DirectionalLightGraphNode, ActorGraphNode)
	public:
		DirectionalLightGraphNode(::SE::Actor* actor);

		/// <inheritdoc />
		/*void OnDebugDraw(ViewportDebugDrawData data) override
		{
			base.OnDebugDraw(data);

			var transform = Actor.Transform;
			DebugDraw.DrawWireArrow(transform.Translation, transform.Orientation, 1.0f, 0.5f, Color.Red, 0.0f, false);
		}*/
	};

} // SE

