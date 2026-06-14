#pragma once
#include "Runtime/Level/Actor.h"
#include "Runtime/Render/SceneRendering.h"
#include "Runtime/Render/Assets/Geometry/ModelInstanceEntry.h"


namespace SE
{
	class IRender;

	class SE_API_RUNTIME ModelInstance : public RenderActor
	{
		SE_DEFINE_CLASS(ModelInstance, RenderActor);

	public:
		/// <summary>
		/// The model instance buffer.
		/// </summary>
		ModelInstanceEntries Entries;

		FORCE_INLINE const List<ModelInstanceEntry>& GetEntries() const
		{
			return Entries;
		}

		/// <summary>
		/// Sets the model entries collection. Each entry contains data how to render meshes using this entry (transformation, material, shadows casting, etc.).
		/// </summary>
		void SetEntries(const List<ModelInstanceEntry>& value);

		/// <summary>
		/// Gets the material slots array set on the asset (eg. model or skinned model asset).
		/// </summary>
		virtual const Span<class MaterialSlot> GetMaterialSlots() const = 0;

		/// <summary>
		/// Gets the material used to draw the meshes which are assigned to that slot (set in Entries or model's default).
		/// </summary>
		/// <param name="entryIndex">The material slot entry index.</param>
		virtual MaterialBase* GetMaterial(int32 entryIndex) = 0;

		/// <summary>
		/// Sets the material to the entry slot. Can be used to override the material of the meshes using this slot.
		/// </summary>
		/// <param name="entryIndex">The material slot entry index.</param>
		/// <param name="material">The material to set.</param>
		void SetMaterial(int32 entryIndex, MaterialBase* material);

		/// <summary>
		/// Updates the model bounds (eg. when mesh has applied significant deformation).
		/// </summary>
		virtual void UpdateBounds() = 0;

		ModelInstance();

		// [Actor]
		void OnLayerChanged() override;
		void OnTransformChanged() override;

	public:
		// [RenderActor]
		void RenderDraw(RenderContext& context) override = 0;
		void RenderDraw(RenderContextBatch& contextBatch) override = 0;

	protected:
		int32 _sceneRenderingKey = -1; // Uses SceneRendering::DrawCategory::SceneDrawAsync

		// [Actor]
		void OnEnable() override;
		void OnDisable() override;

		virtual void WaitForModelLoad();
	};

} // SE

