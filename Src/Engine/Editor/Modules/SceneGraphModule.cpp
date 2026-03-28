
#include "SceneGraphModule.h"

#include "Editor/SceneGraph/ActorGraphNode.h"
#include "Editor/SceneGraph/Actor/CameraGraphNode.h"
#include "Editor/SceneGraph/Actor/DirectionalLightGraphNode.h"
#include "Editor/SceneGraph/Actor/PointLightGraphNode.h"
#include "Editor/SceneGraph/Actor/SceneGraphNode.h"
#include "Editor/SceneGraph/Actor/SkyGraphNode.h"
#include "Editor/SceneGraph/Actor/StaticModelGraphNode.h"
#include "Runtime/Level/Actors/Camera.h"
#include "Runtime/Level/Actors/PointLight.h"
#include "Runtime/Level/Actors/Sky.h"
#include "Runtime/Level/Actors/Staticmodel.h"
#include "Runtime/Level/Scene/Scene.h"


namespace SE::Editor
{
	int SceneGraphModule::InitOrder()
	{
		return 0;
	}

	void SceneGraphModule::OnInit()
	{
        // Init default nodes types
		CustomNodesTypes.Add(Typeof<Scene>(), NodeFactory(Typeof<SceneGraphNode>(), [](Actor* actor)
		{
			return New<SceneGraphNode>((Scene*)actor);
		}));
		CustomNodesTypes.Add(Typeof<Camera>(), NodeFactory(Typeof<CameraGraphNode>(), [](Actor* actor)
		{
			return New<CameraGraphNode>((Camera*)actor);
		}));
		CustomNodesTypes.Add(Typeof<StaticModel>(), NodeFactory(Typeof<StaticModelGraphNode>(), [](Actor* actor)
		{
			return New<StaticModelGraphNode>((StaticModel*)actor);
		}));
		CustomNodesTypes.Add(Typeof<Sky>(), NodeFactory(Typeof<SkyGraphNode>(), [](Actor* actor)
		{
			return New<SkyGraphNode>((Sky*)actor);
		}));
		CustomNodesTypes.Add(Typeof<DirectionalLight>(), NodeFactory(Typeof<DirectionalLightGraphNode>(), [](Actor* actor)
		{
			return New<DirectionalLightGraphNode>((DirectionalLight*)actor);
		}));
		CustomNodesTypes.Add(Typeof<PointLight>(), NodeFactory(Typeof<PointLightGraphNode>(), [](Actor* actor)
		{
			return New<PointLightGraphNode>((PointLight*)actor);
		}));
        /*CustomNodesTypes.Add(Typeof<EnvironmentProbe>, Typeof<EnvironmentProbeNode>);
        CustomNodesTypes.Add(Typeof<PointLight>, Typeof<PointLightNode>);
        CustomNodesTypes.Add(Typeof<DirectionalLight>, Typeof<DirectionalLightNode>);
        CustomNodesTypes.Add(Typeof<SpotLight), Typeof<SpotLightNode>);
        CustomNodesTypes.Add(Typeof<Skybox), Typeof<SkyboxNode>);
        CustomNodesTypes.Add(Typeof<Sky), Typeof<SkyNode>);
        CustomNodesTypes.Add(Typeof<ExponentialHeightFog), Typeof<ExponentialHeightFogNode>);
        CustomNodesTypes.Add(Typeof<SkyLight), Typeof<SkyLightNode>);
        CustomNodesTypes.Add(Typeof<PostFxVolume), Typeof<PostFxVolumeNode>);
        CustomNodesTypes.Add(Typeof<StaticModel), Typeof<StaticModelNode>);
        CustomNodesTypes.Add(Typeof<AnimatedModel), Typeof<AnimatedModelNode>);
        CustomNodesTypes.Add(Typeof<BoxBrush), Typeof<BoxBrushNode>);
        CustomNodesTypes.Add(Typeof<TextRender), Typeof<TextRenderNode>);
        CustomNodesTypes.Add(Typeof<AudioListener), Typeof<AudioListenerNode>);
        CustomNodesTypes.Add(Typeof<AudioSource), Typeof<AudioSourceNode>);
        CustomNodesTypes.Add(Typeof<BoneSocket), Typeof<BoneSocketNode>);
        CustomNodesTypes.Add(Typeof<Decal), Typeof<DecalNode>);
        CustomNodesTypes.Add(Typeof<BoxCollider), Typeof<BoxColliderNode>);
        CustomNodesTypes.Add(Typeof<SphereCollider), Typeof<ColliderNode>);
        CustomNodesTypes.Add(Typeof<CapsuleCollider), Typeof<ColliderNode>);
        CustomNodesTypes.Add(Typeof<MeshCollider), Typeof<ColliderNode>);
        CustomNodesTypes.Add(Typeof<CharacterController), Typeof<ColliderNode>);
        CustomNodesTypes.Add(Typeof<UICanvas), Typeof<UICanvasNode>);
        CustomNodesTypes.Add(Typeof<UIControl), Typeof<UIControlNode>);
        CustomNodesTypes.Add(Typeof<Terrain), Typeof<TerrainNode>);
        CustomNodesTypes.Add(Typeof<Foliage), Typeof<FoliageNode>);
        CustomNodesTypes.Add(Typeof<NavMeshBoundsVolume), Typeof<NavMeshBoundsVolumeNode>);
        CustomNodesTypes.Add(Typeof<BoxVolume), Typeof<BoxVolumeNode>);
        CustomNodesTypes.Add(Typeof<NavLink), Typeof<NavLinkNode>);
        CustomNodesTypes.Add(Typeof<NavModifierVolume), Typeof<NavModifierVolumeNode>);
        CustomNodesTypes.Add(Typeof<ParticleEffect), Typeof<ParticleEffectNode>);
        CustomNodesTypes.Add(Typeof<SceneAnimationPlayer), Typeof<SceneAnimationPlayerNode>);
        CustomNodesTypes.Add(Typeof<Spline), Typeof<SplineNode>);
        CustomNodesTypes.Add(Typeof<SplineModel), Typeof<ActorNode>);
        CustomNodesTypes.Add(Typeof<SplineCollider), Typeof<ColliderNode>);
        CustomNodesTypes.Add(Typeof<SplineRopeBody), Typeof<ActorNode>);
        CustomNodesTypes.Add(Typeof<NavMesh), Typeof<ActorNode>);
        CustomNodesTypes.Add(Typeof<SpriteRender), Typeof<SpriteRenderNode>);
        CustomNodesTypes.Add(Typeof<Joint), Typeof<JointNode>);*/
	}

