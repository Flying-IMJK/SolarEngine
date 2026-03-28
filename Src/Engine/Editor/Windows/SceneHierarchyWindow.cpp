
#include "SceneHierarchyWindow.h"

#include "Editor/EditorApp.h"
#include "Editor/GUI/ContextMenu/ContextMenu.h"
#include "Editor/GUI/ContextMenu/ContextMenuButton.h"
#include "Editor/GUI/ContextMenu/ContextMenuChildMenu.h"
#include "Editor/GUI/Drag/DragHandlers.h"
#include "Editor/GUI/Input/SearchBox.h"
#include "Editor/GUI/Tree/ActorTreeNode.h"
#include "Editor/GUI/Tree/Tree.h"
#include "Editor/Modules/SceneModule.h"
#include "Editor/Resource/Items/AssetItem.h"
#include "Editor/SceneGraph/ActorGraphNode.h"
#include "Runtime/Level/Actor.h"
#include "Runtime/Level/Level.h"
#include "Runtime/Level/Actors/Camera.h"
#include "Runtime/Level/Actors/DirectionalLight.h"
#include "Runtime/Level/Actors/PointLight.h"
#include "Runtime/level/actors/staticmodel.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/Style.h"


namespace SE::Editor
{
	class ScenesGraphNode;

	SceneHierarchyWindow::SceneHierarchyWindow() : EditorWindow(&EditorApp::Ins(), true, ScrollBars::None)
	{
		Title = SE_TEXT("Hierarchy");

		// Scene searching query input box
		ContainerControl* headerPanel = New<ContainerControl>();
		headerPanel->AnchorPreset = AnchorPresets::HorizontalStretchTop;
		headerPanel->BackgroundColor = Style::Current->Background;
		headerPanel->IsScrollable = false;
		headerPanel->Offsets = Margin(0, 0, 0, 18 + 6);
		headerPanel->Parent = this;

		m_SearchBox = New<SearchBox>();
		m_SearchBox->AnchorPreset = AnchorPresets::HorizontalStretchMiddle,
		m_SearchBox->Parent = headerPanel,
		m_SearchBox->Bounds = Rectangle(4, 4, headerPanel->Width - 8, 18),
		m_SearchBox->TextChanged.Bind<SceneHierarchyWindow, &SceneHierarchyWindow::OnSearchBoxTextChanged>(this);;
		m_SearchBox->Name = SE_TEXT("SceneHierarchyWindow-SearchBox");

		// Scene tree panel
		m_SceneTreePanel = New<Panel>();
		m_SceneTreePanel->AnchorPreset = AnchorPresets::StretchAll,
		m_SceneTreePanel->Offsets = Margin(0, 0, headerPanel->Bottom, 0),
		m_SceneTreePanel->IsScrollable = true,
		m_SceneTreePanel->SetScrollBars(ScrollBars::Both);
		m_SceneTreePanel->Parent = this;
		m_SceneTreePanel->Name = SE_TEXT("SceneHierarchyWindow-SceneTreePanel");

		// Create scene structure tree
		auto root = editor->sceneModule->Root;
		root->TreeNode->ChildrenIndent = 0;
		root->TreeNode->Expand();
		root->TreeNode->Name = SE_TEXT("SceneHierarchyWindow-Tree/Root");

		m_Tree = New<Tree>(true);
		m_Tree->Margin = Margin(0.0f, 0.0f, -16.0f, m_SceneTreePanel->GetScrollBarsSize()), // Hide root node
		m_Tree->IsScrollable = true;
		m_Tree->AddChild(root->TreeNode.Get());
		m_Tree->SelectedChanged.Bind<SceneHierarchyWindow, &SceneHierarchyWindow::Tree_OnSelectedChanged>(this);
		m_Tree->RightClick.Bind<SceneHierarchyWindow, &SceneHierarchyWindow::OnTreeRightClick>(this);
		m_Tree->Parent = m_SceneTreePanel;
		m_Tree->Name = SE_TEXT("SceneHierarchyWindow-Tree");

		// Setup input actions
		/*InputActions.Add(options => options.TranslateMode, () => Editor.MainTransformGizmo.ActiveMode = TransformGizmoBase.Mode.Translate);
		InputActions.Add(options => options.RotateMode, () => Editor.MainTransformGizmo.ActiveMode = TransformGizmoBase.Mode.Rotate);
		InputActions.Add(options => options.ScaleMode, () => Editor.MainTransformGizmo.ActiveMode = TransformGizmoBase.Mode.Scale);
		InputActions.Add(options => options.FocusSelection, () => Editor.Windows.EditWin.Viewport.FocusSelection());
		InputActions.Add(options => options.LockFocusSelection, () => Editor.Windows.EditWin.Viewport.LockFocusSelection());
		InputActions.Add(options => options.Rename, Rename);*/
	}

