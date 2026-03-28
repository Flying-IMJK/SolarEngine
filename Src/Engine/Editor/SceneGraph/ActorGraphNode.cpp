
#include "ActorGraphNode.h"

#include "ActorChildNode.h"
#include "Editor/GUI/Tree/ActorTreeNode.h"
#include "Editor/Modules/SceneGraphModule.h"
#include "Runtime/Level/Actor.h"
#include "Runtime/Level/Scene/Scene.h"
#include "runtime/ui/gui/rootcontrol.h"

namespace SE::Editor
{
	String ActorGraphNode::__GetName()
	{
		return _actor->GetName();;
	}

	ScenesGraphNode* ActorGraphNode::__GetParentScene()
	{
		Scene* scene = _actor ? _actor->GetScene() : nullptr;
		return scene != nullptr ? TypeCast<ScenesGraphNode>(EditorApp::Ins().sceneGraphModule->FindNode(scene->GetInstanceID())) : nullptr;
	}

	ActorGraphNode* ActorGraphNode::Find(::SE::Actor* actor)
	{
		// Check itself
		if (_actor == actor)
			return this;

		// Check deeper
		for (int i = 0; i < ChildNodes.Count(); i++)
		{
			ActorGraphNode* node;
			if (TypeTryCast(ChildNodes[i], node))
			{
				ActorGraphNode* result = node->Find(actor);
				if (result != nullptr)
				{
					return result;
				}
			}
		}

		return nullptr;
	}

	ActorChildNode* ActorGraphNode::AddChildNode(ActorChildNode* node)
	{
		ActorChildNodes.Add(node);
		node->ParentNode = this;
		return node;
	}

	void ActorGraphNode::DisposeChildNodes()
	{
		// Send event to root so if any of this child nodes is selected we can handle it
		RootControl* root = nullptr ;//Root;
		if (root != nullptr)
		{
			// root->OnActorChildNodesDispose(this);
		}

		if (!ActorChildNodes.IsEmpty())
		{
			for (int i = 0; i < ActorChildNodes.Count(); i++)
			{
				ActorChildNode* node = ActorChildNodes[i];
				node->Dispose();
				Delete(node);
			}
			ActorChildNodes.Clear();
		}
	}

	ActorGraphNode* ActorGraphNode::FindChildActor(::SE::Actor* actor)
	{
		for (int i = 0; i < ChildNodes.Count(); i++)
		{
			ActorGraphNode* node;
			if (TypeTryCast(ChildNodes[i], node) && node->Actor == actor)
			{
				return node;
			}
		}

		return nullptr;
	}
	Object* ActorGraphNode::GetEditableObject()
	{
		return _actor;
	}

	void ActorGraphNode::LinkTreeNode()
	{
		_treeNode->LinkNode(this);
	}

	ActorGraphNode::ActorGraphNode() : ScenesGraphNode(UID::New())
	{

	}

	ActorGraphNode::ActorGraphNode(::SE::Actor* actor) : ScenesGraphNode(actor->GetInstanceID())
	{
		_actor = actor;
		_treeNode = New<ActorTreeNode>();
	}

	ActorGraphNode::ActorGraphNode(::SE::Actor* actor, UID id) : ScenesGraphNode(id)
	{
		_actor = actor;
		_treeNode = New<ActorTreeNode>();
	}

	ActorGraphNode::ActorGraphNode(::SE::Actor* actor, ActorTreeNode* treeNode) : ScenesGraphNode(actor->GetInstanceID())
	{
		_actor = actor;
		_treeNode = treeNode;
	}

	void ActorGraphNode::OnParentChanged()
	{
		_treeNode->OnParentChanged(_actor, TypeCast<ActorGraphNode>(parentNode));
	}
} // SE