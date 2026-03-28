
#include "BinaryAssetItem.h"

#include "Runtime/Level/Actors/StaticModel.h"
#include "Runtime/Render/Assets/Geometry/Model.h"

namespace SE::Editor
{
	BinaryAssetItem::BinaryAssetItem(StringView path, UID id, TypeID typeID, ContentItemSearchFilter searchFilter) : AssetItem(path, typeID, id)
	{
		m_SearchFilter = searchFilter;
	}

	bool BinaryAssetItem::GetImportPath(String& importPath)
	{
		// TODO: add internal call to content backend with fast import asset metadata gather (without asset loading)

		BinaryAsset* asset = AssetContent::Load<BinaryAsset>(id, 100);
		if (asset)
		{
			// Get meta from loaded asset
			importPath = asset->GetPath();
			return importPath.Length() > 0;
		}

		importPath = String::Empty;
		return true;
	}

	void BinaryAssetItem::OnReimport(UID id)
	{
		m_ID = id;
		AssetItem::OnReimport();
	}

	bool BinaryAssetItem::IsOfType(TypeID type)
	{
		return Types::IsTypeDerivedFrom(type, typeID);
	}

	ContentItemSearchFilter BinaryAssetItem::__GetSearchFilter()
	{
		return  m_SearchFilter;
	}

	TextureAssetItem::TextureAssetItem(StringView path, UID id, TypeID typeID) : BinaryAssetItem(path, id, typeID, ContentItemSearchFilter::Texture)
	{
	}

	void TextureAssetItem::OnBuildTooltipText(StringBuilder sb)
	{
		BinaryAssetItem::OnBuildTooltipText(sb);

		TextureBase* asset = AssetContent::Load<TextureBase>(id, 100);
		if (asset)
		{
			sb.Append("Format: ").Append(PixelFormatGetString(asset->Format())).AppendLine();
			sb.Append("Size: ").Append(asset->Width()).Append('x').Append(asset->Height());
			if (asset->GetArraySize() != 1)
				sb.Append('[').Append(asset->GetArraySize()).Append(']');
			sb.AppendLine();
			sb.Append("Mip Levels: ").Append(asset->GetMipLevels()).AppendLine();
		}
	}

	ModelItem::ModelItem(StringView path, UID id, TypeID typeID) : BinaryAssetItem(path, id, typeID, ContentItemSearchFilter::Model)
	{
	}

	Actor* ModelItem::OnEditorDrop(void* context)
	{
		StaticModel* model = New<StaticModel>();
		model->Model = AssetContent::LoadAsync<Model>(id);
		return model;
	}

	void ModelItem::OnBuildTooltipText(StringBuilder sb)
	{
		BinaryAssetItem::OnBuildTooltipText(sb);

		Model* asset = AssetContent::Load<Model>(id, 100);
		if (asset)
		{
			auto& lods = asset->LODs;
			int triangleCount = 0, vertexCount = 0;
			for (int lodIndex = 0; lodIndex < lods.Count(); lodIndex++)
			{
				ModelLOD& lod = lods[lodIndex];
				for (int meshIndex = 0; meshIndex < lod.Meshes.Count(); meshIndex++)
				{
					Mesh& mesh = lod.Meshes[meshIndex];
					triangleCount += mesh.GetTriangleCount();
					vertexCount += mesh.GetVertexCount();
				}
			}
			sb.Append("LODs: ").Append(lods.Count()).AppendLine();
			sb.Append("Triangles: ").Append(triangleCount).AppendLine();
			sb.Append("Vertices: ").Append(vertexCount).AppendLine();
		}
	}
} // SE