#pragma once
#include "AssetOperate.h"

namespace SE::Editor
{
	class SceneOperate : public AssetOperate
	{
		SE_CLASS_DEFAULT(SceneOperate, AssetOperate)

	public:
		/// <summary>
		/// The scene files extension.
		/// </summary>
		static String Extension;

		/// <inheritdoc />
		bool IsProxyFor(ContentItem* item) override;

		bool IsProxyFor(TypeID typeID) override;

		/// <inheritdoc />
		bool AcceptsAsset(TypeID typeID, StringView path) override;

		bool CanCreate(ContentFolder* targetLocation) override;

		void Create(String outputPath, void* arg) override;

		EditorWindow* Open(const EditorApp* editor, ContentItem* item) override;

		AssetItem* ConstructItem(StringView path, TypeID typeID, UID id) override;

		void OnContentWindowContextMenu(ContextMenu* menu, ContentItem* item) override;

	protected:

		TypeID GetAssetType() override;

		String __GetFileExtension() override;

		String __GetName() override;

		Color __GetAccentColor() override;
	};
} // SE

