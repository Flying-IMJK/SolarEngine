
#pragma once

#include "Runtime/Core/Types/Collections/List.h"
#include "TextureGroup.h"

namespace SE
{
	class GPUSampler;

	// Streaming service statistics container.
	struct SE_API_RUNTIME StreamingStats
	{
		// Amount of active streamable resources.
		int32 ResourcesCount = 0;
		// Amount of resources that are during streaming in (target residency is higher that the current). Zero if all resources are streamed in.
		int32 StreamingResourcesCount = 0;
	};

	/// <summary>
	/// The content streaming service.
	/// </summary>
	class SE_API_RUNTIME Streaming
	{
	public:
		/// <summary>
		/// Textures streaming configuration (per-group).
		/// </summary>
		static List<TextureGroup, InlinedAllocation<32>> GetTextureGroups();

		/// <summary>
		/// Gets streaming statistics.
		/// </summary>
		static StreamingStats GetStats();

		/// <summary>
		/// Requests the streaming update for all the loaded resources. Use it to refresh content streaming after changing configuration.
		/// </summary>
		static void RequestStreamingUpdate();

		/// <summary>
		/// Gets the texture sampler for a given texture group. Sampler objects is managed and cached by streaming service. Returned value is always valid (uses fallback object).
		/// </summary>
		/// <param name="index">The texture group index.</param>
		/// <returns>The texture sampler (always valid).</returns>
		static GPUSampler* GetTextureGroupSampler(int32 index);
	};
}