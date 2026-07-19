#pragma once

#include "ModelInstance.h"
#include "Runtime/Render/RenderDrawCall.h"
#include "Runtime/Render/Assets/Geometry/Model.h"

namespace SE
{
	class GPUBuffer;

	SE_CLASS(Reflect)
	class SE_API_RUNTIME StaticModel : public ModelInstance
	{
		SE_DEFINE_CLASS(StaticModel, ModelInstance);

	private:
		GeometryDrawStateData _drawState;
		float _scaleInLightmap;
		float _boundsScale;
		char _lodBias;
		char _forcedLod;
		bool _vertexColorsDirty;
		byte _vertexColorsCount;
		int16 _sortOrder;
		List<Color32> _vertexColorsData[MODEL_MAX_LODS];
		GPUBuffer* _vertexColorsBuffer[MODEL_MAX_LODS];
		Model* _residencyChangedModel = nullptr;
		mutable MeshDeformation* _deformation = nullptr;
	public:

		StaticModel();

		/// <summary>
		/// Finalizes an instance of the <see cref="StaticModel"/> class.
		/// </summary>
		~StaticModel() override;

		/// <summary>
		/// The model asset to draw.
		/// </summary>
		AssetRef<Model> Model;

		/// <summary>
		/// The draw passes to use for rendering this object.
		/// </summary>
		EnumFlags<DrawPass> DrawModes = DrawPass::Default;

	public:
		/// <summary>
		/// Gets the model Level Of Detail bias value. Allows to increase or decrease rendered model quality.
		/// </summary>
		int32 GetLODBias() const;

		/// <summary>
		/// Sets the model Level Of Detail bias value. Allows to increase or decrease rendered model quality.
		/// </summary>
		void SetLODBias(int32 value);

		/// <summary>
		/// Gets the material used to render mesh at given index (overriden by model instance buffer or model default).
		/// </summary>
		/// <param name="meshIndex">The zero-based mesh index.</param>
		/// <param name="lodIndex">The LOD index.</param>
		/// <returns>Material or null if not assigned.</returns>
		MaterialBase* GetMaterial(int32 meshIndex, int32 lodIndex) const;


		void Serialize(SerializeContext& context) override;
		void Deserialize(DeserializeContext& context) override;

		// [ModelInstanceActor]
		bool HasContentLoaded() const;
		const Span<MaterialSlot> GetMaterialSlots() const override;
		MaterialBase* GetMaterial(int32 entryIndex) override;
		void UpdateBounds() override;
		void RenderDraw(RenderContext& renderContext) override;
		void RenderDraw(RenderContextBatch& contextBatch) override;

	protected:
		// [ModelInstanceActor]
		void OnEnable() override;
		void OnDisable() override;
		void WaitForModelLoad() override;

	private:
		void OnModelChanged();
		void OnModelLoaded();
		void OnModelResidencyChanged();
	};

} // SE

