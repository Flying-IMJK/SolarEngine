
#include "ScenesGraphNode.h"

#include "Editor/EditorApp.h"
#include "Editor/Modules/SceneGraphModule.h"

namespace SE::Editor
{
	ScenesGraphNode* ScenesGraphNode::__GetParentNode()
	{
		return parentNode;
	}

	void ScenesGraphNode::__SetParentNode(ScenesGraphNode* value)
	{
		if (parentNode != value)
		{
			if (parentNode != nullptr)
			{
				parentNode->ChildNodes.Remove(this);
			}
			parentNode = value;
			if (parentNode != nullptr)
			{
				parentNode->ChildNodes.Add(this);
			}

			OnParentChanged();
		}
	}

	RootGraphNode* ScenesGraphNode::__GetRoot()
	{
		if (ParentNode == nullptr)
		{
			return nullptr;
		}

		return ParentNode->Root;
	}

	void ScenesGraphNode::Dispose()
	{
		OnDispose();

		// Unlink from the parent
		if (parentNode != nullptr)
		{
			parentNode->ChildNodes.Remove(this);
			parentNode = nullptr;
		}
	}

	void ScenesGraphNode::OnDispose()
	{
		// Call deeper
		for (int i = 0; i < ChildNodes.Count(); i++)
		{
			ChildNodes[i]->OnDispose();
		}

		EditorApp::Ins().sceneGraphModule->Nodes.Remove(ID);
	}


	ScenesGraphNode::ScenesGraphNode(UID id)
	{
		m_ID = id;
		ScenesGraphNode* duplicate;
		SceneGraphModule* sceneGraphModule = EditorApp::Ins().sceneGraphModule;;
		if (sceneGraphModule->Nodes.TryGet(id, duplicate) && duplicate != nullptr)
		{
			LOG_WARNING("Scene", "Duplicated Scene Graph node with ID {0} of type '{1}'", id, duplicate->GetTypeInfo()->friendlyName);
		}
		sceneGraphModule->Nodes[id] = this;
	}

	::SE::Transform ScenesGraphNode::__GetTransform()
	{
		return m_Transform;
	}

	void ScenesGraphNode::__SetTransform(::SE::Transform value)
	{
		m_Transform = value;
	}

} // SE