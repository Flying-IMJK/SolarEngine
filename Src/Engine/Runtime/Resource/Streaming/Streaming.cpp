
#include "Streaming.h"
#include "StreamableResource.h"
#include "StreamingGroup.h"
#include "StreamingSettings.h"
#include "Runtime/Core/Systems.h"
#include "Runtime/Core/Profiler/ProfilerCPU.h"
#include "Runtime/Core/Thread/Threading.h"
#include "Runtime/Core/Thread/TaskGraph.h"
#include "Runtime/Core/Thread/Task.h"

#include "Runtime/Core/Serialization/Serialization.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/Graphics/Textures/GPUSampler.h"
#include "Runtime/Engine.h"

namespace SE
{
	struct StreamingSystemData
	{
		int32 LastUpdateResourcesIndex = 0;
		CriticalSection ResourcesLock;
		List<StreamableResource*> Resources;
		List<GPUSampler*, InlinedAllocation<32>> TextureGroupSamplers;
		GPUSampler* FallbackSampler = nullptr;
		Threading::TaskGraphSystem* System = nullptr;
		List<TextureGroup, InlinedAllocation<32>> TextureGroups;
	};

	StreamingSystemData* streamingData = nullptr;

	class StreamingSystem : public ISystem
	{
	ENGINE_SYSTEM(StreamingSystem)
	public:
		StreamingSystem()
			: ISystem(SE_TEXT("Streaming"), -499)
		{
		}

		bool OnInit() override;
		void OnBeforeExit() override;
	};

	class StreamingTaskSystem : public Threading::TaskGraphSystem
	{
	public:
		void Job(int32 index);
		void Execute(Threading::TaskGraph* graph) override;
	};

	ENGINE_SYSTEM_REGISTER(StreamingSystem)

	bool StreamingSystem::OnInit()
	{
		streamingData = New<StreamingSystemData>();

		streamingData->System = New<StreamingTaskSystem>();
		Engine::UpdateGraph->AddSystem(streamingData->System);
		return false;
	}

	void StreamingSystem::OnBeforeExit()
	{
//	SAFE_DELETE_GPU_RESOURCE(FallbackSampler);
//	SAFE_DELETE_GPU_RESOURCES(TextureGroupSamplers);
		streamingData->TextureGroupSamplers.Resize(0);
//	SAFE_DELETE(System);
	}

	void UpdateResource(StreamableResource* resource, double currentTime);

	List<TextureGroup, InlinedAllocation<32>> Streaming::GetTextureGroups()
	{
		return streamingData->TextureGroups;
	}


/*	void StreamingSettings::Apply()
	{
		Streaming::TextureGroups = TextureGroups;
		SAFE_DELETE_GPU_RESOURCES(TextureGroupSamplers);
		TextureGroupSamplers.Resize(TextureGroups.Count(), false);
	}

	void StreamingSettings::Deserialize(DeserializeStream& stream, ISerializeModifier* modifier)
	{
		DESERIALIZE(TextureGroups);
	}*/

	StreamableResource::StreamableResource() : m_Group(nullptr), m_IsDynamic(true), m_IsStreaming(false), m_StreamingQuality(1.0f)
	{
	}

	StreamableResource::StreamableResource(StreamingGroup* group)
		: m_Group(group), m_IsDynamic(true), m_IsStreaming(false), m_StreamingQuality(1.0f)
	{
		// ENGINE_ASSERT(_group != nullptr);
	}

	StreamableResource::~StreamableResource()
	{
		StopStreaming();
	}

	void StreamableResource::RequestStreamingUpdate()
	{
		Streaming.LastUpdateTime = 0.0;
	}

	void StreamableResource::ResetStreaming(bool error)
	{
		Streaming.Error = error;
		Streaming.TargetResidency = 0;
		Streaming.LastUpdateTime = 3e+30f; // Very large number to skip any updates
	}

