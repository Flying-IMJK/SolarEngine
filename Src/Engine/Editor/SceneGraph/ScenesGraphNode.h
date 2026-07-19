#pragma once

#include "Runtime/Core/Math/Transform.h"
#include "Runtime/Core/Types/Property.h"
#include "Runtime/Core/Types/UID.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/TypeSystem/IType.h"

namespace SE
{
	class Object;
}

namespace SE::Editor
{
	class RootGraphNode;

	SE_CLASS(Reflect)
	class ScenesGraphNode : public IType
	{
		SE_DEFINE_CLASS_DEFAULT(ScenesGraphNode, IType)
	public:
		/// <summary>
		/// Gets the children list.
		/// </summary>
		List<ScenesGraphNode*> ChildNodes;

		PRO_GET(Name, ScenesGraphNode, String, __GetName);

		/// <summary>
		/// Gets the identifier. Must be unique and immutable.
		/// </summary>
		PRO_GET(ID, ScenesGraphNode, UID, __GetID);

		PRO_GET(Root, ScenesGraphNode, RootGraphNode*, __GetRoot);

		/// <summary>
		/// Gets or sets the transform of the node.
		/// </summary>
		PRO(Transform, ScenesGraphNode, Transform, __GetTransform, __SetTransform);

		/// <summary>
		/// Gets the parent scene.
		/// </summary>
		PRO_GET(ParentScene, ScenesGraphNode, ScenesGraphNode*, __GetParentScene);

		/// <summary>
		/// Gets or sets order of the node in the parent container.
		/// </summary>
		PRO(OrderInParent, ScenesGraphNode, int, __GetOrderInParent, __SetOrderInParent);

		/// <summary>
		/// Gets or sets the parent node.
		/// </summary>
		PRO(ParentNode, ScenesGraphNode, ScenesGraphNode*, __GetParentNode,  __SetParentNode);

		virtual Object* GetEditableObject() = 0;

		virtual void Dispose();

		/// <summary>
		/// Called when node or parent node is disposing.
		/// </summary>
		virtual void OnDispose();

	protected:
		/// <summary>
		/// The parent node.
		/// </summary>
		ScenesGraphNode* parentNode;

		/// <summary>
		/// Initializes a new instance of the <see cref="SceneGraphNode"/> class.
		/// </summary>
		/// <param name="id">The unique node identifier. Cannot be changed at runtime.</param>
		ScenesGraphNode(UID id);

		virtual void OnParentChanged()
		{
		}

		String virtual __GetName() = 0;

		::SE::Transform virtual __GetTransform();

		void virtual __SetTransform(::SE::Transform value);

		virtual int __GetOrderInParent()
		{
			return m_OrderInParent;
		}

		virtual void __SetOrderInParent(int value)
		{
			m_OrderInParent = value;
		}

		virtual ScenesGraphNode* __GetParentNode();

		virtual void __SetParentNode(ScenesGraphNode* value);

		virtual ScenesGraphNode* __GetParentScene() = 0;

		virtual RootGraphNode* __GetRoot();

	private:
		UID m_ID;
		::SE::Transform m_Transform;
		int m_OrderInParent;

		UID __GetID() { return m_ID; };
	};

} // SE
