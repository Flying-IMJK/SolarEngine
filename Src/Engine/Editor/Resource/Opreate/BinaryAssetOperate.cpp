
#include "BinaryAssetOperate.h"

#include "Editor/Resource/Items/BinaryAssetItem.h"
#include "Runtime/Render/Assets/Geometry/Model.h"

namespace SE::Editor
{
	bool BinaryAssetOperate::IsProxyFor(ContentItem* item)
	{
		BinaryAssetItem* binaryAssetItem = TypeTryCast<BinaryAssetItem>(item);;
		return binaryAssetItem != nullptr&& GetAssetType() == binaryAssetItem->typeID;
	}

	bool BinaryAssetOperate::IsProxyFor(TypeID typeID)
	{
		return typeID == GetAssetType();
	}

	AssetItem* BinaryAssetOperate::ConstructItem(StringView path, TypeID typeId, UID id)
	{
		if (Types::IsTypeDerivedFrom(Typeof<TextureBase>(), typeId))
			return New<TextureAssetItem>(path, id, typeId);
		if (Types::IsTypeDerivedFrom(Typeof<Model>(), typeId))
			return New<ModelItem>(path, id, typeId);
		/*if (typeof(SkinnedModel).IsAssignableFrom(type))
			return new SkinnedModeItem(path, ref id, typeName, type);*/

		ContentItemSearchFilter searchFilter;
		/*if (typeof(MaterialBase).IsAssignableFrom(type))
			searchFilter = ContentItemSearchFilter.Material;
		else if (typeof(Prefab).IsAssignableFrom(type))
			searchFilter = ContentItemSearchFilter.Prefab;
		else if (typeof(SceneAsset).IsAssignableFrom(type))
			searchFilter = ContentItemSearchFilter.Scene;
		else if (typeof(Animation).IsAssignableFrom(type))
			searchFilter = ContentItemSearchFilter.Animation;
		else if (typeof(ParticleEmitter).IsAssignableFrom(type))
			searchFilter = ContentItemSearchFilter.Particles;
		else*/
			searchFilter = ContentItemSearchFilter::Other;
		return New<BinaryAssetItem>(path, id, typeId, searchFilter);
	}

	String BinaryAssetOperate::__GetFileExtension()
	{
		return SE_TEXT("sge");
	}
} // SE