	void StreamableResource::StartStreaming(bool isDynamic)
	{
		m_IsDynamic = isDynamic;

		if (!m_IsStreaming)
		{
			m_IsStreaming = true;
			streamingData->ResourcesLock.Lock();
			streamingData->Resources.Add(this);
			streamingData->ResourcesLock.Unlock();
		}
	}

	void StreamableResource::StopStreaming()
	{
		if (m_IsStreaming)
		{
			streamingData->ResourcesLock.Lock();
			streamingData->Resources.Remove(this);
			streamingData->ResourcesLock.Unlock();
			Streaming = StreamingCache();
			m_IsStreaming = false;
		}
	}

	void UpdateResource(StreamableResource* resource, double currentTime)
	{
		ENGINE_ASSERT(resource && resource->CanBeUpdated());

		// Pick group and handler dedicated for that resource
		auto group = resource->GetGroup();
		auto handler = group->GetHandler();

		// Calculate target quality for that asset
		float targetQuality = 1.0f;
		if (resource->IsDynamic())
		{
			targetQuality = handler->CalculateTargetQuality(resource, currentTime);
			targetQuality = Math::Saturate(targetQuality);
		}

		// Update quality smoothing
		resource->Streaming.QualitySamples.Add(targetQuality);
		targetQuality = resource->Streaming.QualitySamples.Maximum();
		targetQuality = Math::Saturate(targetQuality);

		// Calculate target residency level (discrete value)
		auto maxResidency = resource->GetMaxResidency();
		auto currentResidency = resource->GetCurrentResidency();
		auto allocatedResidency = resource->GetAllocatedResidency();
		auto targetResidency = handler->CalculateResidency(resource, targetQuality);
		ENGINE_ASSERT(allocatedResidency >= currentResidency && allocatedResidency >= 0);
		resource->Streaming.LastUpdateTime = currentTime;

		// Check if a target residency level has been changed
		if (targetResidency != resource->Streaming.TargetResidency)
		{
			// Register change
			resource->Streaming.TargetResidency = targetResidency;
			resource->Streaming.TargetResidencyChangeTime = currentTime;
		}

		// Check if need to change resource current residency
		if (handler->RequiresStreaming(resource, currentResidency, targetResidency))
		{
			// Check if need to change allocation for that resource
			if (allocatedResidency != targetResidency)
			{
				// Update resource allocation
				Threading::Task* allocateTask = resource->UpdateAllocation(targetResidency);
				if (allocateTask)
				{
					// When resource wants to perform reallocation on a task then skip further updating until it's done
					allocateTask->Start();
					resource->RequestStreamingUpdate();
					return;
				}
				else if (resource->GetAllocatedResidency() < targetResidency)
				{
					// Allocation failed (eg. texture format is not supported or run out of memory)
					resource->ResetStreaming();
					return;
				}
			}

			// Calculate residency level to stream in (resources may want to increase/decrease it's quality in steps rather than at once)
			int32 requestedResidency = handler->CalculateRequestedResidency(resource, targetResidency);

			// Create streaming task (resource type specific)
			Threading::Task* streamingTask = resource->CreateStreamingTask(requestedResidency);
			if (streamingTask != nullptr)
			{
				streamingTask->Start();
			}
		}
		else
		{
			// TODO: Check if target residency is stable (no changes for a while)

			// TODO: deallocate or decrease memory usage after timeout? (timeout should be smaller on low mem)
		}

		// low memory case:
		// if we are on budget and cannot load everything we have to:
		// decrease global resources quality scale (per resources group)
		// decrease asset deallocate timeout

		// low mem detecting:
		// for low mem we have to get whole memory budget for a group and then
		// subtract immutable resources mem usage (like render targets or non dynamic resources)
		// so we get amount of memory we can distribute and we can detect if we are out of the limit
		// low mem should be updated once per a few frames
	}

