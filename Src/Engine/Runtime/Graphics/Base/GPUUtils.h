#pragma once

#include "PixelFormat.h"
#include "GPUEnums.h"

namespace SE
{

	class SE_API_RUNTIME GPUUtils
	{
	public:
		// Calculate a subresource index for a texture
		inline static int32 CalcSubresourceIndex(uint32 mipSlice, int32 arraySlice, int32 mipLevels)
		{
			return mipSlice + arraySlice * mipLevels;
		}

		/// <summary>
		/// Computes the image row pitch in bytes, and the slice pitch (size in bytes of the image) based on format, width, and height.
		/// </summary>
		/// <param name="format">The pixel format of the surface.</param>
		/// <param name="width">The surface width.</param>
		/// <param name="height">The surface height.</param>
		/// <param name="rowPitch">The number of bytes in a scanline of pixels in the image. A standard pitch is 'byte' aligned and therefore it is equal to bytes-per-pixel * width-of-image. For block-compressed (BC) formats, this is the number of bytes in a row of blocks (which covers up to 4 scanlines at once). The rowPitch can be larger than the number of valid pixels in the image due to alignment padding.</param>
		/// <param name="slicePitch">For volume (3D) textures, slicePitch is the number of bytes in each depth slice. For 1D and 2D images, this is simply the total size of the image including any alignment padding.</param>
		static void ComputePitch(PixelFormat format, int32 width, int32 height, uint32& rowPitch, uint32& slicePitch);

		// Calculate mip levels count for a texture 1D
		// @param width Most detailed mip width
		// @param useMipLevels True if use mip levels, otherwise false (use only 1 mip)
		// @returns Mip levels count
		static int32 MipLevelsCount(int32 width, bool useMipLevels = true);

		// Calculate mip levels count for a texture 2D
		// @param width Most detailed mip width
		// @param height Most detailed mip height
		// @param useMipLevels True if use mip levels, otherwise false (use only 1 mip)
		// @returns Mip levels count
		static int32 MipLevelsCount(int32 width, int32 height, bool useMipLevels = true);

		// Calculate mip levels count for a texture 3D
		// @param width Most detailed mip width
		// @param height Most detailed mip height
		// @param depth Most detailed mip depths
		// @param useMipLevels True if use mip levels, otherwise false (use only 1 mip)
		// @returns Mip levels count
		static int32 MipLevelsCount(int32 width, int32 height, int32 depth, bool useMipLevels = true);

	public:
		static uint64 CalculateTextureMemoryUsage(PixelFormat format, int32 width, int32 height, int32 mipLevels);

		static uint64 CalculateTextureMemoryUsage(PixelFormat format, int32 width, int32 height, int32 depth, int32 mipLevels);

		/// <summary>
		/// Computes the feature level for the given shader profile.
		/// </summary>
		/// <param name="profile">The shader profile.</param>
		/// <returns>The feature level matching the given shader profile.</returns>
		static FeatureLevel GetFeatureLevel(ShaderProfile profile);

		static bool CanSupportTessellation(ShaderProfile profile);
	};

} // SE