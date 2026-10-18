
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

	bool MaterialBase::IsSLC2Ready() const
	{
		const MaterialShader* shader = GetMaterialShaderObject();
		return shader != nullptr && shader->IsSLC2Ready();
	}

	uint64 MaterialBase::GetRuntimeGenerationID() const
	{
		const MaterialShader* shader = GetMaterialShaderObject();
		return shader != nullptr ? shader->GetRuntimeGenerationID() : 0;
	}

	bool MaterialBase::PrepareSLC2Binding(const MaterialDrawContext& context, MaterialSLC2Binding& output)
	{
		MaterialShader* shader = GetMaterialShaderObject();
		output.Clear();
		if (shader == nullptr)
		{
			return false;
		}

		// Preserve the legacy MaterialParams override order: the leaf instance is
		// inspected first and a non-overriding value falls through toward the root.
		List<const MaterialParams*> parameterChain;
		const MaterialBase* material = this;
		while (material != nullptr)
		{
			parameterChain.Add(&material->Params);
			material = material->IsMaterialInstance()
				? static_cast<const MaterialInstance*>(material)->GetBaseMaterial()
				: nullptr;
		}
		return shader->PrepareSLC2Binding(context, parameterChain, m_SLC2ProgramInstances, output);
	}

	void MaterialBase::ClearSLC2RuntimeCache()
	{
		m_SLC2ProgramInstances.Clear();
	}

	/*MaterialInstance* MaterialBase::CreateVirtualInstance()
	{
		auto instance = AssetContent::CreateVirtualAsset<MaterialInstance>();
		instance->SetBaseMaterial(this);
		return instance;
	}*/

} // SE