	void StreamingTaskSystem::Job(int32 index)
	{
		PROFILE_CPU_NAMED("Streaming.Job");

		// TODO: use streaming settings
		const double ResourceUpdatesInterval = 0.1;
		int32 MaxResourcesPerUpdate = 50;

		// Start update
		Threading::ScopeLock lock(streamingData->ResourcesLock);
		const int32 resourcesCount = streamingData->Resources.Count();
		int32 resourcesUpdates = Math::Min(MaxResourcesPerUpdate, resourcesCount);
		const double currentTime = Platform::GetTimeSeconds();

		// Update high priority queue and then rest of the resources
		// Note: resources in the update queue are updated always, while others only between specified intervals
		int32 resourcesChecks = resourcesCount;
		while (resourcesUpdates > 0 && resourcesChecks-- > 0)
		{
			// Move forward
			streamingData->LastUpdateResourcesIndex++;
			if (streamingData->LastUpdateResourcesIndex >= resourcesCount)
				streamingData->LastUpdateResourcesIndex = 0;

			// Peek resource
			const auto resource = streamingData->Resources[streamingData->LastUpdateResourcesIndex];

			// Try to update it
			if (currentTime - resource->Streaming.LastUpdateTime >= ResourceUpdatesInterval && resource->CanBeUpdated())
			{
				UpdateResource(resource, currentTime);
				resourcesUpdates--;
			}
		}

		// TODO: add StreamingManager stats, update time per frame, updates per frame, etc.
	}

	void StreamingTaskSystem::Execute(Threading::TaskGraph* graph)
	{
		if (streamingData->Resources.Count() == 0 || GPUDevice::instance->GetState() != GPUDevice::DeviceState::Ready)
			return;

		// Schedule work to update all storage containers in async
		Function<void(int32)> job;
		job.Bind<StreamingTaskSystem, &StreamingTaskSystem::Job>(this);
		graph->DispatchJob(job, 1);
	}

	StreamingStats Streaming::GetStats()
	{
		StreamingStats stats;
		streamingData->ResourcesLock.Lock();
		stats.ResourcesCount = streamingData->Resources.Count();
		for (auto e : streamingData->Resources)
		{
			if (e->Streaming.TargetResidency > e->GetCurrentResidency())
				stats.StreamingResourcesCount++;
		}
		streamingData->ResourcesLock.Unlock();
		return stats;
	}

	void Streaming::RequestStreamingUpdate()
	{
		PROFILE_CPU();
		streamingData->ResourcesLock.Lock();
		for (auto e : streamingData->Resources)
			e->RequestStreamingUpdate();
		streamingData->ResourcesLock.Unlock();
	}

	GPUSampler* Streaming::GetTextureGroupSampler(int32 index)
	{
		GPUSampler* sampler = nullptr;
		if (index >= 0 && index < streamingData->TextureGroupSamplers.Count())
		{
			// Sampler from texture group options
			auto& group = streamingData->TextureGroups[index];
			auto desc = GPUSamplerDescription::New(group.SamplerFilter);
			desc.MaxAnisotropy = group.MaxAnisotropy;
			sampler = streamingData->TextureGroupSamplers[index];
			if (!sampler)
			{
				sampler = GPUSampler::New();
#if GPU_ENABLE_RESOURCE_NAMING
//            sampler-> SetName(group.Name);
#endif
				sampler->Init(desc);
				streamingData->TextureGroupSamplers[index] = sampler;
			}
			if (sampler->GetDescription().Filter != desc.Filter || sampler->GetDescription().MaxAnisotropy != desc.MaxAnisotropy)
				sampler->Init(desc);
		}
		if (!sampler)
		{
			// Default sampler to prevent issue
			if (!streamingData->FallbackSampler)
			{
				streamingData->FallbackSampler = GPUSampler::New();
#if GPU_ENABLE_RESOURCE_NAMING
//            FallbackSampler->SetName(SE_TEXT("FallbackSampler"));
#endif
				streamingData->FallbackSampler->Init(GPUSamplerDescription::New(GPUSamplerFilter::Trilinear));
			}
			sampler = streamingData->FallbackSampler;
		}
		return sampler;
	}

}