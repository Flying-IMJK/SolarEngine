
#include "StaticModel.h"

#include "Runtime/Core/Serialization/Serialization.h"
#include "Runtime/Engine.h"
#include "../Scene/Scene.h"
#include "Runtime/Render/RenderDrawCall.h"
#include "Runtime/Render/Assets/Geometry/MeshDeformation.h"
#include "Runtime/Resource/AssetRefSerialize.h"

namespace SE
{
	StaticModel::StaticModel()
		: _scaleInLightmap(1.0f)
		, _boundsScale(1.0f)
		, _lodBias(0)
		, _forcedLod(-1)
		, _vertexColorsDirty(false)
		, _vertexColorsCount(0)
		, _sortOrder(0)
	{
		Model.Changed.Bind<StaticModel, &StaticModel::OnModelChanged>(this);
		Model.Loaded.Bind<StaticModel, &StaticModel::OnModelLoaded>(this);
	}

	StaticModel::~StaticModel()
	{
		for (int32 lodIndex = 0; lodIndex < _vertexColorsCount; lodIndex++)
		{
			DeleteObjectSafe(_vertexColorsBuffer[lodIndex]);
		}

		if (_deformation)
		{
			Delete(_deformation);
		}
	}

	int32 StaticModel::GetLODBias() const
	{
		return _lodBias;
	}

	void StaticModel::SetLODBias(int32 value)
	{
		_lodBias = static_cast<char>(Math::Clamp(value, -100, 100));
	}

	MaterialBase* StaticModel::GetMaterial(int32 meshIndex, int32 lodIndex) const
	{
		auto model = Model.Get();
		ENGINE_ASSERT(model &&
			Math::RangeInclusive(lodIndex, 0, model->GetLODsCount()) &&
			Math::RangeInclusive(meshIndex, 0, model->LODs[lodIndex].Meshes.Count()));

		const auto& mesh = model->LODs[lodIndex].Meshes[meshIndex];
		const auto materialSlotIndex = mesh.GetMaterialSlotIndex();
		MaterialBase* material = Entries[materialSlotIndex].Material.Get();
		return material ? material : model->MaterialSlots[materialSlotIndex].Material.Get();
	}

	void StaticModel::Serialize(SerializeContext& context)
	{
		// Base
		ModelInstance::Serialize(context);

		SERIALIZE_GET_OTHER_OBJ(StaticModel, context.otherObj);

		SERIALIZE_MEMBER(ScaleInLightmap, _scaleInLightmap);
		SERIALIZE_MEMBER(BoundsScale, _boundsScale);
		SERIALIZE(Model);
		SERIALIZE_MEMBER(LODBias, _lodBias);
		SERIALIZE_MEMBER(ForcedLOD, _forcedLod);
		SERIALIZE_MEMBER(SortOrder, _sortOrder);
		SERIALIZE(DrawModes);

		/*if (HasLightmap()
	#if USE_EDITOR
			&& !PrefabManager::IsCreatingPrefab
	#endif
		)
		{
			stream.JKEY("LightmapIndex");
			stream.Int(Lightmap.TextureIndex);

			stream.JKEY("LightmapArea");
			stream.Rectangle(Lightmap.UVsArea);
		}*/

		context.stream.JKEY("Buffer");
		context.stream.Object(&Entries, other ? &other->Entries : nullptr);

		if (_vertexColorsCount)
		{
			context.stream.JKEY("VertexColors");
			context.stream.StartArray();
			List<char> encodedData;
			for (int32 lodIndex = 0; lodIndex < _vertexColorsCount; lodIndex++)
			{
				auto& vertexColorsData = _vertexColorsData[lodIndex];
				if (vertexColorsData.HasItems())
				{
					const int32 size = vertexColorsData.Count() * sizeof(Color32);
					Encoding::Base64::Encode((byte*)vertexColorsData.Get(), size, encodedData);
					context.stream.String(encodedData.Get(), encodedData.Count());
				}
				else
				{
					context.stream.String("", 0);
				}
			}
			context.stream.EndArray();
		}
	}