	void SceneHierarchyWindow::ScrollingOnSceneTreeView(bool enabled)
	{
		if (m_SceneTreePanel->vScrollBar != nullptr)
			m_SceneTreePanel->vScrollBar->ThumbEnabled = enabled;
		if (m_SceneTreePanel->hScrollBar != nullptr)
			m_SceneTreePanel->hScrollBar->ThumbEnabled = enabled;
	}

	void SceneHierarchyWindow::Search()
	{
		m_SearchBox->Focus();
	}

	void SceneHierarchyWindow::ShowContextMenu(Control* parent, Float2 location)
	{
		ContextMenu* contextMenu = CreateContextMenu();
		contextMenu->Show(parent, location);
	}

	void SceneHierarchyWindow::OnInit()
	{
		// editor->sceneModule->SelectionChanged += OnSelectionChanged;

		m_ActorCreateMenus.Add(Pair<String, TypeID>(SE_TEXT("Camera"), Typeof<Camera>()));
		m_ActorCreateMenus.Add(Pair<String, TypeID>(SE_TEXT("Light/Direction"), Typeof<DirectionalLight>()));
		m_ActorCreateMenus.Add(Pair<String, TypeID>(SE_TEXT("Light/Point"), Typeof<PointLight>()));
		m_ActorCreateMenus.Add(Pair<String, TypeID>(SE_TEXT("Model/Static"), Typeof<StaticModel>()));
	}

	void SceneHierarchyWindow::Draw()
	{
		Style* style = Style::Current;

		// Draw overlay
		String overlayText;
		// var state = Editor.StateMachine.CurrentState;
		TextWrapping textWrap = TextWrapping::NoWrap;
		/*if (state is LoadingState)
		{
			overlayText = "Loading...";
		}
		else if (state is ChangingScenesState)
		{
			overlayText = "Loading scene...";
		}
		else */if (((ContainerControl*)m_Tree->GetChild(0))->ChildrenCount() == 0)
		{
			overlayText = "No scene\nOpen one from the content window";
			textWrap = TextWrapping::WrapWords;
		}

		if (!overlayText.IsEmpty())
		{
			Render2D::RenderText(style->FontLarge, overlayText, GetClientArea(), style->ForegroundDisabled, TextAlignment::Center, TextAlignment::Center, textWrap);
		}

		EditorWindow::Draw();
	}

	bool SceneHierarchyWindow::OnMouseDown(Float2 location, MouseButton buttons)
	{
		if (EditorWindow::OnMouseDown(location, buttons))
			return true;

		if (buttons == MouseButton::Right)
		{
			_isMouseDown = true;
			return true;
		}

		return false;
	}

	bool SceneHierarchyWindow::OnMouseUp(Float2 location, MouseButton buttons)
	{
		if (EditorWindow::OnMouseUp(location, buttons))
			return true;

		if (_isMouseDown && buttons == MouseButton::Right)
		{
			_isMouseDown = false;

			/*if (Editor.StateMachine.CurrentState.CanEditScene)
			{*/
				// Show context menu
				// Editor.SceneEditing.Deselect();
				ShowContextMenu(Parent, location + m_SearchBox->BottomLeft);
			/*}*/

			return true;
		}

		if (buttons == MouseButton::Left)
		{
			/*if (Editor.StateMachine.CurrentState.CanEditScene)
			{
				Editor.SceneEditing.Deselect();
			}*/
			return true;
		}

		return false;
	}

