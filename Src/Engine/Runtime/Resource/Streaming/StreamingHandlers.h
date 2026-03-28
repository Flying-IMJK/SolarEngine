
#pragma once

#include "IStreamingHandler.h"

namespace SE
{
	/// <summary>
	/// Implementation of IStreamingHandler for streamable textures.
	/// </summary>
	class SE_API_RUNTIME TexturesStreamingHandler : public IStreamingHandler
	{
	public:
		// [IStreamingHandler]
		float CalculateTargetQuality(StreamableResource* resource, double currentTime) override;
		int32 CalculateResidency(StreamableResource* resource, float quality) override;
		int32 CalculateRequestedResidency(StreamableResource* resource, int32 targetResidency) override;
	};

	/// <summary>
	/// Implementation of IStreamingHandler for streamable models.
	/// </summary>
	class SE_API_RUNTIME ModelsStreamingHandler : public IStreamingHandler
	{
	public:
		// [IStreamingHandler]
		float CalculateTargetQuality(StreamableResource* resource, double currentTime) override;
		int32 CalculateResidency(StreamableResource* resource, float quality) override;
		int32 CalculateRequestedResidency(StreamableResource* resource, int32 targetResidency) override;
	};
	/*
	/// <summary>
	/// Implementation of IStreamingHandler for streamable skinned models.
	/// </summary>
	class SE_API_RUNTIME SkinnedModelsStreamingHandler : public IStreamingHandler
	{
	public:
		// [IStreamingHandler]
		float CalculateTargetQuality(StreamableResource* resource, double currentTime) override;
		int32 CalculateResidency(StreamableResource* resource, float quality) override;
		int32 CalculateRequestedResidency(StreamableResource* resource, int32 targetResidency) override;
	};

	/// <summary>
	/// Implementation of IStreamingHandler for audio clips.
	/// </summary>
	class SE_API_RUNTIME AudioStreamingHandler : public IStreamingHandler
	{
	public:
		// [IStreamingHandler]
		float CalculateTargetQuality(StreamableResource* resource, double currentTime) override;
		int32 CalculateResidency(StreamableResource* resource, float quality) override;
		int32 CalculateRequestedResidency(StreamableResource* resource, int32 targetResidency) override;
		bool RequiresStreaming(StreamableResource* resource, int32 currentResidency, int32 targetResidency) override;
	};*/
}