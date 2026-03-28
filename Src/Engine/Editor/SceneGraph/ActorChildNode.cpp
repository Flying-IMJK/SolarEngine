

#include "ActorChildNode.h"

#include "ActorGraphNode.h"
#include "Core/Types/Strings/String.h"

namespace SE::Editor
{
	String ActorChildNode::__GetName()
	{
		return String::Format(SE_TEXT("{0}.{1}"), ParentNode->Name.operator->(), Index);
	}
	void ActorChildNode::Dispose()
	{
		ActorGraphNode* parentActorNode;
		// Unlink from the parent
		if (TypeTryCast(parentNode, parentActorNode) && !parentActorNode->ActorChildNodes.IsEmpty())
		{
			parentActorNode->ActorChildNodes.Remove(this);
		}

		ScenesGraphNode::Dispose();
	}

} // SE