	void SceneHierarchyWindow::OnLostFocus()
	{
		_isMouseDown = false;

		EditorWindow::OnLostFocus();
	}

	DragDropEffect SceneHierarchyWindow::OnDragEnter(const Float2& location, DragData* data)
	{
		DragDropEffect result = EditorWindow::OnDragEnter(location, data);
		/*if (Editor.StateMachine.CurrentState.CanEditScene)
		{
			if (_dragHandlers == nullptr)
				_dragHandlers = new DragHandlers();
			if (_dragAssets == nullptr)
			{
				_dragAssets = new DragAssets(ValidateDragAsset);
				_dragHandlers.Add(_dragAssets);
			}
			if (_dragAssets.OnDragEnter(data) && result == DragDropEffect.None)
				return _dragAssets.Effect;
			if (_dragActorType == nullptr)
			{
				_dragActorType = new DragActorType(ValidateDragActorType);
				_dragHandlers.Add(_dragActorType);
			}
			if (_dragActorType.OnDragEnter(data) && result == DragDropEffect.None)
				return _dragActorType.Effect;
			if (_dragControlType == nullptr)
			{
				_dragControlType = new DragControlType(ValidateDragControlType);
				_dragHandlers.Add(_dragControlType);
			}
			if (_dragControlType.OnDragEnter(data) && result == DragDropEffect.None)
				return _dragControlType.Effect;
			if (_dragScriptItems == nullptr)
			{
				_dragScriptItems = new DragScriptItems(ValidateDragScriptItem);
				_dragHandlers.Add(_dragScriptItems);
			}
			if (_dragScriptItems.OnDragEnter(data) && result == DragDropEffect.None)
				return _dragScriptItems.Effect;
		}*/
		return result;
	}

	DragDropEffect SceneHierarchyWindow::OnDragMove(const Float2& location, DragData* data)
	{
		DragDropEffect result = EditorWindow::OnDragMove(location, data);
		if (result == DragDropEffect::None /*&& Editor.StateMachine.CurrentState.CanEditScene*/ && _dragHandlers != nullptr)
		{
			result = _dragHandlers->GetEffect();
		}
		return result;
	}

	void SceneHierarchyWindow::OnDragLeave()
	{
		EditorWindow::OnDragLeave();

		if (_dragHandlers != nullptr)
		{
			_dragHandlers->OnDragLeave();
		}
	}

