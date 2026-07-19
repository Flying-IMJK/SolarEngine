#pragma once
#include "Editor/SceneGraph/ActorGraphNode.h"

namespace SE::Editor
{

	SE_CLASS(Reflect)
	class PointLightGraphNode : public ActorGraphNode
	{
		SE_DEFINE_CLASS_DEFAULT(PointLightGraphNode, ActorGraphNode)
	public:
		PointLightGraphNode(::SE::Actor* actor);
	};

} // SE