	SceneGraphModule::SceneGraphModule(EditorApp* editorApp) : EditorModule(editorApp)
	{
	}

	ScenesGraphNode* SceneGraphModule::FindNode(UID id)
	{
		if (id == UID::Empty)
			return nullptr;

		ScenesGraphNode* result;
		Nodes.TryGet(id, result);
		return result;
	}

	ScenesGraphNode* SceneGraphModule::GetNode(UID id)
	{
		ScenesGraphNode* result;
		Nodes.TryGet(id, result);
		if (result == nullptr)
		{
			/*Actor* actor = Object.TryFind<Actor>(id);
			if (actor != null)
			{
				result = BuildActorNode(actor);
				if (result != nullptr && actor.HasParent)
				{
					result->ParentNode = FindNode(actor.Parent.ID);
				}
			}*/
		}
		return result;
	}

	SceneGraphNode* SceneGraphModule::BuildSceneTree(Scene* scene)
	{
		return (SceneGraphNode*)BuildActorNode(scene);
	}

	ActorGraphNode* SceneGraphModule::BuildActorNode(Actor* actor)
	{
		ActorGraphNode* result = nullptr;

		// Try to pick custom node type for that actor object
		TypeID type = actor->GetType();
		NodeFactory nodeFactory;
		if (CustomNodesTypes.TryGet(type, nodeFactory))
		{
			// Use custom type
			result = (ActorGraphNode*)nodeFactory.CreateFunc(actor);
		}
		else
		{
			// Use default type for actors
			result = New<ActorGraphNode>(actor);
		}

		result->LinkTreeNode();

		// Build children
		int childrenCount = actor->GetChildrenCount();
		for (int i = 0; i < childrenCount; i++)
		{
			auto child = actor->GetChild(i);
			if (child == nullptr)
			{
				continue;
			}

			auto childNode = BuildActorNode(child);
			if (childNode != nullptr)
			{
				childNode->ParentNode = result;
			}
		}


		return result;
	}
} // SE