
#include "MaterialBase.h"

#include "Material.h"
#include "MaterialInstance.h"
#include "Runtime/Render/Assets/Material/MaterialShader.h"
#include "Runtime/Utilities/Variant.h"
#include "Runtime/Resource/AssetContent.h"

namespace SE
{

	MaterialBase::MaterialBase(const SpawnParams& params, const AssetInfo* info) : BinaryAsset(params, info)
	{
	}

	Variant MaterialBase::GetParameterValue(const StringView& name)
	{
		const auto param = Params.Get(name);
		if (param)
		{
			return param->GetValue();
		}
		LOG_WARNING("Render", "Missing material parameter '{0}' in material {1}", String(name), ToString());
		return Variant::Null;
	}

	void MaterialBase::SetParameterValue(const StringView& name, const Variant& value, bool warnIfMissing)
	{
		const auto param = Params.Get(name);
		if (param)
		{
			if (param->TrySetValue(value) == MaterialParameterSetResult::Success)
			{
				param->SetIsOverride(true);
			}
			else
			{
				LOG_WARNING("Render", "Invalid value for material parameter '{0}' in material {1}", String(name), ToString());
			}
		}
		else if (warnIfMissing)
		{
			LOG_WARNING("Render", "Missing material parameter '{0}' in material {1}", String(name), ToString());
		}
	}

	uint32 MaterialBase::GetSerializedVersion() const
	{
		return MATERIAL_VERSION;
	}


	/*MaterialInstance* MaterialBase::CreateVirtualInstance()
	{
		auto instance = AssetContent::CreateVirtualAsset<MaterialInstance>();
		instance->SetBaseMaterial(this);
		return instance;
	}*/

} // SE
