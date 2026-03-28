
#include "SceneModule.h"

#include "SceneGraphModule.h"
#include "Core/Platform/Win32/Win32File.h"
#include "Core/Types/Collections/ListExtensions.h"
#include "Core/Types/Strings/StringView.h"
#include "Runtime/Level/Level.h"
#include "Runtime/Level/Scene/Scene.h"
#include "Runtime/UI/GUI//Rootcontrol.h"

#include "Editor/EditorApp.h"
#include "Editor/GUI/Tree/ActorTreeNode.h"
#include "Editor/SceneGraph/ActorGraphNode.h"
#include "Editor/SceneGraph/Actor/SceneGraphNode.h"

namespace SE::Editor
{
	SceneModule::SceneModule(EditorApp* editor): EditorModule(editor)
	{
	}

	void SceneModule::OnSceneSaving(Scene* scene, const UID &sceneId)
	{
		OnSaveStart(RootControl::GetGameRoot());
	}

	void SceneModule::OnSceneSaved(Scene* scene, const UID &sceneId)
	{
		OnSaveEnd(RootControl::GetGameRoot());
	}

	void SceneModule::OnSceneSaveError(Scene* scene, const UID &sceneId)
	{
		OnSaveEnd(RootControl::GetGameRoot());
	}

	void SceneModule::OnSceneLoaded(Scene* scene, const UID &sceneId)
	{
		DateTime startTime = DateTime::NowUTC();

		// Build scene tree
		SceneGraphNode* sceneNode = editor->sceneGraphModule->BuildSceneTree(scene);
		ActorTreeNode* treeNode = sceneNode->TreeNode;
		treeNode->SetIsLayoutLocked(true);
		treeNode->Expand(true);

		// Add to the tree
		ActorTreeNode* rootNode = Root->TreeNode;
		rootNode->SetIsLayoutLocked(true);
		sceneNode->ParentNode = Root;
		rootNode->SortChildren();
		rootNode->SetIsLayoutLocked(false);
		rootNode->Parent->PerformLayout();

		DateTime endTime = DateTime::NowUTC();
		int milliseconds = (endTime - startTime).GetTotalMilliseconds();
		LOG_INFO("Scene", "Created graph for scene \'{0}\' in {1} ms", scene->GetName(), milliseconds);
	}

	void SceneModule::OnSceneUnloading(Scene* scene, const UID &sceneId)
	{
		// Find scene tree node
		ActorGraphNode* node = Root->FindChildActor(scene);
		if (node != nullptr)
		{
			LOG_INFO("Scene", "Cleanup graph for scene \'{0}\'", scene->GetName());

			// Cleanup
			auto selection = editor->sceneModule->Selection;
			bool hasSceneSelection = false;
			for (int i = 0; i < selection.Count(); i++)
			{
				if (selection[i]->ParentScene == node)
				{
					hasSceneSelection = true;
					break;
				}
			}
			if (hasSceneSelection)
			{
				List<ScenesGraphNode*> newSelection;
				for (int i = 0; i < selection.Count(); i++)
				{
					if (selection[i]->ParentScene != node)
						newSelection.Add(selection[i]);
				}
				editor->sceneModule->Select(newSelection);
			}
			node->Dispose();
		}
	}

	void SceneModule::OnActorSpawned(Actor* actor)
	{
		// Skip for not loaded scenes (spawning actors during scene loading in script Start function)
		ActorGraphNode* sceneNode = GetActorNode(actor->GetScene());
		if (sceneNode == nullptr)
		{
			return;
		}

		// Skip for missing parent
		Actor* parent = actor->GetParent();
		if (parent == nullptr)
		{
			return;
		}

		ActorGraphNode* parentNode = GetActorNode(parent);
		if (parentNode == nullptr)
		{
			// Missing parent node when adding child actor to not spawned or unlinked actor
			return;
		}

		ActorGraphNode* node = editor->sceneGraphModule->BuildActorNode(actor);
		if (node != nullptr)
		{
			node->ParentNode = parentNode;
		}
	}

	void SceneModule::OnActorDeleted(Actor* actor)
	{
		ActorGraphNode* node = GetActorNode(actor);
		if (node != nullptr)
		{
			OnActorDeleted(node);
		}
	}

	void SceneModule::OnActorDeleted(ActorGraphNode* node)
	{
		for (int i = 0; i < node->ChildNodes.Count(); i++)
		{
			ActorGraphNode* child;
			if (TypeTryCast(node->ChildNodes[i], child))
			{
				i--;
				OnActorDeleted(child);
			}
		}

		// ActorRemoved?.Invoke(node);

		// Cleanup part of the graph
		node->Dispose();
	}

