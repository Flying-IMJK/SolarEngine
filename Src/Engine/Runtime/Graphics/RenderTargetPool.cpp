
#include "RenderTargetPool.h"
#include "Runtime/Core/Types/Collections/List.h"
#include "Runtime/Core/Platform/StringUtils.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/Profiler/ProfilerCPU.h"

#include "GPUDevice.h"
#include "Runtime/EngineContext.h"
#include "Runtime/Engine.h"

namespace SE
{
	struct Entry
	{
		GPUTexture* RT;
		uint64 LastFrameReleased;
		uint32 DescriptionHash;
		bool IsOccupied;
	};

	namespace
	{
		List<Entry> TemporaryRTs;
	}

	void RenderTargetPool::Flush(bool force, int32 framesOffset)
	{
		PROFILE_CPU();
		if (framesOffset < 0)
			framesOffset = 3 * 60; // For how many frames RTs should be cached (by default)
		const uint64 frameCount = 0;//Engine::FrameCount;
		const uint64 maxReleaseFrame = frameCount - Math::Min<uint64>(frameCount, framesOffset);
//		force |= Engine::ShouldExit();

		for (int32 i = 0; i < TemporaryRTs.Count(); i++)
		{
			const auto& e = TemporaryRTs[i];
			if (!e.IsOccupied && (force || e.LastFrameReleased < maxReleaseFrame))
			{
				e.RT->ReleaseGPU();// DeleteObjectNow();
				TemporaryRTs.RemoveAt(i--);
				if (TemporaryRTs.IsEmpty())
					break;
			}
		}
	}

	GPUTexture* RenderTargetPool::Get(const GPUTextureDescription& desc)
	{
		PROFILE_CPU();

		// Find free render target with the same properties
		const uint32 descHash = GetHash(desc);
		for (int32 i = 0; i < TemporaryRTs.Count(); i++)
		{
			auto& e = TemporaryRTs[i];
			if (!e.IsOccupied && e.DescriptionHash == descHash)
			{
				// Mark as used
				e.IsOccupied = true;
				return e.RT;
			}
		}

		if (TemporaryRTs.Count() > 2000)
		{
			LOG_ERROR("Graphic", "RenderTargetPool Too many textures allocated in RenderTargetPool. Know your limits, sir!");
			return nullptr;
		}

		// Create new rt
		const String name = SE_TEXT("TemporaryRT_") + StringUtils::ToString(TemporaryRTs.Count());
		GPUTexture* rt = GPUDevice::instance->CreateTexture(name);
		if (!rt->Init(desc))
		{
			Delete(rt);
			LOG_ERROR("Graphic", "RenderTargetPool Cannot create temporary render target. Description: {0}", desc.ToString());
			return nullptr;
		}

		// Create temporary rt entry
		Entry e;
		e.IsOccupied = true;
		e.LastFrameReleased = 0;
		e.RT = rt;
		e.DescriptionHash = descHash;
		TemporaryRTs.Add(e);

		return rt;
	}

	void RenderTargetPool::Release(GPUTexture* rt)
	{
		if (!rt)
			return;
		for (int32 i = 0; i < TemporaryRTs.Count(); i++)
		{
			auto& e = TemporaryRTs[i];
			if (e.RT == rt)
			{
				// Mark as free
				ENGINE_ASSERT(e.IsOccupied);
				e.IsOccupied = false;
				e.LastFrameReleased = Engine::FrameCount;
				return;
			}
		}
		LOG_ERROR("Graphic", "RenderTargetPool Trying to release temporary render target which has not been registered in service!");
	}
} // SE