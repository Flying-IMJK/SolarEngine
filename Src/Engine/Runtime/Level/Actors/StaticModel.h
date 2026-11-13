#pragma once

#include "ModelInstance.h"
#include "Runtime/Render/RenderDrawCall.h"
#include "Runtime/Render/Assets/Geometry/Model.h"

namespace SE
{
	class GPUBuffer;

	SE_CLASS(Reflect, API())
	class SE_API_RUNTIME StaticModel : public ModelInstance
	{
		SCRIPTING_TYPE(StaticModel);
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

		/// <summary>
		/// Finalizes an instance of the <see cref="StaticModel"/> class.
		/// </summary>
		~StaticModel() override;

		/// <summary>
		/// The model asset to draw.
		/// </summary>
		SE_FIELD(API(Attributes="EditorOrder(20), DefaultValue(null), EditorDisplay(\"Model\")"))
		AssetRef<Model> Model;

		/// <summary>
		/// The draw passes to use for rendering this object.
		/// </summary>
		SE_FIELD(API(Attributes="EditorOrder(15), DefaultValue(DrawPass.Default), EditorDisplay(\"Model\")"))
		DrawPass DrawModes = DrawPass::Default;

	public:
		/// <summary>
		/// Gets the model Level Of Detail bias value. Allows to increase or decrease rendered model quality.
		/// </summary>
		SE_FUNCTION(API(Prop))
		int32 GetLODBias() const;

        /// <summary>
        /// Gets the requested LOD, or -1 for automatic selection. The rendered LOD is clamped to residency.
        /// </summary>
        SE_FUNCTION(API(Prop))
        int32 GetForcedLOD() const;

        /// <summary>
        /// Sets the requested LOD (-1 for automatic, otherwise 0 through MODEL_MAX_LODS - 1).
        /// Call on the actor update thread. Values outside this range are programming errors.
        /// </summary>
        SE_FUNCTION(API(Prop))
        void SetForcedLOD(int32 value);

        /// <summary>
        /// Computes the target resident LOD using the renderer selection and bias rules for this context.
        /// Returns -1 when no loaded resident model is available or the model is culled. During transitions two LODs may be drawn.
        /// Call on the rendering thread; screenSize receives the diameter relative to the LOD view. Does not wait for streaming.
        /// </summary>
        SE_FUNCTION(API())
        int32 ComputeLODIndex(const RenderContext& renderContext, float& screenSize) const;

		/// <summary>
		/// Sets the model Level Of Detail bias value. Allows to increase or decrease rendered model quality.
		/// </summary>
		SE_FUNCTION(API(Prop))
		void SetLODBias(int32 value);

		/// <summary>
		/// Gets the material used to render mesh at given index (overriden by model instance buffer or model default).
		/// </summary>
		/// <param name="meshIndex">The zero-based mesh index.</param>
		/// <param name="lodIndex">The LOD index.</param>
		/// <returns>Material or null if not assigned.</returns>
		SE_FUNCTION(API())
		MaterialBase* GetMaterial(int32 meshIndex, int32 lodIndex) const;


		void Serialize(SerializeContext& context) override;
		void Deserialize(DeserializeContext& context) override;

		// [ModelInstanceActor]
        /// <summary>
        /// Queries model and instance-material loading without waiting. This does not guarantee LOD or texture residency.
        /// </summary>
        SE_FUNCTION(API())
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
