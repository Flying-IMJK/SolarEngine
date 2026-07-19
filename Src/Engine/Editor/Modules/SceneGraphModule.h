#pragma once
#include "EditorModule.h"
#include "Runtime/Core/Types/UID.h"
#include "Runtime/Core/Types/Collections/Dictionary.h"
#include "Runtime/Core/TypeSystem/TypeID.h"
#include "Runtime/Core/Types/Delegate.h"

namespace SE
{
    class Actor;
    class Scene;
}

namespace SE::Editor
{
	class SceneGraphNode;
	class ActorGraphNode;
    class ScenesGraphNode;

    class SceneGraphModule : public EditorModule
	{
	public:
    	struct NodeFactory
    	{
    		TypeID typeID;
    		Function<ScenesGraphNode*(Actor*)> CreateFunc;

    		NodeFactory() {}
    		NodeFactory(TypeID typeID, const Function<ScenesGraphNode*(Actor*)>& createFunc) : typeID(typeID), CreateFunc(createFunc) {}
    	};

    	/// <summary>
    	/// The custom nodes types. Key = object type, Value = custom graph node type
    	/// </summary>
    	Dictionary<TypeID, NodeFactory> CustomNodesTypes = Dictionary<TypeID, NodeFactory>();

    	/// <summary>
    	/// The nodes collection where key is ID.
    	/// </summary>
    	Dictionary<UID, ScenesGraphNode*> Nodes = Dictionary<UID, ScenesGraphNode*>();

        int InitOrder() override;
        void OnInit() override;

    	SceneGraphModule(EditorApp* editorApp);

        /// <summary>
        /// Tries to find the node by the given ID.
        /// </summary>
        /// <param name="id">The identifier.</param>
        /// <returns>Found node or null if cannot.</returns>
        ScenesGraphNode* FindNode(UID id);

        /// <summary>
        /// Gets the node for the given object ID or creates a new node for actors (with automatic linkage to the parent).
        /// </summary>
        /// <param name="id">The identifier.</param>
        /// <returns>The result node.</returns>
        ScenesGraphNode* GetNode(UID id);

        /// <summary>
        /// Builds the scene tree.
        /// </summary>
        /// <param name="scene">The scene.</param>
        /// <returns>The root scene node.</returns>
        SceneGraphNode* BuildSceneTree(Scene* scene);

        /// <summary>
        /// Builds the actor node. Warning! Don't create duplicated nodes.
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <returns>Created node or null if failed.</returns>
        ActorGraphNode* BuildActorNode(Actor* actor);
	};

} // SE
