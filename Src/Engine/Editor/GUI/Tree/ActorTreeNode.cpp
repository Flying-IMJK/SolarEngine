
#include "ActorTreeNode.h"

#include "Tree.h"
#include "Editor/GUI/Drag/DragActors.hpp"
#include "Editor/GUI/Drag/DragAssets.hpp"
#include "Editor/Resource/Items/Assetitem.h"
#include "Editor/SceneGraph/ActorGraphNode.h"
#include "Runtime/Level/Actor.h"
#include "Runtime/Level/Level.h"
#include "../../../Runtime/Level/Scene/Scene.h"
#include "Runtime/Render/2D/Render2D.h"
#include "runtime/ui/gui/rootcontrol.h"
#include "Runtime/UI/GUI/Style.h"
#include "Runtime/UI/GUI/Panels/Panel.h"

namespace SE::Editor
{
	Actor* ActorTreeNode::GetActor()
	{
		return _actorNode->Actor;
	}
	ActorGraphNode* ActorTreeNode::GetActorGraphNode()
	{
		return _actorNode;
	}

	ActorTreeNode::ActorTreeNode() : TreeNode(true)
	{
		ChildrenIndent = 16.0f;
	}

	void ActorTreeNode::OnOrderInParentChanged()
	{
		// Use cached value to check if we need to update UI layout (and update siblings order at once)
		ActorTreeNode* parent;
		if (TypeTryCast(Parent, parent))
		{
			bool anyChanged = false;
			auto& children = parent->Children();
			for (int i = 0; i < children.Count(); i++)
			{
				ActorTreeNode* child;
				if (TypeTryCast(children[i], child) && child->GetActor())
				{
					float orderInParent = child->GetActor()->GetOrderInParent();
					anyChanged |= child->_orderInParent != orderInParent;
					if (anyChanged)
						child->_orderInParent = orderInParent;
				}
			}
			if (anyChanged)
				parent->SortChildren();
		}
		else if (GetActor())
		{
			_orderInParent = GetActor()->GetOrderInParent();
		}
	}
	
	void ActorTreeNode::UpdateText()
	{
		Text = _actorNode->Name;
	}
	
	void ActorTreeNode::UpdateFilter(String filterText)
	{
		// SKip hidden actors
		Actor* actor = GetActor();
		if (actor != nullptr/* && (actor.HideFlags & HideFlags.HideInHierarchy) != 0*/)
			return;

		bool noFilter = filterText.IsEmpty();
		_hasSearchFilter = !noFilter;

		// Update itself
		bool isThisVisible;
		if (noFilter)
		{
			// Clear filter
			_highlights.Clear();
			isThisVisible = true;
		}
		else
		{
			/*var text = Text;
			if (QueryFilterHelper.Match(filterText, text, out QueryFilterHelper.Range[] ranges))
			{
				// Update highlights
				if (_highlights == nullptr)
					_highlights = new List<Rectangle>(ranges.Length);
				else
					_highlights.Clear();
				var font = Style.Current.FontSmall;
				var textRect = TextRect;
				for (int i = 0; i < ranges.Length; i++)
				{
					var start = font.GetCharPosition(text, ranges[i].StartIndex);
					var end = font.GetCharPosition(text, ranges[i].EndIndex);
					_highlights.Add(new Rectangle(start.X + textRect.X, textRect.Y, end.X - start.X, textRect.Height));
				}
				isThisVisible = true;
			}
			else
			{
				// Hide
				_highlights?.Clear();
				isThisVisible = false;
			}*/
		}

		// Update children
		bool isAnyChildVisible = false;
		for (int i = 0; i < m_Children.Count(); i++)
		{
			ActorTreeNode* child;
			if (TypeTryCast(m_Children[i], child))
			{
				child->UpdateFilter(filterText);
				isAnyChildVisible |= child->Visible;
			}
		}

		bool isExpanded = isAnyChildVisible;

		// Restore cached state on query filter clear
		if (noFilter && actor != nullptr)
		{
			// Pick the correct id when inside a prefab window.
			UID id = actor->HasPrefabLink() && actor->GetScene() == nullptr ? actor->GetPrefabObjectID() : actor->GetInstanceID();
			isExpanded = false;//Editor.Instance.ProjectCache.IsExpandedActor(ref id);
		}

		if (isExpanded)
		{
			Expand(true);
		}
		else
		{
			Collapse(true);
		}

		Visible = isThisVisible | isAnyChildVisible;
	}
	
