
#include "ThumbnailRequest.h"

#include "Editor/Resource/Items/AssetItem.h"
#include "Editor/Resource/Opreate/AssetOperate.h"
#include "Runtime/Resource/Asset.h"
#include "Runtime/Resource/AssetContent.h"

namespace SE::Editor
{
	ThumbnailRequest::ThumbnailRequest(AssetItem* item, AssetOperate* proxy)
	{
		Item = item;
		operate = proxy;
	}
	void ThumbnailRequest::Update()
	{
		if (state == States::Prepared && (!asset || asset->LastLoadFailed()))
		{
			state = States::Failed;
		}
	}

	void ThumbnailRequest::Prepare()
	{
		ENGINE_ASSERT(state == States::Created);

		asset = AssetContent::LoadAsync<Asset>(Item->Path);
		operate->OnThumbnailDrawPrepare(this);
		state = States::Prepared;
	}

	void ThumbnailRequest::FinishRender(SpriteHandle icon)
	{
		ENGINE_ASSERT(state == States::Prepared);

		Item->Thumbnail = icon;
		state = States::Rendered;
	}

	void ThumbnailRequest::Dispose()
	{
		ENGINE_ASSERT(state != States::Disposed);

		if (state != States::Created)
		{
			// Cleanup
			operate->OnThumbnailDrawCleanup(this);
			asset = nullptr;
		}

		// Tag = null;
		state = States::Disposed;
	}

	bool ThumbnailRequest::__GetIsReady()
	{
		return state == States::Prepared && asset && asset->IsLoaded() && operate->CanDrawThumbnail(this);
	}

} // SE