	void StaticModel::Deserialize(DeserializeContext& context)
	{
		// Base
		ModelInstance::Deserialize(context);

		DESERIALIZE_MEMBER(ScaleInLightmap, _scaleInLightmap);
		DESERIALIZE_MEMBER(BoundsScale, _boundsScale);
		DESERIALIZE(Model);
		DESERIALIZE_MEMBER(LODBias, _lodBias);
		DESERIALIZE_MEMBER(ForcedLOD, _forcedLod);
		DESERIALIZE_MEMBER(SortOrder, _sortOrder);
		// DESERIALIZE(DrawModes);
		// DESERIALIZE_MEMBER(LightmapIndex, Lightmap.TextureIndex);
		// DESERIALIZE_MEMBER(LightmapArea, Lightmap.UVsArea);

		Entries.DeserializeIfExists(context, "Buffer");

		{
			const auto member = context.stream->FindMember("VertexColors");
			if (member != context.stream->MemberEnd() && member->value.IsArray())
			{
				// TODO: don't stall but just check the length of the loaded vertex colors arrays size later when asset gets loaded
				if (Model && !Model->WaitForLoaded())
				{
					// RemoveVertexColors();
					auto& array = member->value;
					_vertexColorsCount = array.Size();
					List<byte> decodedData;
					if (_vertexColorsCount == Model->GetLODsCount())
					{
						for (int32 lodIndex = 0; lodIndex < _vertexColorsCount; lodIndex++)
						{
							_vertexColorsBuffer[lodIndex] = nullptr;
							auto& vertexColorsData = _vertexColorsData[lodIndex];
							vertexColorsData.Clear();
							auto& v = array[lodIndex];
							if (v.IsString())
							{
								Encoding::Base64::Decode(v.GetString(), v.GetStringLength(), decodedData);
								const int32 length = decodedData.Count() / sizeof(Color32);
								vertexColorsData.Resize(length);
								Platform::MemoryCopy(vertexColorsData.Get(), decodedData.Get(), decodedData.Count());
							}
						}
					}
					else
					{
						LOG_ERROR("Level", "Loaded vertex colors data for {0} has different size than the model {1} LODs count.", ToString(), Model->ToString());
					}
					_vertexColorsDirty = true;
				}
			}
		}

		/*// [Deprecated on 11.10.2019, expires on 11.10.2020]
		if (modifier->EngineBuild <= 6187)
		{
			const auto member = stream.FindMember("HiddenShadow");
			if (member != stream.MemberEnd() && member->value.IsBool() && member->value.GetBool())
			{
				DrawModes = DrawPass::Depth;
			}
		}
		// [Deprecated on 07.02.2022, expires on 07.02.2024]
		if (modifier->EngineBuild <= 6330)
		{
			DrawModes |= DrawPass::GlobalSDF;
		}
		// [Deprecated on 27.04.2022, expires on 27.04.2024]
		if (modifier->EngineBuild <= 6331)
		{
			DrawModes |= DrawPass::GlobalSurfaceAtlas;
		}*/

		{
			const auto member = context.stream->FindMember("RenderPasses");
			if (member != context.stream->MemberEnd() && member->value.IsInt())
			{
				DrawModes = (DrawPass)member->value.GetInt();
			}
		}
	}

	bool StaticModel::HasContentLoaded() const
	{
		return (Model == nullptr || Model->IsLoaded()) && Entries.HasContentLoaded();
	}

	const Span<MaterialSlot> StaticModel::GetMaterialSlots() const
	{
		const auto model = Model.Get();
		if (model && !model->WaitForLoaded())
		{
			return ToSpan(model->MaterialSlots);
		}
		return Span<MaterialSlot>();
	}

	MaterialBase* StaticModel::GetMaterial(int32 entryIndex)
	{
		if (!Model || Model->WaitForLoaded())
		{
			return nullptr;
		}

		if (entryIndex < 0 || entryIndex >= Entries.Count())
		{
			return nullptr;
		}

		MaterialBase* material = Entries[entryIndex].Material.Get();
		if (!material && entryIndex < Model->MaterialSlots.Count())
		{
			material = Model->MaterialSlots[entryIndex].Material.Get();
			if (!material)
			{
				// material = GPUDevice::Instance->GetDefaultMaterial();
			}
		}
		return material;
	}

