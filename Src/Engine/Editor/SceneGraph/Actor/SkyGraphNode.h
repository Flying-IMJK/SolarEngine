#pragma once
#include "Editor/SceneGraph/ActorGraphNode.h"

namespace SE::Editor
{

	class SkyGraphNode : public ActorGraphNode
	{
		SE_CLASS_DEFAULT(SkyGraphNode, ActorGraphNode)
	public:
		SkyGraphNode(::SE::Actor* actor);
	};

} // SE