	void SceneModule::OnActorParentChanged(Actor* actor, Actor* prevParent)
	{
		ActorGraphNode* node = nullptr;
		ActorGraphNode* parentNode = GetActorNode(actor->GetParent());

		// Try use previous parent actor to find actor node
		ActorGraphNode* prevParentNode = GetActorNode(prevParent);
		if (prevParentNode != nullptr)
		{
			// If should be one of the children
			node = prevParentNode->FindChildActor(actor);

			// Search whole tree if node was not found
			if (node == nullptr)
			{
				node = GetActorNode(actor);
			}
		}
		else if (parentNode != nullptr)
		{
			// Create new node for that actor (user may unlink it from the scene before and now link it)
			node = editor->sceneGraphModule->BuildActorNode(actor);
		}
		if (node == nullptr)
			return;

		// Get the new parent node (may be missing)
		node->ParentNode = parentNode;
		if (parentNode == nullptr)
		{
			// Check if actor is selected in editor
			if (Selection.Contains(node))
			{
				Deselect();
			}

			// Remove node (user may unlink actor from the scene but not destroy the actor)
			node->Dispose();
		}
	}

	void SceneModule::OnActorOrderInParentChanged(Actor* actor)
	{
		ActorGraphNode* node = GetActorNode(actor);
		if (node != nullptr)
		{
			node->TreeNode->OnOrderInParentChanged();
		}
	}

	void SceneModule::OnActorNameChanged(Actor* actor)
	{
		ActorGraphNode* node = GetActorNode(actor);
		if (node != nullptr)
		{
			node->TreeNode->UpdateText();
		}
	}

	void SceneModule::OnActorActiveChanged(Actor* actor)
	{
		ActorGraphNode* node = GetActorNode(actor);
		if (node != nullptr)
		{
			ENGINE_UNREACHABLE_CODE()
			// node->TreeNode->OnActiveChanged();
		}
	}

	List<ScenesGraphNode*>& SceneModule::ScenesRootNode::__GetSelection()
	{
		return _editor.sceneModule->Selection;
	}

	void SceneModule::Select(Actor* actor)
	{
		ActorGraphNode* node = GetActorNode(actor);
		if (node != nullptr)
		{
			Select(node);
		}
	}

	void SceneModule::Select(List<ScenesGraphNode*>& selection, bool additive)
	{
		if (selection.IsEmpty())
		{
			Deselect();
			return;
		}

		// Prevent from selecting null nodes
		for (int i = selection.Count() - 1; i >= 0; i--)
		{
			if (selection[i] == nullptr)
			{
				selection.RemoveAt(i);
			}
		}


		// Check if won't change
		if (!additive && Selection.Count() == selection.Count())
		{
			auto equalFunc = CreateFunc([](ScenesGraphNode* &a, ScenesGraphNode* &b)
			{
				return a == b;
			});

			if (ListExtensions::SequenceEqual(Selection, selection, equalFunc))
			{
				return;
			}
		}


		if (!additive)
		{
			Selection.Clear();
		}
		Selection.Add(selection);

		SelectionChanged();
	}

	void SceneModule::Select(ScenesGraphNode* selection, bool additive)
	{
		if (selection == nullptr)
		{
			return;
		}

		// Check if won't change
		if (!additive && Selection.Count() == 1 && Selection[0] == selection)
		{
			return;
		}
		if (additive && Selection.Contains(selection))
		{
			return;
		}

		if (!additive)
		{
			Selection.Clear();
		}
		Selection.Add(selection);

		// SelectionChange(Selection);
	}