	void ActorTreeNode::Update(float deltaTime)
	{
		// Update hidden state
		Actor* actor = GetActor();
		if (actor && !_hasSearchFilter)
		{
			Visible = true;/*(actor.HideFlags & HideFlags.HideInHierarchy) == 0*/;
		}

		TreeNode::Update(deltaTime);
	}
	
	bool ActorTreeNode::OnShowTooltip(String text, Float2& location, Rectangle& area)
	{
		// Evaluate tooltip text once it's actually needed
		Actor* actor = _actorNode->Actor;
		if (TooltipText.IsEmpty() && actor)
		{
			// TooltipText = Surface.SurfaceUtils.GetVisualScriptTypeDescription(TypeUtils.GetObjectType(actor));
		}

		return TreeNode::OnShowTooltip(text, location, area);
	}
	
	int ActorTreeNode::Compare(const Control* other) const
	{
		ActorTreeNode* node;
		if (TypeTryCast(const_cast<Control*>(other), node))
		{
			return _orderInParent - node->_orderInParent;
		}
		return TreeNode::Compare(other);
	}
	
	void ActorTreeNode::StartRenaming(EditorWindow* window, Panel* treePanel)
	{
		// Block renaming during scripts reload
		/*if (Editor.Instance.ProgressReporting.CompileScripts.IsActive)
			return;*/

		Select();

		/*// Disable scrolling of view
		if (window is SceneTreeWindow)
			(window as SceneTreeWindow).ScrollingOnSceneTreeView(false);
		else if (window is PrefabWindow)
			(window as PrefabWindow).ScrollingOnTreeView(false);

		// Start renaming the actor
		var rect = TextRect;
		if (treePanel != nullptr)
		{
			treePanel.ScrollViewTo(this, true);
			rect.Size = new Float2(treePanel.Width - TextRect.Location.X, TextRect.Height);
		}
		var dialog = RenamePopup.Show(this, rect, _actorNode.Name, false);
		dialog.Renamed += OnRenamed;
		dialog.Closed += popup =>
		{
			// Enable scrolling of view
			if (window is SceneTreeWindow)
				(window as SceneTreeWindow).ScrollingOnSceneTreeView(true);
			else if (window is PrefabWindow)
				(window as PrefabWindow).ScrollingOnTreeView(true);
		};*/
	}
	
	void ActorTreeNode::Draw()
	{
		TreeNode::Draw();

		// Draw all highlights
		if (!_highlights.IsEmpty())
		{
			Style* style = Style::Current;
			Color color = style->ProgressNormal * 0.6f;
			for (int i = 0; i < _highlights.Count(); i++)
			{
				Render2D::FillRectangle(_highlights[i], color);
			}
		}
	}

	bool ActorTreeNode::ShowTooltip()
	{
		return true;
	}
	
	Color ActorTreeNode::CacheTextColor()
	{
		// Update node text color (based on ActorGraphNode*.IsActiveInHierarchy but with optimized logic a little)
		if (TypeIs<ActorTreeNode>(Parent))
		{
			Color color = Style::Current->TextColor;
			Actor* actor = GetActor();
			if (actor)
			{
				if (actor->HasPrefabLink())
				{
					// Prefab
					color = Style::Current->ProgressNormal;
				}

				if (!actor->IsActiveInHierarchy())
				{
					// Inactive
					return Style::Current->ForegroundGrey;
				}

				if (actor->HasScene() /*&& Editor.Instance.StateMachine.IsPlayMode*/ && actor->IsStatic())
				{
					// Static
					return color * 0.85f;
				}
			}

			// Default
			return color;
		}

		return TreeNode::CacheTextColor();
	}
	
	void ActorTreeNode::OnExpandedChanged()
	{
		TreeNode::OnExpandedChanged();
		Actor* actor = GetActor();

		if (!GetIsLayoutLocked() && actor)
		{
			// Pick the correct id when inside a prefab window.
			UID id = actor->HasPrefabLink() && !actor->HasScene() ? actor->GetPrefabObjectID() : actor->GetInstanceID();
			// Editor.Instance.ProjectCache.SetExpandedActor(ref id, IsExpanded);
		}
	}
	
