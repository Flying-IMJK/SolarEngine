
#include "MaterialBase.h"

#include "Material.h"
#include "Runtime/Utilities/Variant.h"
#include "Runtime/Resource/AssetContent.h"

namespace SE
{

	MaterialBase::MaterialBase(const AssetInfo* info) : BinaryAsset(info)
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
			param->SetValue(value);
			param->SetIsOverride(true);
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