#pragma once

#include "Core/Types/Variable.h"
#include "Runtime/API.h"
#include "Runtime/Graphics/Textures/GPUTexture.h"

namespace SE
{
	struct FloatR10G10B10A2;
	struct RenderView;
    struct RenderContext;
    class Model;

}

namespace SE::RenderUtils
{
    bool SE_API_RUNTIME CreateTextureFromFile(String const& path, GPUTexture*& texture);

    bool SE_API_RUNTIME CreateTextureFromBase64(uint8 const* pData, uint64 size, uint32 width, uint32 height, GPUTexture*& texture);


	void SE_API_RUNTIME UpdateModelLODTransition(byte& lodTransition);

    /// <summary>
    /// Computes the bounds screen radius squared.
    /// </summary>
    /// <param name="origin">The bounds origin.</param>
    /// <param name="radius">The bounds radius.</param>
    /// <param name="view">The render view.</param>
    /// <returns>The squared radius.</returns>
	float SE_API_RUNTIME ComputeBoundsScreenRadiusSquared(const Float3& origin, const float radius, const RenderView& view);

    /// <summary>
    /// Computes the bounds screen radius squared.
    /// </summary>
    /// <param name="origin">The bounds origin.</param>
    /// <param name="radius">The bounds radius.</param>
    /// <param name="viewOrigin">The render view position.</param>
    /// <param name="projectionMatrix">The render view projection matrix.</param>
    /// <returns>The squared radius.</returns>
	float SE_API_RUNTIME ComputeBoundsScreenRadiusSquared(const Float3& origin, float radius, const Float3& viewOrigin, const Matrix& projectionMatrix);


    /// <summary>
    /// Computes the model LOD index to use during rendering.
    /// </summary>
    /// <param name="model">The model.</param>
    /// <param name="origin">The bounds origin.</param>
    /// <param name="radius">The bounds radius.</param>
    /// <param name="renderContext">The rendering context.</param>
    /// <returns>The zero-based LOD index. Returns -1 if model should not be rendered.</returns>
    int32 SE_API_RUNTIME ComputeModelLOD(const Model* model, const Float3& origin, float radius, const RenderContext& renderContext);

	uint64 SE_API_RUNTIME CalculateTextureMemoryUsage(PixelFormat format, int32 width, int32 height, int32 mipLevels);

	FORCE_INLINE uint64 CalculateTextureMemoryUsage(PixelFormat format, int32 width, int32 height, int32 depth, int32 mipLevels)
	{
		return CalculateTextureMemoryUsage(format, width, height, mipLevels) * depth;
	}


	void SE_API_RUNTIME CalculateTangentFrame(FloatR10G10B10A2& resultNormal, FloatR10G10B10A2& resultTangent, const Float3& normal);
	void SE_API_RUNTIME CalculateTangentFrame(FloatR10G10B10A2& resultNormal, FloatR10G10B10A2& resultTangent, const Float3& normal, const Float3& tangent);

	/// <summary>
	/// Computes the sorting key for depth value (quantized)
	/// Reference: http://aras-p.info/blog/2014/01/16/rough-sorting-by-depth/
	/// </summary>
	FORCE_INLINE static uint32 ComputeDistanceSortKey(float distance)
	{
		const uint32 distanceI = *((uint32*)&distance);
		return ((uint32)(-(int32)(distanceI >> 31)) | 0x80000000) ^ distanceI;
	}
}
