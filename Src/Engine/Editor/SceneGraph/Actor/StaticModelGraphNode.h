#pragma once
#include "Editor/SceneGraph/ActorGraphNode.h"

namespace SE::Editor
{

	class StaticModelGraphNode : public ActorGraphNode
	{
		SE_CLASS_DEFAULT(StaticModelGraphNode, ActorGraphNode)
	public:
		StaticModelGraphNode(::SE::Actor* actor);
	};

} // SE
