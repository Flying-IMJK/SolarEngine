
#include "ModelOperate.h"

#include "Editor/EditorApp.h"
#include "Editor/Resource/Items/BinaryAssetItem.h"
#include "Runtime/Render/Assets/Geometry/Model.h"

namespace SE::Editor
{
	CONTENT_OPERATE_REGISTER(ModelOperate);

	EditorWindow* ModelOperate::Open(const EditorApp* editor, ContentItem* item)
	{
		return nullptr; //new ModelWindow(editor, item as AssetItem);
	}

	void ModelOperate::OnContentWindowContextMenu(ContextMenu* menu, ContentItem* item)
	{
		BinaryAssetOperate::OnContentWindowContextMenu(menu, item);

		/*menu->AddButton("Create collision data", [&item]()
		{
			var collisionDataProxy = (CollisionDataProxy)Editor.Instance.ContentDatabase.GetProxy<CollisionData>();
			var selection = EditorApp::GetInstance().windowsModule.ContentWin.View.Selection;
			if (selection.Count > 1)
			{
				// Batch action
				var items = selection.ToArray(); // Clone to prevent issue when iterating over and content window changes the selection
				foreach (var contentItem in items)
				{
					if (contentItem is ModelItem modelItem)
						collisionDataProxy.CreateCollisionDataFromModel(AssetContent::LoadAsync<Model>(modelItem.ID), nullptr, false);
				}
			}
			else
			{
				Model* model = AssetContent::LoadAsync<Model>(((ModelItem*)item)->ID);
				collisionDataProxy.CreateCollisionDataFromModel(model);
			}
		});*/
	}

	void ModelOperate::OnThumbnailDrawPrepare(ThumbnailRequest* request)
	{
		/*if (_preview == null)
		{
			_preview = new ModelPreview(false)
			{
				ScaleToFit = false,
			};
			InitAssetPreview(_preview);
		}*/

		// TODO: disable streaming for asset during thumbnail rendering (and restore it after)
	}

	bool ModelOperate::CanDrawThumbnail(ThumbnailRequest* request)
	{
		return true; //_preview.HasLoadedAssets && ThumbnailsModule.HasMinimumQuality((Model)request.Asset);
	}

	void ModelOperate::OnThumbnailDrawBegin(ThumbnailRequest* request, ContainerControl* guiRoot, GPUContext* context)
	{
		/*_preview.Model = (Model)request.Asset;
		_preview.Parent = guiRoot;
		_preview.SyncBackbufferSize();
		_preview.ViewportCamera.SetArcBallView(_preview.Model.GetBox());

		_preview.Task.OnDraw();*/
	}

	void ModelOperate::OnThumbnailDrawEnd(ThumbnailRequest* request, ContainerControl* guiRoot)
	{
		/*_preview.Model = null;
		_preview.Parent = null;*/
	}

	void ModelOperate::Dispose()
	{
		/*
		if (_preview != null)
		{
			_preview.Dispose();
			_preview = null;
		}
		*/

		BinaryAssetOperate::Dispose();
	}

	TypeID ModelOperate::GetAssetType()
	{
		return Typeof<Model>();
	}

	String ModelOperate::__GetName()
	{
		return SE_TEXT("Model");
	}

	Color ModelOperate::__GetAccentColor()
	{
		return Color::FromRGB(0xe67e22);
	}
} // SE