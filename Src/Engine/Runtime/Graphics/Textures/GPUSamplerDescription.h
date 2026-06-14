#pragma once

#include "Core/TypeSystem/IType.h"
#include "Runtime/API.h"

namespace SE
{
	/// <summary>
	/// GPU sampler filter modes.
	/// </summary>
	SE_ENUM(Reflect)
	enum class GPUSamplerFilter
	{
		/// <summary>Filter using the nearest found pixel. Texture appears pixelated.</summary>
		Point = 0,
		/// <summary>Filter using the linear average of the nearby pixels. Texture appears blurry.</summary>
		Bilinear = 1,
		/// <summary>Filter using the linear average of the nearby pixels and nearby mipmaps. Texture appears blurry.</summary>
		Trilinear = 2,
		/// <summary>Filter using the anisotropic filtering that improves quality when viewing textures at a steep angles. Texture appears sharp at extreme viewing angles.</summary>
		Anisotropic = 3,

		MAX
	};

	/// <summary>
	/// GPU sampler address modes.
	/// </summary>
	SE_ENUM(Reflect)
	enum class GPUSamplerAddressMode
	{
		/// <summary>Texture coordinates wrap back to the valid range.</summary>
		Wrap = 0,
		/// <summary>Texture coordinates are clamped within the valid range.</summary>
		Clamp = 1,
		/// <summary>Texture coordinates flip every time the size of the valid range is passed.</summary>
		Mirror = 2,
		/// <summary>Texture coordinates outside of the valid range will return a separately set border color.</summary>
		Border = 3,

		MAX
	};

	/// <summary>
	/// GPU sampler comparision function types.
	/// </summary>
	SE_ENUM(Reflect)
	enum class GPUSamplerCompareFunction
	{
		/// <summary>Never pass the comparison.</summary>
		Never = 0,
		/// <summary>If the source data is less than the destination data, the comparison passes.</summary>
		Less = 1,

		MAX
	};

	/// <summary>
	/// GPU sampler border color types.
	/// </summary>
	SE_ENUM(Reflect)
	enum class GPUSamplerBorderColor
	{
		/// <summary>
		/// Indicates black, with the alpha component as fully transparent.
		/// </summary>
		TransparentBlack = 0,

		/// <summary>
		/// Indicates black, with the alpha component as fully opaque.
		/// </summary>
		OpaqueBlack = 1,

		/// <summary>
		/// Indicates white, with the alpha component as fully opaque.
		/// </summary>
		OpaqueWhite = 2,

		MAX
	};

	/// <summary>
	/// A common description for all samplers.
	/// </summary>
	struct SE_API_RUNTIME GPUSamplerDescription
	{
		/// <summary>
		/// The filtering method to use when sampling a texture.
		/// </summary>
		GPUSamplerFilter Filter;

		/// <summary>
		/// The addressing mode for outside [0..1] range for U coordinate.
		/// </summary>
		GPUSamplerAddressMode AddressU;

		/// <summary>
		/// The addressing mode for outside [0..1] range for V coordinate.
		/// </summary>
		GPUSamplerAddressMode AddressV;

		/// <summary>
		/// The addressing mode for outside [0..1] range for W coordinate.
		/// </summary>
		GPUSamplerAddressMode AddressW;

		/// <summary>
		/// The mip bias to be added to mipmap LOD calculation.
		/// </summary>
		float MipBias;

		/// <summary>
		/// The minimum mip map level that will be used, where 0 is the highest resolution mip level.
		/// </summary>
		float MinMipLevel;

		/// <summary>
		/// The maximum mip map level that will be used, where 0 is the highest resolution mip level. To have no upper limit on LOD set this to a large value such as MAX_float.
		/// </summary>
		float MaxMipLevel;

		/// <summary>
		/// The maximum number of samples that can be used to improve the quality of sample footprints that are anisotropic.
		/// </summary>
		int32 MaxAnisotropy;

		/// <summary>
		/// The border color to use if Border is specified for AddressU, AddressV, or AddressW.
		/// </summary>
		GPUSamplerBorderColor BorderColor;

		/// <summary>
		/// A function that compares sampled data against existing sampled data.
		/// </summary>
		GPUSamplerCompareFunction ComparisonFunction;

	public:
		/// <summary>
		/// Creates a new <see cref="GPUSamplerDescription" /> with default settings.
		/// </summary>
		/// <param name="filter">The filtering method.</param>
		/// <param name="addressMode">The addressing mode.</param>
		/// <returns>A new instance of <see cref="GPUSamplerDescription" /> class.</returns>
		static GPUSamplerDescription New(GPUSamplerFilter filter = GPUSamplerFilter::Point,
			GPUSamplerAddressMode addressMode = GPUSamplerAddressMode::Wrap);

	public:
		void Clear();
		bool Equals(const GPUSamplerDescription& other) const;
		String ToString() const;

	public:
		inline bool operator==(const GPUSamplerDescription& other) const
		{
			return Equals(other);
		}

		inline bool operator!=(const GPUSamplerDescription& other) const
		{
			return !Equals(other);
		}
	};

	uint32 GetHash(const GPUSamplerDescription& key);
}