	void StaticModel::OnModelChanged()
	{
		if (_residencyChangedModel)
		{
			_residencyChangedModel = nullptr;
			Model->ResidencyChanged.Unbind<StaticModel, &StaticModel::OnModelResidencyChanged>(this);
		}
		// RemoveVertexColors();
		Entries.Release();
		if (Model && !Model->IsLoaded())
			UpdateBounds();
		/*if (_deformation)
			_deformation->Clear();*/
		else if (!Model && _sceneRenderingKey != -1)
		{
			GetScene()->Rendering.RemoveRender(this, _sceneRenderingKey);
		}
	}

	void StaticModel::OnModelLoaded()
	{
		Entries.SetupIfInvalid(Model);
		UpdateBounds();
		if (_sceneRenderingKey == -1 && m_Scene && m_IsActiveInHierarchy && m_IsEnabled && !_residencyChangedModel)
		{
			// Register for rendering but once the model has any LOD loaded
			if (Model->GetLoadedLODs() == 0)
			{
				_residencyChangedModel = Model;
				_residencyChangedModel->ResidencyChanged.Bind<StaticModel, &StaticModel::OnModelResidencyChanged>(this);
			}
			else
			{
				GetScene()->Rendering.AddRender(this, _sceneRenderingKey);
			}
		}
	}

	void StaticModel::OnModelResidencyChanged()
	{
		if (_sceneRenderingKey == -1 && m_Scene && Model && Model->GetLoadedLODs() > 0 && _residencyChangedModel)
		{
			GetScene()->Rendering.AddRender(this, _sceneRenderingKey);
			_residencyChangedModel->ResidencyChanged.Unbind<StaticModel, &StaticModel::OnModelResidencyChanged>(this);
			_residencyChangedModel = nullptr;
		}
	}

	void StaticModel::UpdateBounds()
	{
		const auto model = Model.Get();
		if (model && model->IsLoaded() && model->LODs.Count() != 0)
		{
			Transform transform = m_Transform;
			transform.Scale *= _boundsScale;
			m_Box = model->LODs[0].GetBox(transform, _deformation);
		}
		else
		{
			m_Box = BoundingBox(m_Transform.Translation);
		}
		BoundingSphere::FromBox(m_Box, m_Sphere);
		if (_sceneRenderingKey != -1)
		{
			GetScene()->Rendering.UpdateRender(this, _sceneRenderingKey);
		}
	}

	void StaticModel::RenderDraw(RenderContext& renderContext)
	{
		if (!Model || !Model->IsLoaded() || !Model->CanBeRendered())
			return;

		/*if (renderContext.view.Pass.IsFlagSet(DrawPass::GlobalSDF))
		{
			if (EnumHasAnyFlags(DrawModes, DrawPass::GlobalSDF))
				GlobalSignDistanceFieldPass::Instance()->RasterizeModelSDF(this, Model->SDF, _transform, _box);
			return;
		}*/
		/*if (renderContext.view.Pass.IsFlagSet(DrawPass::GlobalSurfaceAtlas))
		{
			if (DrawModes.IsFlagSet(DrawPass::GlobalSurfaceAtlas))
			{
				GlobalSurfaceAtlasPass::Instance()->RasterizeActor(this, this, _sphere, _transform, Model->LODs.Last().GetBox());
			}
			return;
		}*/
		Matrix world;
		GetLocalToWorldMatrix(world);
		renderContext.view.GetWorldMatrix(world);
		GEOMETRY_DRAW_STATE_EVENT_BEGIN(_drawState, world);
		if (_vertexColorsDirty)
		{
			// FlushVertexColors();
		}

		Mesh::DrawInfo draw;
		draw.Buffer = &Entries;
		draw.World = &world;
		draw.DrawState = &_drawState;
		draw.Deformation = _deformation;
		draw.Lightmap = nullptr; // _scene ? _scene->LightmapsData.GetReadyLightmap(Lightmap.TextureIndex) : nullptr;
		draw.LightmapUVs = nullptr ; //&Lightmap.UVsArea;
		draw.Flags = m_StaticFlags;
		draw.DrawModes = DrawModes;
		draw.Bounds = m_Sphere;
		draw.Bounds.Center -= renderContext.view.Origin;
		draw.PerInstanceRandom = GetPerInstanceRandom();
		draw.LODBias = _lodBias;
		draw.ForcedLOD = _forcedLod;
		draw.SortOrder = _sortOrder;
		draw.VertexColors = _vertexColorsCount ? _vertexColorsBuffer : nullptr;

		Model->Draw(renderContext, draw);

		GEOMETRY_DRAW_STATE_EVENT_END(_drawState, world);
	}

