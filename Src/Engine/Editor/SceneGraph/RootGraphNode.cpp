
#include "RootGraphNode.h"

#include "Editor/GUI/Tree/ActorTreeNode.h"

namespace SE::Editor
{
	::SE::Transform RootGraphNode::__GetTransform()
	{
		return Transform::Identity;
	}

	void RootGraphNode::__SetTransform(::SE::Transform value)
	{

	}

	RootGraphNode::RootGraphNode() : ActorGraphNode(nullptr, UID::New())
	{
		_treeNode->AutoFocus = false;
	}
	RootGraphNode::RootGraphNode(UID id) : ActorGraphNode(nullptr, id)
	{
		_treeNode->AutoFocus = false;
	}
} // SE