	DragDropEffect ActorTreeNode::OnDragEnterHeader(DragData* data)
	{
		// Check if cannot edit scene or there is no scene loaded (handle case for actors in prefab editor)
		/*if (_actorNode != nullptr && _actorNode->ParentScene != nullptr)
		{
			if (!Editor.Instance.StateMachine.CurrentState.CanEditScene || !Level::IsAnySceneLoaded())
				return DragDropEffect::None;
		}
		else
		{
			if (!Editor.Instance.StateMachine.CurrentState.CanEditContent)
				return DragDropEffect::None;
		}*/

		if (_dragHandlers == nullptr)
			_dragHandlers = new DragHandlers();

		// Check if drop actors
		if (_dragActors == nullptr)
		{
			_dragActors = New<DragActors>(CreateFunc<ActorTreeNode, &ActorTreeNode::ValidateDragActor>(this));
			_dragHandlers->Add(_dragActors);
		}
		if (_dragActors->OnDragEnter(data))
			return _dragActors->GetEffect();
		
		// Check if drag assets
		if (_dragAssets == nullptr)
		{
			_dragAssets = New<DragAssets>(CreateFunc<ActorTreeNode, &ActorTreeNode::ValidateDragAsset>(this));
			_dragHandlers->Add(_dragAssets);
		}
		if (_dragAssets->OnDragEnter(data))
			return _dragAssets->GetEffect();

		return DragDropEffect::None;
	}
	
	DragDropEffect ActorTreeNode::OnDragMoveHeader(DragData* data)
	{
		return _dragHandlers->GetEffect();
	}
	DragDropEffect ActorTreeNode::OnDragDropHeader(DragData* data)
	{
		DragDropEffect result = DragDropEffect::None;

		Actor* myActor = GetActor();
		Actor* newParent;
		int newOrder = -1;

		// Check if has no actor (only for Root Actor*)
		if (myActor == nullptr)
		{
			// Append to the last scene
			auto& scenes = Level::Scenes;
			if (scenes.IsEmpty())
			{
				LOG_ERROR("Node", "No scene loaded.");
				return DragDropEffect::None;
			}
			newParent = scenes[scenes.Count() - 1];
		}
		else
		{
			newParent = myActor;

			// Use drag positioning to change target parent and index
			if (DragOverMode == DragItemPositioning::Above)
			{
				if (myActor->HasParent())
				{
					newParent = myActor->GetParent();
					newOrder = myActor->GetOrderInParent();
				}
			}
			else if (DragOverMode == DragItemPositioning::Below)
			{
				if (myActor->HasParent())
				{
					newParent = myActor->GetParent();
					newOrder = myActor->GetOrderInParent() + 1;
				}
			}
		}

		if (newParent == nullptr)
		{
			LOG_ERROR("Node", "Missing parent actor.");
			return DragDropEffect::None;
		}

		// Drag actors
		if (_dragActors != nullptr && _dragActors->GetHasValidDrag())
		{
			bool worldPositionsStays = Root->GetKey(KeyboardKeys::Control) == false;
			SceneObject** objects = NewArray<SceneObject*>(_dragActors->Objects.Count());
			for (int i = 0; i < _dragActors->Objects.Count(); i++)
			{
				objects[i] = _dragActors->Objects[i]->Actor.operator->();
			}
			/*var action = new ParentActorsAction(objects, newParent, newOrder, worldPositionsStays);
			ActorGraphNode*.Root.Undo?.AddAction(action);
			action.Do();*/
			result = DragDropEffect::Move;
		}
		// Drag assets
		else if (_dragAssets != nullptr && _dragAssets->GetHasValidDrag())
		{
			ENGINE_UNREACHABLE_CODE();
			/*var spawnParent = myActor;
			if (DragOverMode == DragItemPositioning::Above || DragOverMode == DragItemPositioning::Below)
				spawnParent = newParent;

			for (int i = 0; i < _dragAssets.Objects.Count; i++)
			{
				var item = _dragAssets.Objects[i];
				var actor = item.OnEditorDrop(this);
				if (spawnParent.GetType() != typeof(Scene))
				{
					// Set all Actors static flags to match parents
					List<Actor*> childActors = new List<Actor*>();
					Utilities.Utils.GetActorsTree(childActors, actor);
					foreach (var child in childActors)
					{
						child.StaticFlags = spawnParent.StaticFlags;
					}
				}
				actor.Name = item.ShortName;
				if (_dragAssets.Objects[i] is not PrefabItem)
					actor.Transform = Transform.Identity;
				var previousTrans = actor.Transform;
				ActorGraphNode*.Root.Spawn(actor, spawnParent);
				actor.LocalTransform = previousTrans;
				actor->GetOrderInParent() = newOrder;
			}*/
			result = DragDropEffect::Move;
		}

		// Clear cache
		_dragHandlers->OnDragDrop(nullptr);

		// Check if scene has been modified
		if (result != DragDropEffect::None)
		{
			/*var node = SceneGraph::FindNode(newParent.ID) as ActorGraphNode*;
			node?.TreeNode.Expand();*/
		}

		return result;
	}
	
