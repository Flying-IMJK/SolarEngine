
#include "SceneOperate.h"

#include "Editor/EditorApp.h"
#include "Editor/GUI/ContextMenu/ContextMenu.h"
#include "Editor/Modules/SceneModule.h"
#include "Editor/Resource/Items/Contentfolder.h"
#include "Editor/Resource/Items/SceneItem.h"
#include "Runtime/Level/Level.h"
#include "../../../Runtime/Level/Scene/Scene.h"
#include "Runtime/Level/Scene/SceneAsset.h"

namespace SE::Editor
{
	CONTENT_OPERATE_REGISTER(SceneOperate);

	String SceneOperate::Extension = SE_TEXT("scene");

	bool SceneOperate::IsProxyFor(ContentItem* item)
	{
		return TypeIs<SceneItem>(item);
	}

	bool SceneOperate::IsProxyFor(TypeID typeID)
	{
		return typeID == Typeof<SceneAsset>();
	}

	bool SceneOperate::AcceptsAsset(TypeID typeID, StringView path)
	{
		return typeID == Typeof<SceneAsset>() && path.EndsWith(Extension);
	}

	bool SceneOperate::CanCreate(ContentFolder* targetLocation)
	{
		return targetLocation->CanHaveAssets;
	}

	void SceneOperate::Create(String outputPath, void* arg)
	{
		EditorApp::Ins().sceneModule->CreateSceneFile(outputPath);
	}

	EditorWindow* SceneOperate::Open(const EditorApp* editor, ContentItem* item)
	{
		// Load scene
		EditorApp::Ins().sceneModule->OpenScene(((SceneItem*)item)->id);

		return nullptr;
	}

	AssetItem* SceneOperate::ConstructItem(StringView path, TypeID typeID, UID id)
	{
		return New<SceneItem>(path, id);
	}

	void SceneOperate::OnContentWindowContextMenu(ContextMenu* menu, ContentItem* item)
	{
		UID id = ((SceneItem*)item)->id;
		if (Level::FindScene(id) == nullptr)
		{
			menu->AddButton(SE_TEXT("Open (additive)"), CreateFunc([id]()
			{
				EditorApp::Ins().sceneModule->OpenScene(id, true);
			}));
		}
	}

	TypeID SceneOperate::GetAssetType()
	{
		return Typeof<Scene>();
	}

	String SceneOperate::__GetFileExtension()
	{
		return Extension;
	}

	String SceneOperate::__GetName()
	{
		return SE_TEXT("Scene");
	}

	Color SceneOperate::__GetAccentColor()
	{
		return Color::FromRGB(0xbb37ef);
	}
} // SE