	DragDropEffect SceneHierarchyWindow::OnDragDrop(const Float2& location, DragData* data)
	{
		DragDropEffect result = EditorWindow::OnDragDrop(location, data);
		/*if (result == DragDropEffect::None)
		{
			// Drag assets
			if (_dragAssets != nullptr && _dragAssets.HasValidDrag)
			{
				for (int i = 0; i < _dragAssets.Objects.Count; i++)
				{
					var item = _dragAssets.Objects[i];
					if (item.IsOfType<SceneAsset>())
					{
						Editor.Instance.Scene.OpenScene(item.ID, true);
						continue;
					}
					var actor = item.OnEditorDrop(this);
					actor.Name = item.ShortName;
					Level.SpawnActor(actor);
					Editor.Scene.MarkSceneEdited(actor.Scene);
				}
				result = DragDropEffect.Move;
			}
			// Drag actor type
			else if (_dragActorType != nullptr && _dragActorType.HasValidDrag)
			{
				for (int i = 0; i < _dragActorType.Objects.Count; i++)
				{
					var item = _dragActorType.Objects[i];
					var actor = item.CreateInstance() as Actor;
					if (actor == nullptr)
					{
						Editor.LogWarning("Failed to spawn actor of type " + item.TypeName);
						continue;
					}
					actor.Name = item.Name;
					Level.SpawnActor(actor);
					Editor.Scene.MarkSceneEdited(actor.Scene);
				}
				result = DragDropEffect.Move;
			}
			// Drag control type
			else if (_dragControlType != nullptr && _dragControlType.HasValidDrag)
			{
				for (int i = 0; i < _dragControlType.Objects.Count; i++)
				{
					var item = _dragControlType.Objects[i];
					var control = item.CreateInstance() as Control;
					if (control == nullptr)
					{
						Editor.LogWarning("Failed to spawn UIControl with control type " + item.TypeName);
						continue;
					}
					var uiControl = new UIControl
					{
						Control = control,
						Name = item.Name,
					};
					Level.SpawnActor(uiControl);
					Editor.Scene.MarkSceneEdited(uiControl.Scene);
				}
				result = DragDropEffect.Move;
			}
			// Drag script item
			else if (_dragScriptItems != nullptr && _dragScriptItems.HasValidDrag)
			{
				for (int i = 0; i < _dragScriptItems.Objects.Count; i++)
				{
					var item = _dragScriptItems.Objects[i];
					var actorType = Editor.Instance.CodeEditing.Actors.Get(item);
					if (actorType != ScriptType.nullptr)
					{
						var actor = actorType.CreateInstance() as Actor;
						if (actor == nullptr)
						{
							Editor.LogWarning("Failed to spawn actor of type " + actorType.TypeName);
							continue;
						}
						actor.Name = actorType.Name;
						Level.SpawnActor(actor);
						Editor.Scene.MarkSceneEdited(actor.Scene);
					}
				}
				result = DragDropEffect.Move;
			}

			_dragHandlers.OnDragDrop(nullptr);
		}*/
		return result;
	}

	void SceneHierarchyWindow::OnDestroy()
	{
		_dragAssets = nullptr;
		_dragHandlers->Clear();
		_dragHandlers = nullptr;
		Delete(m_Tree);;
		Delete(m_SearchBox);

		EditorWindow::OnDestroy();
	}

	void SceneHierarchyWindow::ScrollToSelectedNode()
	{
		// Scroll to node
		auto& nodeSelection = m_Tree->Selection;
		if (nodeSelection.Count() != 0)
		{
			auto scrollControl = nodeSelection[nodeSelection.Count() - 1];
			m_SceneTreePanel->ScrollViewTo(scrollControl);
		}
	}

	void SceneHierarchyWindow::OnSearchBoxTextChanged()
	{
		// Skip events during setup or init stuff
		if (GetIsLayoutLocked())
			return;

		m_Tree->LockChildrenRecursive();

		// Update tree
		String& query = m_SearchBox->Text;
		/*var root = Editor.Scene.Root;
		root.TreeNode.UpdateFilter(query);*/

		m_Tree->UnlockChildrenRecursive();
		PerformLayout();
	}

	void SceneHierarchyWindow::Rename()
	{
		List<ScenesGraphNode*>& selection = editor->sceneModule->Selection;
		ActorGraphNode* actor;
		if (selection.Count() != 0 && TypeTryCast(selection[0], actor))
		{
			if (selection.Count() != 0)
			{
				editor->sceneModule->Select(actor);
			}
			actor->TreeNode->StartRenaming(this, m_SceneTreePanel);
		}
	}

	void SceneHierarchyWindow::Spawn(TypeID type)
	{
		SceneModule* sceneModule = editor->sceneModule;

		// Create actor
		Actor* actor = (Actor*)Types::CreateInstance(type);
		if (actor == nullptr)
		{
			return;
		}

		Actor* parentActor = nullptr;
		ActorGraphNode* actorNode;
		if (editor->sceneModule->HasSthSelected && TypeTryCast(sceneModule->Selection[0], actorNode))
		{
			parentActor = actorNode->Actor;
			actorNode->TreeNode->Expand();
		}
		if (parentActor == nullptr)
		{
			List<Scene*>& scenes = Level::Scenes;
			if (scenes.Count() > 0)
			{
				parentActor = scenes[scenes.Count() - 1];
			}
		}
		if (parentActor != nullptr)
		{
			// Use the same location
			actor->SetTransform(parentActor->GetTransform());

			// Rename actor to identify it easily
			actor->SetName(type.ToString());
		}

		// Spawn it
		sceneModule->Spawn(actor, parentActor);

		sceneModule->Select(actor);
		Rename();
	}

