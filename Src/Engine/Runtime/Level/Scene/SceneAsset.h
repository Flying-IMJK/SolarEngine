#pragma once
#include "Runtime/Resource/JsonAsset.h"

namespace SE
{
	/// <summary>
	/// The scene asset.
	/// </summary>
	SE_CLASS(Reflect, API, NoSpawn)
	class SE_API_RUNTIME SceneAsset : public JsonAsset
	{
		SE_DEFINE_CLASS(SceneAsset, JsonAsset);
		ASSET_HEADER(SceneAsset);

	protected:
		bool IsInternalType() const override;
	};
}
