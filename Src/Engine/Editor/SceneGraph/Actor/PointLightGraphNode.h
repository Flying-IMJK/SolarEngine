#pragma once
#include "Editor/SceneGraph/ActorGraphNode.h"

namespace SE::Editor
{

	class PointLightGraphNode : public ActorGraphNode
	{
		SE_CLASS_DEFAULT(PointLightGraphNode, ActorGraphNode)
	public:
		PointLightGraphNode(::SE::Actor* actor);
	};

} // SE

