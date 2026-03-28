
#include "TextureOperate.h"

#include "Editor/GUI/Viewport/Previews/TexturePreview.h"
#include "Editor/Resource/Thumbnails/ThumbnailRequest.h"
#include "Runtime/Render/Assets/Texture/Texture.h"

namespace SE::Editor
{
	CONTENT_OPERATE_REGISTER(TextureOperate);

	EditorWindow* TextureOperate::Open(const EditorApp* editor, ContentItem* item)
	{
		return nullptr; //new TextureWindow(editor, (AssetItem*)item);
	}
	TypeID TextureOperate::GetAssetType()
	{
		return Typeof<Texture>();
	}

	Color TextureOperate::__GetAccentColor()
	{
		return Color::FromRGB(0x25B84C);
	}

	String TextureOperate::__GetName()
	{
		return SE_TEXT("Texture");
	}

	void TextureOperate::OnThumbnailDrawPrepare(ThumbnailRequest* request)
	{
		if (_preview == nullptr)
		{
			_preview = New<SimpleTexturePreview>();
			_preview->AnchorPreset = AnchorPresets::StretchAll;
			_preview->Offsets = Margin::Zero;
		}

		// TODO: disable streaming for asset during thumbnail rendering (and restore it after)
	}

	bool TextureOperate::CanDrawThumbnail(ThumbnailRequest* request)
	{
		return true;//ThumbnailsModule::HasMinimumQuality((Texture)request.Asset);
	}

	void TextureOperate::OnThumbnailDrawBegin(ThumbnailRequest* request, ContainerControl* guiRoot, GPUContext* context)
	{
		_preview->Asset = static_cast<Texture*>(request->asset);
		_preview->Parent = guiRoot;
	}

	void TextureOperate::OnThumbnailDrawEnd(ThumbnailRequest* request, ContainerControl* guiRoot)
	{
		_preview->Asset = nullptr;
		_preview->Parent = nullptr;
	}

	void TextureOperate::Dispose()
	{
		if (_preview != nullptr)
		{
			_preview->Dispose();
			_preview = nullptr;
		}

		BinaryAssetOperate::Dispose();
	}
} // SE