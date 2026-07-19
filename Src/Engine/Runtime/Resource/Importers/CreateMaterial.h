#pragma once

#include "Types.h"
#include "Runtime/Core/Math/Color.h"
#include "Runtime/Core/Types/UID.h"
#include "Runtime/Render/Assets/Material/MaterialInfo.h"

namespace SE
{

	/// <summary>
	/// Creating materials utility
	/// </summary>
	class CreateMaterial
	{
	public:
		struct Options
		{
			MaterialInfo Info;

			struct
			{
				Color Color = Colors::White;
				UID Texture = UID::Empty;
				bool HasAlphaMask = false;
			} Diffuse;

			struct
			{
				Color Color = Colors::Transparent;
				UID Texture = UID::Empty;
			} Emissive;

			struct
			{
				float Value = 1.0f;
				UID Texture = UID::Empty;
			} Opacity;

			struct
			{
				float Value = 0.5f;
				UID Texture = UID::Empty;
			} Roughness;

			struct
			{
				UID Texture = UID::Empty;
			} Normals;

			Options();
		};

		/// <summary>
		/// Creates the material asset.
		/// </summary>
		/// <param name="context">The importing context.</param>
		/// <returns>Result.</returns>
		static CreateAssetResult Create(CreateAssetContext& context);
	};

} // SE