	void ActorTreeNode::OnRenamed(RenamePopup* renamePopup)
	{
		/*using (new UndoBlock(ActorGraphNode*.Root.Undo, Actor*, "Rename"))
			Actor*.Name = renamePopup.Text.Trim();*/
	}
	
	bool ActorTreeNode::ValidateDragActor(ActorGraphNode* actorNode)
	{
		return false;
		/*// Reject dragging actors not linked to scene (eg. from prefab) or in the opposite way
		var thisHasScene = ActorGraphNode*.ParentScene != nullptr;
		var otherHasScene = actorNode.ParentScene != nullptr;
		if (thisHasScene != otherHasScene)
			return false;

		// Reject dragging actors between prefab windows (different roots)
		if (!thisHasScene && ActorGraphNode*.Root != actorNode.Root)
			return false;

		// Reject dragging parents and itself
		return actorNode.Actor* != nullptr && actorNode != ActorGraphNode* && actorNode.Find(Actor*) == nullptr;*/
	}
	
	bool ActorTreeNode::ValidateDragAsset(AssetItem* assetItem)
	{
		return assetItem->OnEditorDrag(this);
	}

	void ActorTreeNode::DoDragDrop(DragData* data)
	{
		/*DragData* data;
		var tree = ParentTree;

		// Check if this node is selected
		if (tree.Selection.Contains(this))
		{
			// Get selected actors
			var actors = new List<ActorGraphNode*>();
			for (var i = 0; i < tree.Selection.Count; i++)
			{
				var e = tree.Selection[i];

				// Skip if parent is already selected to keep correct parenting
				if (tree.Selection.Contains(e.Parent))
					continue;

				if (e is ActorTreeNode node && node.ActorGraphNode*.CanDrag)
					actors.Add(node.ActorGraphNode*);
			}
			data = DragActors.GetDragData(actors);
		}
		else
		{
			data = DragActors.GetDragData(ActorGraphNode*);
		}

		// Start drag operation
		DoDragDrop(data);*/
	}

	void ActorTreeNode::LinkNode(ActorGraphNode* node)
	{
		_actorNode = node;
		Actor* actor = node->Actor;
		if (actor != nullptr)
		{
			_orderInParent = actor->GetOrderInParent();
			Visible = true; // (actor.HideFlags & HideFlags.HideInHierarchy) == 0;

			// Pick the correct id when inside a prefab window.
			UID id = actor->HasPrefabLink() && !actor->HasScene() ? actor->GetPrefabObjectID() : actor->GetInstanceID();
			/*if (Editor.Instance.ProjectCache.IsExpandedActor(id))
			{
				Expand(true);
			}
			*/
		}
		else
		{
			_orderInParent = 0;
		}

		UpdateText();
	}
	
	void ActorTreeNode::OnParentChanged(Actor* actor, ActorGraphNode* parentNode)
	{
		// Update cached value
		_orderInParent = actor->GetOrderInParent();

		// Update UI (special case if actor is spawned and added to existing scene tree)
		ActorTreeNode* parentTreeNode = nullptr;
		if (parentNode != nullptr)
		{
			parentTreeNode = parentNode->TreeNode;
		}

		if (parentTreeNode != nullptr && !parentTreeNode->GetIsLayoutLocked())
		{
			parentTreeNode->SetIsLayoutLocked(true);
			Parent = parentTreeNode;
			IndexInParent = _orderInParent;
			parentTreeNode->SetIsLayoutLocked(false);

			// Skip UI update if node won't be in a view
			if (parentTreeNode->IsCollapsedInHierarchy)
			{
				UnlockChildrenRecursive();
			}
			else
			{
				// Try to perform layout at the level where it makes it the most performant (the least computations)
				Tree* tree = parentTreeNode->ParentTree;
				if (tree != nullptr)
				{
					Panel* treeParent;
					if (TypeTryCast(tree->Parent, treeParent))
						treeParent->PerformLayout();
					else
						tree->PerformLayout();
				}
				else
				{
					parentTreeNode->PerformLayout();
				}
			}
		}
		else
		{
			Parent = parentTreeNode;
		}
	}
} // SE