	void SceneModule::Duplicate()
	{
		/*// Peek things that can be copied (copy all actors)
		List<ScenesGraphNode*> nodes = Selection.Where(x => x.CanDuplicate).ToList().BuildAllNodes();
		if (nodes.Count == 0)
			return;
		List<Actor*> actors = List<Actor*>();
		List<SceneGraphNode*> newSelection = List<SceneGraphNode*>();
		// List<IUndoAction> customUndoActions = null;
		for (ScenesGraphNode* node : nodes)
		{
			if (node->CanDuplicate)
			{
				ActorGraphNode* actorNode;
				if (TypeTryCast(node, actorNode))
				{
					actors.Add(actorNode->Actor);
				}
				else
				{
					var customDuplicatedObject = node->Duplicate(out var customUndoAction);
					if (customDuplicatedObject != null)
						newSelection.Add(customDuplicatedObject);
					if (customUndoAction != null)
					{
						if (customUndoActions == null)
							customUndoActions = new List<IUndoAction>();
						customUndoActions.Add(customUndoAction);
					}
				}
			}
		}

		if (actors.Count() == 0)
		{
			// Duplicate custom scene graph nodes only without actors
			if (newSelection.Count != 0)
			{
				// Select spawned objects (parents only)
				var selectAction = new SelectionChangeAction(Selection.ToArray(), newSelection.ToArray(), OnSelectionUndo);
				selectAction.Do();

				// Build a single compound undo action that pastes the actors, pastes custom stuff (scene graph extension) and selects the created objects (parents only)
				var customUndoActionsCount = customUndoActions?.Count ?? 0;
				var undoActions = new IUndoAction[1 + customUndoActionsCount];
				for (int i = 0; i < customUndoActionsCount; i++)
					undoActions[i] = customUndoActions[i];
				undoActions[undoActions.Length - 1] = selectAction;

				Undo.AddAction(new MultiUndoAction(undoActions));
				OnSelectionChanged();
			}
			return;
		}

		// Serialize actors
		var data = Actor.ToBytes(actors.ToArray());
		if (data == null)
		{
			Editor.LogError("Failed to copy actors data.");
			return;
		}

		// Create paste action (with selecting spawned objects)
		var pasteAction = PasteActorsAction.Duplicate(data, Guid.Empty);
		if (pasteAction != null)
		{
			pasteAction.Do(out _, out var nodeParents);

			// Select spawned objects (parents only)
			newSelection.AddRange(nodeParents);
			var selectAction = new SelectionChangeAction(Selection.ToArray(), newSelection.ToArray(), OnSelectionUndo);
			selectAction.Do();

			// Build a single compound undo action that pastes the actors, pastes custom stuff (scene graph extension) and selects the created objects (parents only)
			var customUndoActionsCount = customUndoActions?.Count ?? 0;
			var undoActions = new IUndoAction[2 + customUndoActionsCount];
			undoActions[0] = pasteAction;
			for (int i = 0; i < customUndoActionsCount; i++)
				undoActions[i + 1] = customUndoActions[i];
			undoActions[undoActions.Length - 1] = selectAction;

			Undo.AddAction(new MultiUndoAction(undoActions));
			OnSelectionChanged();
		}

		// Scroll to new selected node while duplicating
		Editor.Windows.SceneWin.ScrollToSelectedNode();*/
	}

	void SceneModule::Delete()
	{
		// Peek things that can be removed
		/*var objects = Selection.Where(x => x.CanDelete).ToList().BuildAllNodes().Where(x => x.CanDelete).ToList();
		if (objects.Count == 0)
			return;
		var isSceneTreeFocus = Editor.Windows.SceneWin.ContainsFocus;

		SelectionDeleteBegin?.Invoke();

		// Change selection
		var action1 = new SelectionChangeAction(Selection.ToArray(), new SceneGraphNode[0], OnSelectionUndo);

		// Delete objects
		var action2 = new DeleteActorsAction(objects);

		// Merge two actions and perform them
		var action = new MultiUndoAction(new IUndoAction[]
		{
			action1,
			action2
		}, action2.ActionString);
		action.Do();
		Undo.AddAction(action);

		SelectionDeleteEnd?.Invoke();

		if (isSceneTreeFocus)
		{
			Editor.Windows.SceneWin.Focus();
		}

		// fix scene window layout
		Editor.Windows.SceneWin.PerformLayout();
		Editor.Windows.SceneWin.PerformLayout();*/
	}

	void SceneModule::Copy()
	{
		// Peek things that can be copied (copy all actors)
		/*var objects = Selection.Where(x => x.CanCopyPaste).ToList().BuildAllNodes().Where(x => x.CanCopyPaste && x is ActorNode).ToList();
		if (objects.Count == 0)
			return;

		// Serialize actors
		var actors = objects.ConvertAll(x => ((ActorNode)x).Actor);
		var data = Actor.ToBytes(actors.ToArray());
		if (data == null)
		{
			Editor.LogError("Failed to copy actors data.");
			return;
		}

		// Copy data
		Clipboard.RawData = data;*/
	}

