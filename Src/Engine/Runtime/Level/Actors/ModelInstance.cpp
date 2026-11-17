
#include "ModelInstance.h"

#include "../Scene/Scene.h"

namespace SE
{
	ModelInstance::ModelInstance(const SpawnParams& params)
		: RenderActor(params)
	{
	}

	void ModelInstance::OnEnable()
	{
		GetScene()->Rendering.AddRender(this, m_SceneRenderingKey);

		// Base
		RenderActor::OnEnable();
	}

	void ModelInstance::OnDisable()
	{
		// Base
		RenderActor::OnDisable();

		GetScene()->Rendering.RemoveRender(this, m_SceneRenderingKey);
	}

	void ModelInstance::WaitForModelLoad()
	{

	}

	void ModelInstance::SetEntries(const List<ModelInstanceEntry>& value)
	{
		WaitForModelLoad();
		bool anyChanged = false;
		Entries.Resize(value.Count());
		for (int32 i = 0; i < value.Count(); i++)
		{
			anyChanged |= Entries[i] != value[i];
			Entries[i] = value[i];
		}

		if (anyChanged && m_SceneRenderingKey != -1)
		{
			GetScene()->Rendering.UpdateRender(this, m_SceneRenderingKey);
		}
	}


    void ModelInstance::SetMaterial(int32 entryIndex, MaterialBase* material)
	{
		WaitForModelLoad();
		if (Entries.Count() == 0 && !material)
		{
		    return;
		}

		if (entryIndex < 0 || entryIndex >= Entries.Count())
		{
			return;
		}

		if (Entries[entryIndex].Material == material)
		{
		    return;
		}

		Entries[entryIndex].Material = material;
		if (m_SceneRenderingKey != -1)
		{
			GetScene()->Rendering.UpdateRender(this, m_SceneRenderingKey);
		}
	}

    void ModelInstance::OnLayerChanged()
    {
		if (m_SceneRenderingKey != -1)
		{
			GetScene()->Rendering.UpdateRender(this, m_SceneRenderingKey);
		}
	}

	void ModelInstance::OnTransformChanged()
	{
		// Base
		RenderActor::OnTransformChanged();

		UpdateBounds();
	}

} // SE