	void SceneHierarchyWindow::Tree_OnSelectedChanged(List<TreeNode*>& before, List<TreeNode*>& after)
	{
		// Check if lock events
		if (_isUpdatingSelection)
			return;

		SceneModule* sceneModule = editor->sceneModule;

		if (after.Count() > 0)
		{
			// Get actors from nodes
			List<ScenesGraphNode*> actors = List<ScenesGraphNode*>(after.Count());
			for (int i = 0; i < after.Count(); i++)
			{
				ActorTreeNode* node;
				if (TypeTryCast(after[i], node) && node->GetActor())
				{
					actors.Add(node->GetActorGraphNode());
				}
			}

			// Select
			sceneModule->Select(actors);
		}
		else
		{
			// Deselect
			sceneModule->Deselect();
		}
	}

	void SceneHierarchyWindow::OnTreeRightClick(TreeNode* node, Float2 location)
	{
		/*if (!Editor.StateMachine.CurrentState.CanEditScene)
			return;*/
		ShowContextMenu(node, location);
	}

	void SceneHierarchyWindow::OnSelectionChanged()
	{
		_isUpdatingSelection = true;

		List<ScenesGraphNode*> selection = EditorApp::Ins().sceneModule->Selection;
		if (selection.Count() == 0)
		{
			m_Tree->Deselect();
		}
		else
		{
			// Find nodes to select
			List<TreeNode*> nodes = List<TreeNode*>(selection.Count());
			for (int i = 0; i < selection.Count(); i++)
			{
				ActorGraphNode* node;
				if (TypeTryCast<ActorGraphNode>(selection[i], node))
				{
					nodes.Add(node->TreeNode);
				}
			}

			// Select nodes
			m_Tree->Select(nodes);

			// For single node selected scroll view so user can see it
			if (nodes.Count() == 1)
			{
				nodes[0]->ExpandAllParents(true);
				m_SceneTreePanel->ScrollViewTo(nodes[0]);
			}
		}

		_isUpdatingSelection = false;
	}

	bool SceneHierarchyWindow::ValidateDragAsset(AssetItem* assetItem)
	{
		/*if (assetItem->IsOfType<SceneAsset>())
		{
			return true;
		}*/
		return assetItem->OnEditorDrag(this);
	}