	void SceneModule::PasteActor(Actor* pasteTargetActor)
	{
		// Get clipboard data
		/*var data = Clipboard.RawData;

		// Set paste target if only one actor is selected and no target provided
		if (pasteTargetActor == null && SelectionCount == 1 && Selection[0] is ActorNode actorNode)
		{
			pasteTargetActor = actorNode.Actor.Scene == actorNode.Actor ? actorNode.Actor : actorNode.Actor.Parent;
		}

		// Create paste action
		var pasteAction = PasteActorsAction.Paste(data, pasteTargetActor?.ID ?? Guid.Empty);
		if (pasteAction != null)
		{
			pasteAction.Do(out _, out var nodeParents);

			// Select spawned objects (parents only)
			var selectAction = new SelectionChangeAction(Selection.ToArray(), nodeParents.Cast<SceneGraphNode>().ToArray(), OnSelectionUndo);
			selectAction.Do();

			// Build single compound undo action that pastes the actors and selects the created objects (parents only)
			Undo.AddAction(new MultiUndoAction(pasteAction, selectAction));
			OnSelectionChanged();
		}

		// Scroll to new selected node while pasting
		Editor.Windows.SceneWin.ScrollToSelectedNode();*/
	}

	void SceneModule::Spawn(Actor* actor, Actor* parent, bool autoSelect)
	{
		bool isPlayMode = false;// Editor.StateMachine.IsPlayMode;

		if (Level::IsAnySceneLoaded() == false)
		{
			LOG_ERROR("Scene", "Cannot spawn actor when no scene is loaded.");
			return;
		}

		// SpawnBegin?.Invoke();

		// Add it
		Level::SpawnActor(actor, parent);

		// Peek spawned node
		ActorGraphNode* actorNode = GetActorNode(actor);
		// "Failed to create scene node for the spawned actor."
		ENGINE_ASSERT(actorNode != nullptr);

		// During play in editor mode spawned actors should be dynamic (user can move them)
		/*if (isPlayMode)
			actor.StaticFlags = StaticFlags.None;*/

		// Call post spawn action (can possibly setup custom default values)
		// actorNode->PostSpawn();

		// Create undo action
		/*IUndoAction action = new DeleteActorsAction(actorNode, true);
		if (autoSelect)
		{
			var before = Selection.ToArray();
			Selection.Clear();
			Selection.Add(actorNode);
			OnSelectionChanged();
			action = new MultiUndoAction(action, new SelectionChangeAction(before, Selection.ToArray(), OnSelectionUndo));
		}
		Undo.AddAction(action);*/

		// Mark scene as dirty
		/*MarkSceneEdited(actor->Scene);

		SpawnEnd?.Invoke();

		OnDirty(actorNode);*/
	}

	ActorGraphNode* SceneModule::GetActorNode(Actor* actor)
	{
		if (actor == nullptr)
			return nullptr;

		return GetActorNode(actor->GetInstanceID());
	}

	ActorGraphNode* SceneModule::GetActorNode(UID actorId)
	{
		// ActorNode has the same ID as actor does
		ScenesGraphNode* sceneGraphNode = editor->sceneGraphModule->FindNode(actorId);
		ActorGraphNode* actorGraphNode = nullptr;
		if (!TypeTryCast<ActorGraphNode>(sceneGraphNode, actorGraphNode))
		{
			actorGraphNode = nullptr;
		}
		return actorGraphNode;
	}

	void SceneModule::Deselect()
	{
		// Check if won't change
		if (Selection.Count() == 0)
			return;

		List<ScenesGraphNode*> before = Selection;
		Selection.Clear();

		// SelectionChange(before);
	}

	void SceneModule::OnInit()
	{
		Root = New<ScenesRootNode>();
		Root->LinkTreeNode();

		// Bind events
		Level::SceneSaving.Bind<SceneModule, &SceneModule::OnSceneSaving>(this);
		Level::SceneSaved.BindUnique<SceneModule, &SceneModule::OnSceneSaved>(this);
		Level::SceneSaveError.BindUnique<SceneModule, &SceneModule::OnSceneSaveError>(this);
		Level::SceneLoaded.BindUnique<SceneModule, &SceneModule::OnSceneLoaded>(this);
		Level::SceneUnloading.BindUnique<SceneModule, &SceneModule::OnSceneUnloading>(this);
		Level::ActorSpawned.BindUnique<SceneModule, &SceneModule::OnActorSpawned>(this);
		Level::ActorDeleted.BindUnique<SceneModule, &SceneModule::OnActorDeleted>(this);
		Level::ActorParentChanged.BindUnique<SceneModule, &SceneModule::OnActorParentChanged>(this);
		Level::ActorOrderInParentChanged.BindUnique<SceneModule, &SceneModule::OnActorOrderInParentChanged>(this);
		Level::ActorNameChanged.BindUnique<SceneModule, &SceneModule::OnActorNameChanged>(this);
		Level::ActorActiveChanged .BindUnique<SceneModule, &SceneModule::OnActorActiveChanged>(this);
	}

