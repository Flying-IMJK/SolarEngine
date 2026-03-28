#pragma once
#include "EditorModule.h"
#include "Core/Types/Property.h"
#include "Core/Types/Collections/Dictionary.h"
#include "Core/Types/Collections/List.h"
#include "Core/TypeSystem/TypeID.h"
#include "Editor/EditorApp.h"
#include "Editor/SceneGraph/RootGraphNode.h"

namespace SE
{
	class ContainerControl;
	class Actor;
	class Scene;
}

namespace SE::Editor
{
	class SceneGraphNode;
	class ActorGraphNode;
	class ScenesGraphNode;

	class SceneModule : public EditorModule
	{
	public:

		/// <summary>
		/// The root node for the scene graph created for the loaded scenes and actors hierarchy.
		/// </summary>
		/// <seealso cref="FlaxEditor.SceneGraph.RootNode" />
		class ScenesRootNode : public RootGraphNode
		{
		private:
			EditorApp& _editor;

		public:
			/// <inheritdoc />
			ScenesRootNode() : _editor(EditorApp::Ins())
			{
			}

			/// <inheritdoc />
			void Spawn(::SE::Actor* actor, ::SE::Actor* parent) override
			{
				// _editor.SceneEditing.Spawn(actor, parent);
			}

		protected:
			/// <inheritdoc />
			// public override Undo Undo => Editor.Instance.Undo;
			/// <inheritdoc />
			List<ScenesGraphNode*>& __GetSelection() override;
		};

		/// <summary>
		/// The root tree node for the whole scene graph.
		/// </summary>
		ScenesRootNode* Root;

		/// <summary>
		/// The selected objects.
		/// </summary>
		List<ScenesGraphNode*> Selection = List<ScenesGraphNode*>(64);

		/// <summary>
		/// Occurs when selected objects collection gets changed.
		/// </summary>
		Action SelectionChanged;

		/// <summary>
		/// Gets the amount of the selected objects.
		/// </summary>
		PRO_GET(SelectionCount, SceneModule, int, __GetSelectionCount);

		/// <summary>
		/// Gets a value indicating whether any object is selected.
		/// </summary>
		PRO_GET(HasSthSelected, SceneModule, bool, __GetHasSthSelected);

		/// <summary>
		/// Selects the specified actor (finds it's scene graph node).
		/// </summary>
		/// <param name="actor">The actor.</param>
		void Select(Actor* actor);

		/// <summary>
		/// Selects the specified collection of objects.
		/// </summary>
		/// <param name="selection">The selection.</param>
		/// <param name="additive">if set to <c>true</c> will use additive mode, otherwise will clear previous selection.</param>
		void Select(List<ScenesGraphNode*>& selection, bool additive = false);


		/// <summary>
		/// Selects the specified object.
		/// </summary>
		/// <param name="selection">The selection.</param>
		/// <param name="additive">if set to <c>true</c> will use additive mode, otherwise will clear previous selection.</param>
		void Select(ScenesGraphNode* selection, bool additive = false);

        /// <summary>
        /// Duplicates the selected objects. Supports undo/redo.
        /// </summary>
        void Duplicate();

        /// <summary>
        /// Deletes the selected objects. Supports undo/redo.
        /// </summary>
		void Delete();

        /// <summary>
        /// Copies the selected objects.
        /// </summary>
		void Copy();

        /// <summary>
        /// Pastes the copied objects. Supports undo/redo.
        /// </summary>
		void Paste()
        {
            PasteActor(nullptr);
        }

        /// <summary>
        /// Pastes the copied objects. Supports undo/redo.
        /// </summary>
        /// <param name="pasteTargetActor">The target actor to paste copied data.</param>
		void PasteActor(Actor* pasteTargetActor);

        /// <summary>
        /// Cuts the selected objects. Supports undo/redo.
        /// </summary>
		void Cut()
        {
            Copy();
            Delete();
        }

		/// <summary>
		/// Spawns the specified actor to the game (with undo).
		/// </summary>
		/// <param name="actor">The actor.</param>
		/// <param name="parent">The parent actor. Set null as default.</param>
		/// <param name="autoSelect">True if automatically select the spawned actor, otherwise false.</param>
		void Spawn(Actor* actor, Actor* parent = nullptr, bool autoSelect = true);

		/// <summary>
		/// Gets the actor node.
		/// </summary>
		/// <param name="actor">The actor.</param>
		/// <returns>Found actor node or null if missing. Actor may not be linked to the scene tree so node won't be found in that case.</returns>
		ActorGraphNode* GetActorNode(Actor* actor);

		/// <summary>
		/// Gets the actor node.
		/// </summary>
		/// <param name="actorId">The actor id.</param>
		/// <returns>Found actor node or null if missing. Actor may not be linked to the scene tree so node won't be found in that case.</returns>
		ActorGraphNode* GetActorNode(UID actorId);


		/// <summary>
		/// Clears selected objects collection.
		/// </summary>
		void Deselect();

		void OnInit() override;

		void OnExit() override;

		void CreateSceneFile(StringView path);

		/// <summary>
		/// Saves scene (async).
		/// </summary>
		/// <param name="scene">Scene to save.</param>
		void SaveScene(Scene* scene);

		/// <summary>
		/// Saves scene (async).
		/// </summary>
		/// <param name="scene">Scene to save.</param>
		void SaveScene(SceneGraphNode* scene);

		/// <summary>
		/// Opens scene (async).
		/// </summary>
		/// <param name="sceneId">Scene ID</param>
		/// <param name="additive">True if don't close opened scenes and just add new scene to them, otherwise will release current scenes and load single one.</param>
		void OpenScene(UID sceneId, bool additive = false);

		void OnSaveStart(ContainerControl* uiRoot);

		void OnSaveEnd(ContainerControl* uiRoot);

		int InitOrder() override;

		SceneModule(EditorApp* editor);

	private:
		void OnSceneSaving(Scene* scene, const UID &sceneId);

		void OnSceneSaved(Scene* scene, const UID & sceneId);

		void OnSceneSaveError(Scene* scene, const UID & sceneId);

		void OnSceneLoaded(Scene* scene, const UID & sceneId);

		void OnSceneUnloading(Scene* scene, const UID & sceneId);

		void OnActorSpawned(Actor* actor);

		void OnActorDeleted(Actor* actor);

		void OnActorDeleted(ActorGraphNode* node);

		void OnActorParentChanged(Actor* actor, Actor* prevParent);

		void OnActorOrderInParentChanged(Actor* actor);

		void OnActorNameChanged(Actor* actor);

		void OnActorActiveChanged(Actor* actor);
		
		int __GetSelectionCount();
		bool __GetHasSthSelected();

		Dictionary<ContainerControl*, Float2>* m_UiRootSizes;
	};
} // SE

