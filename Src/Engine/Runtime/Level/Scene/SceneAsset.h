#pragma once
#include "Runtime/Resource/JsonAsset.h"

namespace SE
{
	/// <summary>
	/// The scene asset.
	/// </summary>
	class SE_API_RUNTIME SceneAsset : public JsonAsset
	{
		SE_CLASS_DEFAULT(SceneAsset, JsonAsset);
	public:
		explicit SceneAsset(const AssetInfo* info);

	protected:
		bool IsInternalType() const override;
	};
}
