#pragma once
#include "Editor/SceneGraph/ActorGraphNode.h"

namespace SE::Editor
{
	class CameraGraphNode : public ActorGraphNode
	{
		SE_CLASS_DEFAULT(CameraGraphNode, ActorGraphNode)
	public:
		CameraGraphNode(::SE::Actor* actor);

		/// <inheritdoc />
		/*void OnContextMenu(ContextMenu* contextMenu, EditorWindow* window) override
		{
			base.OnContextMenu(contextMenu, window);
			if (window is not SceneTreeWindow win)
				return;
			var button = new ContextMenuButton(contextMenu, "Move Camera to View");
			button.Parent = contextMenu.ItemsContainer;
			contextMenu.ItemsContainer.Children.Remove(button);
			contextMenu.ItemsContainer.Children.Insert(4, button);
			button.Clicked += () =>
			{
				var c = Actor as Camera;
				var viewport = Editor.Instance.Windows.EditWin.Viewport;
				c.Position = viewport.ViewPosition;
				c.Orientation = viewport.ViewOrientation;
			};
		}*/

		/// <inheritdoc />
		/*public override bool RayCastSelf(ref RayCastData ray, out Real distance, out Vector3 normal)
		{
			normal = Vector3.Up;

			// Check if skip raycasts
			if ((ray.Flags & RayCastData.FlagTypes.SkipEditorPrimitives) == RayCastData.FlagTypes.SkipEditorPrimitives)
			{
				distance = 0;
				return false;
			}

			return Camera.Internal_IntersectsItselfEditor(FlaxEngine.Object.GetUnmanagedPtr(_actor), ref ray.Ray, out distance);
		}*/
	};
} // SE