	void SceneModule::OnExit()
	{
		// Unbind events
		Level::SceneSaving.Unbind<SceneModule, &SceneModule::OnSceneSaving>(this);
		Level::SceneSaved.Unbind<SceneModule, &SceneModule::OnSceneSaved>(this);
		Level::SceneSaveError.Unbind<SceneModule, &SceneModule::OnSceneSaveError>(this);
		Level::SceneLoaded.Unbind<SceneModule, &SceneModule::OnSceneLoaded>(this);
		Level::SceneUnloading.Unbind<SceneModule, &SceneModule::OnSceneUnloading>(this);
		Level::ActorSpawned.Unbind<SceneModule, &SceneModule::OnActorSpawned>(this);
		Level::ActorDeleted.Unbind<SceneModule, &SceneModule::OnActorDeleted>(this);
		Level::ActorParentChanged.Unbind<SceneModule, &SceneModule::OnActorParentChanged>(this);
		Level::ActorOrderInParentChanged.Unbind<SceneModule, &SceneModule::OnActorOrderInParentChanged>(this);
		Level::ActorNameChanged.Unbind<SceneModule, &SceneModule::OnActorNameChanged>(this);
		Level::ActorActiveChanged .Unbind<SceneModule, &SceneModule::OnActorActiveChanged>(this);
	}

	void SceneModule::CreateSceneFile(StringView path)
	{
		Scene* scene = New<Scene>();

		/*var cam = scene->AddChild<Camera>();
		cam.Name = "Camera";
		cam.Position = new Vector3(0, 150, -300);*/

		List<byte> bytes;
		if (!Level::SaveSceneToBytes(bytes, scene))
		{
			LOG_ERROR("Level", "Serialize Scene File failed");
		}

		if (!File::WriteAllBytes(path, bytes))
		{
			LOG_ERROR("Level", "Create Scene File failed");
		}
	}

	void SceneModule::SaveScene(Scene* scene)
	{
		ActorGraphNode* node = GetActorNode(scene);
		SceneGraphNode* sceneNode = static_cast<SceneGraphNode*>(node);
		SaveScene(sceneNode);
	}

	void SceneModule::SaveScene(SceneGraphNode* scene)
	{
		if (!scene->IsEdited)
			return;

		scene->IsEdited = false;
		Level::SaveSceneAsync(scene->scene);
	}

	void SceneModule::OpenScene(UID sceneId, bool additive)
	{
		/*if (!Editor.StateMachine.CurrentState.CanChangeScene)
			return;*/

		// In play-mode Editor mocks the level streaming script
		/*if (Editor.IsPlayMode)
		{
			if (!additive)
			{
				Level.UnloadAllScenesAsync();
			}

			Level::LoadSceneAsync(sceneId);
			return;
		}*/

		/*if (!additive)
		{
			// Ensure to save all pending changes
			if (CheckSaveBeforeClose())
				return;
		}*/

		// Load scene
		Level::LoadSceneAsync(sceneId);
	}

	void SceneModule::OnSaveStart(ContainerControl* uiRoot)
	{
		// Force viewport UI to have fixed size during scene/prefabs saving to result in stable data (less mess in version control diffs)
		if (m_UiRootSizes == nullptr)
		{
			m_UiRootSizes = New<Dictionary<ContainerControl*, Float2>>();
		}
		m_UiRootSizes->At(uiRoot) = uiRoot->Size;
		uiRoot->Size = Float2(1920, 1080);
	}

	void SceneModule::OnSaveEnd(ContainerControl* uiRoot)
	{
		// Restore cached size of the UI root container
		Float2 size;
		if (m_UiRootSizes != nullptr && m_UiRootSizes->TryGet(uiRoot, size))
		{
			uiRoot->Size = size;
			m_UiRootSizes->Remove(uiRoot);
		}
	}

	int SceneModule::InitOrder()
	{
		return -91;
	}

	int SceneModule::__GetSelectionCount()
	{
		return Selection.Count();
	}

	bool SceneModule::__GetHasSthSelected()
	{
		return Selection.Count() > 0;
	}
} // SE