	ContextMenu* SceneHierarchyWindow::CreateContextMenu()
	{
		// Prepare

		bool hasSthSelected = editor->sceneModule->HasSthSelected;
		bool isSingleActorSelected = editor->sceneModule->SelectionCount == 1 && TypeIs<ActorGraphNode>(editor->sceneModule->Selection[0]);
		bool canEditScene = /*Editor.StateMachine.CurrentState.CanEditScene && */Level::IsAnySceneLoaded();
		// var inputOptions = Editor.Options.Options.Input;

		// Create popup

		ContextMenu* contextMenu = New<ContextMenu>();
		contextMenu->MinimumWidth = 120;

		// Expand/collapse

		ContextMenuButton* b = contextMenu->AddButton(SE_TEXT("Expand All"), CreateFunc<SceneHierarchyWindow, &SceneHierarchyWindow::OnExpandAllClicked>(this));
		b->Enabled = hasSthSelected;

		b = contextMenu->AddButton(SE_TEXT("Collapse All"), CreateFunc<SceneHierarchyWindow, &SceneHierarchyWindow::OnCollapseAllClicked>(this));
		b->Enabled = hasSthSelected;

		/*if (hasSthSelected)
		{
			contextMenu->AddButton(Editor.Windows.EditWin.IsPilotActorActive ? "Stop piloting actor" : "Pilot actor", inputOptions.PilotActor, Editor.UI.PilotActor);
		}*/

		contextMenu->AddSeparator();

		// Basic editing options

		b = contextMenu->AddButton(SE_TEXT("Rename"), CreateFunc<SceneHierarchyWindow, &SceneHierarchyWindow::Rename>(this));
		b->Enabled = isSingleActorSelected;

		b = contextMenu->AddButton(SE_TEXT("Duplicate"), CreateFunc<SceneModule, &SceneModule::Duplicate>(editor->sceneModule));
		b->Enabled = hasSthSelected;

		/*if (isSingleActorSelected)
		{
			var convertMenu = contextMenu->AddChildMenu(SE_TEXT("Convert"));
			convertMenu.ContextMenu.AutoSort = true;
			foreach (var actorType in Editor.CodeEditing.Actors.Get())
			{
				if (actorType.IsAbstract)
					continue;
				ActorContextMenuAttribute attribute = null;
				foreach (var e in actorType.GetAttributes(false))
				{
					if (e is ActorContextMenuAttribute actorContextMenuAttribute)
					{
						attribute = actorContextMenuAttribute;
						break;
					}
				}
				if (attribute == null)
					continue;
				var parts = attribute.Path.Split('/');
				ContextMenuChildMenu childCM = convertMenu;
				bool mainCM = true;
				for (int i = 0; i < parts.Length; i++)
				{
					var part = parts[i].Trim();
					if (i == parts.Length - 1)
					{
						if (mainCM)
						{
							convertMenu.ContextMenu.AddButton(part, () => Editor.SceneEditing.Convert(actorType.Type));
							mainCM = false;
						}
						else
						{
							childCM.ContextMenu.AddButton(part, () => Editor.SceneEditing.Convert(actorType.Type));
							childCM.ContextMenu.AutoSort = true;
						}
					}
					else
					{
						// Remove new path for converting menu
						if (parts[i] == "New")
							continue;

						if (mainCM)
						{
							childCM = convertMenu.ContextMenu.GetOrAddChildMenu(part);
							childCM.ContextMenu.AutoSort = true;
							mainCM = false;
						}
						else
						{
							childCM = childCM.ContextMenu.GetOrAddChildMenu(part);
							childCM.ContextMenu.AutoSort = true;
						}
					}
				}
			}
		}*/
		b = contextMenu->AddButton(SE_TEXT("Delete"), CreateFunc<SceneModule, &SceneModule::Delete>(editor->sceneModule));
		b->Enabled = hasSthSelected;

		contextMenu->AddSeparator();

		b = contextMenu->AddButton(SE_TEXT("Copy"), CreateFunc<SceneModule, &SceneModule::Copy>(editor->sceneModule));

		b->Enabled = hasSthSelected;
		contextMenu->AddButton(SE_TEXT("Paste"), CreateFunc<SceneModule, &SceneModule::Paste>(editor->sceneModule));

		b = contextMenu->AddButton(SE_TEXT("Cut"), CreateFunc<SceneModule, &SceneModule::Cut>(editor->sceneModule));
		b->Enabled = canEditScene;

		// Create option

		/*contextMenu->AddSeparator();

		b = contextMenu->AddButton("Create Prefab", Editor.Prefabs.CreatePrefab);
		b->Enabled = isSingleActorSelected &&
					((ActorNode)Editor.SceneEditing.Selection[0]).CanCreatePrefab &&
					Editor.Windows.ContentWin.CurrentViewFolder.CanHaveAssets;

		bool hasPrefabLink = canEditScene && isSingleActorSelected && (Editor.SceneEditing.Selection[0] as ActorNode).HasPrefabLink;
		if (hasPrefabLink)
		{
			contextMenu->AddButton("Select Prefab", Editor.Prefabs.SelectPrefab);
			contextMenu->AddButton("Break Prefab Link", Editor.Prefabs.BreakLinks);
		}*/

		// Load additional scenes option

		/*if (!hasSthSelected)
		{
			var allScenes = FlaxEngine.Content.GetAllAssetsByType(typeof(SceneAsset));
			var loadedSceneIds = Editor.Instance.Scene.Root.ChildNodes.Select(node => node.ID).ToList();
			var unloadedScenes = allScenes.Where(sceneId => !loadedSceneIds.Contains(sceneId)).ToList();
			if (unloadedScenes.Count > 0)
			{
				contextMenu.AddSeparator();
				var childCM = contextMenu.GetOrAddChildMenu("Open Scene");
				foreach (var sceneGuid in unloadedScenes)
				{
					if (FlaxEngine.Content.GetAssetInfo(sceneGuid, out var unloadedScene))
					{
						var splitPath = unloadedScene.Path.Split('/');
						var sceneName = splitPath[^1];
						if (splitPath[^1].EndsWith(".scene"))
							sceneName = sceneName[..^6];
						childCM.ContextMenu.AddButton(sceneName, () => { Editor.Instance.Scene.OpenScene(sceneGuid, true); });
					}
				}
			}
		}*/

		// Spawning actors options

		contextMenu->AddSeparator();

		ContextMenuChildMenu* createMenu = contextMenu->AddChildMenu(SE_TEXT("Create"));
		// go through each actor and add it to the context menu if it has the ActorContextMenu attribute
		List<String> splitPath;
		for (auto actorMenu : m_ActorCreateMenus)
		{
			actorMenu.First.Split(SE_TEXT('/'), splitPath);
			TypeID actorType = actorMenu.Second;

			ContextMenuChildMenu* childCM = nullptr;
			bool mainCM = true;
			for (int i = 0; i < splitPath.Count(); i++)
			{
				if (i == splitPath.Count() - 1)
				{
					if (mainCM)
					{
						createMenu->ContextMenu->AddButton(splitPath[i].TrimTrailing(), CreateFunc([this, actorType]()
						{
							Spawn(actorType);
						}));
						mainCM = false;
					}
					else
					{
						childCM->ContextMenu->AddButton(splitPath[i].TrimTrailing(), CreateFunc([this, actorType]() {Spawn(actorType);}));
						childCM->ContextMenu->AutoSort = true;
					}
				}
				else
				{
					if (mainCM)
					{
						childCM = createMenu->ContextMenu->GetOrAddChildMenu(splitPath[i].TrimTrailing());
						mainCM = false;
					}
					else
					{
						childCM = childCM->ContextMenu->GetOrAddChildMenu(splitPath[i].TrimTrailing());
					}
					childCM->ContextMenu->AutoSort = true;
				}
			}
		}

		// Custom options

		/*bool showCustomNodeOptions = Editor.SceneEditing.Selection.Count == 1;
		if (!showCustomNodeOptions && Editor.SceneEditing.Selection.Count != 0)
		{
			showCustomNodeOptions = true;
			for (int i = 1; i < Editor.SceneEditing.Selection.Count; i++)
			{
				if (Editor.SceneEditing.Selection[0].GetType() != Editor.SceneEditing.Selection[i].GetType())
				{
					showCustomNodeOptions = false;
					break;
				}
			}
		}
		if (showCustomNodeOptions)
		{
			Editor.SceneEditing.Selection[0].OnContextMenu(contextMenu, this);
		}

		ContextMenuShow?.Invoke(contextMenu);*/

		return contextMenu;
	}

	void SceneHierarchyWindow::OnExpandAllClicked(ContextMenuButton* button)
	{
		int selectionCount = editor->sceneModule->SelectionCount;
		for (int i = 0; i < selectionCount; i++)
		{
			ActorGraphNode* node;
			if (TypeTryCast(editor->sceneModule->Selection[i], node))
			{
				node->TreeNode->ExpandAll();
			}
		}
	}

	void SceneHierarchyWindow::OnCollapseAllClicked(ContextMenuButton* button)
	{
		int selectionCount = editor->sceneModule->SelectionCount;
		for (int i = 0; i < selectionCount; i++)
		{
			ActorGraphNode* node;
			if (TypeTryCast(editor->sceneModule->Selection[i], node))
			{
				node->TreeNode->CollapseAll();
			}
		}
	}
} // SE