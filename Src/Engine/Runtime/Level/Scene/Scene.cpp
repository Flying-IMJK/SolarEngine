

#include "Scene.h"
#include "SceneAsset.h"

#include "Core/Platform/Windows/WindowsFileSystem.h"
#include "Core/Serialization/Serialization.h"
#include "Runtime/Resource/AssetContent.h"
#include "Runtime/Resource/AssetInfo.h"
#include "Runtime/Resource/Factories/IAssetFactory.h"
#include "Runtime/Resource/Factories/JsonAssetFactory.h"

namespace SE
{
	JSON_ASSET_FACTORY(SceneAsset);

	SceneAsset::SceneAsset(const AssetInfo* info) : JsonAsset(info)
	{
	}

	bool SceneAsset::IsInternalType() const
	{
		return true;
	}

	Scene::Scene()
	{
		m_Name = SE_TEXT("Scene");
	}

	Scene::~Scene()
	{
	}

#if SE_EDITOR
	String Scene::GetPath() const
	{
		AssetInfo info;
		if (AssetContent::GetAssetInfo(GetInstanceID(), info))
		{
			return info.path;
		}

		return String::Empty;
	}

	String Scene::GetFilename() const
	{
		return FileSystem::GetFileNameWithoutExtension(GetPath());;
	}

	String Scene::GetDataFolderPath() const
	{
		return String::Empty;
	}

	List<UID, HeapAllocation> Scene::GetAssetReferences() const
	{
		List<UID> result;

		return result;
	}
#endif

	void Scene::Serialize(SerializeContext& context)
	{
		// Base
		Actor::Serialize(context);

		SERIALIZE_GET_OTHER_OBJ(Scene, context.otherObj);

		/*LightmapsData.SaveLightmaps(Info.Lightmaps);
		Info.Serialize(stream, other ? &other->Info : nullptr);

		if (CSGData.HasData())
		{
			stream.JKEY("CSG");
			stream.Object(&CSGData, other ? &other->CSGData : nullptr);
		}*/
	}

	void Scene::Deserialize(DeserializeContext& context)
	{
		// Base
		Actor::Deserialize(context);
	}

	void Scene::OnDeleteObject()
	{
		// Cleanup
		/*LightmapsData.UnloadLightmaps();
		CSGData.Model = nullptr;
		CSGData.CollisionData = nullptr;*/

		// Base
		Actor::OnDeleteObject();
	}

	void Scene::Initialize()
	{
		// Initialize
		m_Parent = nullptr;
		m_Scene = this;

		Actor::Initialize();
	}

	void Scene::OnTransformChanged()
	{
		// Base
		Actor::OnTransformChanged();

		m_Box = BoundingBox(m_Transform.Translation);
		m_Sphere = BoundingSphere(m_Transform.Translation, 0.0f);
	}
} // SE