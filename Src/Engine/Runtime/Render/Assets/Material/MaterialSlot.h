#pragma once

#include "Core/Types/Object.h"
#include "Runtime/Render/RenderEnum.h"
#include "Runtime/Resource/AssetRef.h"
#include "../../../Resource/Assets/Materials/MaterialBase.h"

namespace SE
{
	/// <summary>
	/// The material slot descriptor that specifies how to render geometry using it.
	/// </summary>
	class SE_API_RUNTIME MaterialSlot : public Object
	{
	public:
		/// <summary>
		/// The material to use for rendering.
		/// </summary>
		AssetRef<MaterialBase> Material;

		/// <summary>
		/// The shadows casting mode by this visual element.
		/// </summary>
		EnumFlags<ShadowsCastingMode> ShadowsMode = ShadowsCastingMode::All;

		/// <summary>
		/// The slot name.
		/// </summary>
		String Name;

	public:
		MaterialSlot(const MaterialSlot& other) : MaterialSlot()
		{
#if !BUILD_RELEASE
			ENGINE_UNIMPLEMENTED_FUNCTION(); // Not used
#endif
		}

		MaterialSlot() {};
	};
}