	void StaticModel::RenderDraw(RenderContextBatch& renderContextBatch)
	{
	    if (!Model || !Model->IsLoaded())
	        return;
	    const RenderContext& renderContext = renderContextBatch.GetMainContext();
	    Matrix world;
	    GetLocalToWorldMatrix(world);
	    renderContext.view.GetWorldMatrix(world);
	    GEOMETRY_DRAW_STATE_EVENT_BEGIN(_drawState, world);
	    if (_vertexColorsDirty)
	    {
	    	// FlushVertexColors();
	    }

	    Mesh::DrawInfo draw;
	    draw.Buffer = &Entries;
	    draw.World = &world;
	    draw.DrawState = &_drawState;
	    draw.Deformation = _deformation;
	    draw.Lightmap = nullptr; // _scene ? _scene->LightmapsData.GetReadyLightmap(Lightmap.TextureIndex) : nullptr;
	    draw.LightmapUVs = nullptr; //&Lightmap.UVsArea;
	    draw.Flags = m_StaticFlags;
	    draw.DrawModes = DrawModes;
	    draw.Bounds = m_Sphere;
	    draw.Bounds.Center -= renderContext.view.Origin;
	    draw.PerInstanceRandom = GetPerInstanceRandom();
	    draw.LODBias = _lodBias;
	    draw.ForcedLOD = _forcedLod;
	    draw.SortOrder = _sortOrder;
	    draw.VertexColors = _vertexColorsCount ? _vertexColorsBuffer : nullptr;

	    Model->Draw(renderContextBatch, draw);

	    GEOMETRY_DRAW_STATE_EVENT_END(_drawState, world);
	}

	void StaticModel::OnEnable()
	{
		// If model is set and loaded but we still don't have residency registered do it here (eg. model is streaming LODs right now)
		if (m_Scene && _sceneRenderingKey == -1 && !_residencyChangedModel && Model && Model->IsLoaded())
		{
			// Register for rendering but once the model has any LOD loaded
			if (Model->GetLoadedLODs() == 0)
			{
				_residencyChangedModel = Model;
				_residencyChangedModel->ResidencyChanged.Bind<StaticModel, &StaticModel::OnModelResidencyChanged>(this);
			}
			else
			{
				GetScene()->Rendering.AddRender(this, _sceneRenderingKey);
			}
		}

		// Skip ModelInstanceActor (add to SceneRendering manually)
		RenderActor::OnEnable();
	}

	void StaticModel::OnDisable()
	{
		// Skip ModelInstanceActor (add to SceneRendering manually)
		RenderActor::OnDisable();

		if (_sceneRenderingKey != -1)
		{
			GetScene()->Rendering.RemoveRender(this, _sceneRenderingKey);
		}
		if (_residencyChangedModel)
		{
			_residencyChangedModel->ResidencyChanged.Unbind<StaticModel, &StaticModel::OnModelResidencyChanged>(this);
			_residencyChangedModel = nullptr;
		}
	}

	void StaticModel::WaitForModelLoad()
	{
		if (Model)
		{
			Model->WaitForLoaded();
		}
	}


} // SE