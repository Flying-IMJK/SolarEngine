
#include "GPUDevice.h"
#include "RenderTargetPool.h"
#include "Core/Profiler/ProfilerGPU.h"
#include "Core/Platform/Window.h"

#include "Runtime/Graphics/Base/GPUPipelineState.h"
#include "Runtime/Graphics/Base/GPUResource.h"
#include "Runtime/Graphics/Async/GPUTasksContext.h"
#include "Runtime/Graphics/Async/GPUTasksSystem.h"
#include "Runtime/Graphics/Async/GPUTasksExecutor.h"
#include "Runtime/Graphics/GPUSwapChain.h"
#include "Runtime/GraphicsDevice/Vulkan/GPUDeviceVulkan.h"
#include "Runtime/Render/GUI/GUIRenderer.h"
#include "Runtime/Render/RenderTask.h"
#include "Runtime/Engine.h"
#include "GPUResourceProperty.h"
#include "Core/Profiler/ProfilerCPU.h"
#include "Runtime/Render/2D/Render2D.h"

namespace SE
{
	GPUDevice* GPUDevice::instance = nullptr;

	bool GPUDevice::Create(GPUGlobalSettings settings)
	{
		switch (settings.type)
		{
		case GPURendererType::Vulkan:
			settings.shaderProfile = ShaderProfile::Vulkan_SM5;
			instance = GPUDeviceVulkan::Create(settings);
			break;
		}

		return instance != nullptr;
	}

	GPUDevice::GPUDevice(GPUGlobalSettings settings) :
		m_GPUSetting(settings),
		m_RenderType(settings.type),
		m_ShaderProfile(settings.shaderProfile)
	{
	}

	void GPUDevice::PreDispose()
	{
		locker.Lock();
		RenderTargetPool::Flush();

		// Release resources
//		m_Res->DefaultMaterial = nullptr;
//		_res->DefaultDeformableMaterial = nullptr;
//		_res->DefaultNormalMap = nullptr;
//		_res->DefaultWhiteTexture = nullptr;
//		_res->DefaultBlackTexture = nullptr;
//		SAFE_DELETE_GPU_RESOURCE(_res->PS_CopyLinear);
//		SAFE_DELETE_GPU_RESOURCE(_res->PS_Clear);
//		SAFE_DELETE_GPU_RESOURCE(_res->FullscreenTriangleVB);

		locker.Unlock();

		// Release GPU resources memory and unlink from device
		// Note: after that no GPU resources should be used/created, only deleted
		m_ResourcesLock.Lock();
		for (int32 i = m_Resources.Count() - 1; i >= 0 && i < m_Resources.Count(); i--)
		{
			m_Resources[i]->ReleaseGPU();
		}
		m_Resources.Clear();
		m_ResourcesLock.Unlock();
	}


	void GPUDevice::AddResource(GPUResource* resource)
	{
		m_ResourcesLock.Lock();
		ENGINE_ASSERT(resource && !m_Resources.Contains(resource));
		m_Resources.Add(resource);
		m_ResourcesLock.Unlock();
	}

	void GPUDevice::RemoveResource(GPUResource* resource)
	{
		m_ResourcesLock.Lock();
		ENGINE_ASSERT(resource && m_Resources.Contains(resource));
		m_Resources.Remove(resource);
		m_ResourcesLock.Unlock();
	}

	void GPUDevice::Draw(Function<void(GPUContext*)> renderCall)
	{
		DrawBegin();

		auto context = GetMainContext();

		context->FrameBegin();
		m_TasksManager->FrameBegin();
		Render2D::BeginFrame();

		renderCall(context);

		Render2D::EndFrame();
		m_TasksManager->FrameEnd();
		context->FrameEnd();

		DrawEnd();
	}

	void GPUDevice::DrawBegin()
	{
		// Set flag
		m_IsRendering = true;
	}

	void GPUDevice::DrawEnd()
	{
		PROFILE_CPU_NAMED("Present");

		// Check if use VSync
		bool useVSync = true;

		// Find index of the last rendered window task (use vsync only on the last window)
		int32 lastWindowIndex = -1;
		for (int32 i = RenderTask::Tasks.Count() - 1; i >= 0; i--)
		{
			const auto task = RenderTask::Tasks[i];
			if (task && task->LastUsedFrame == Engine::FrameCount && task->SwapChain && task->SwapChain->IsReady())
			{
				lastWindowIndex = i;
				break;
			}
		}

		// Call present on all used tasks
		int32 presentCount = 0;
		bool anyVSync = false;
#if SGE_PROFILER
		const double presentStart = Platform::GetTimeSeconds();
#endif
		for (int32 i = 0; i < RenderTask::Tasks.Count(); i++)
		{
			const auto task = RenderTask::Tasks[i];
			if (task && task->LastUsedFrame == Engine::FrameCount && task->SwapChain && task->SwapChain->IsReady())
			{
				bool vsync = useVSync;
				if (lastWindowIndex != i)
				{
					// Perform VSync only on the last window
					vsync = false;
				}
				else
				{
					// End profiler timer queries
#if SGE_PROFILER
					ProfilerGPU::OnPresent();
#endif
				}

				anyVSync |= vsync;
				task->OnPresent(vsync);
				presentCount++;
			}
		}

		// If no `Present` calls has been performed just execute GPU commands
		if (presentCount == 0)
		{
			// End profiler timer queries
#if SGE_PROFILER
			ProfilerGPU::OnPresent();
#endif
			GetMainContext()->Flush();
		}
#if SGE_PROFILER
		const double presentEnd = Platform::GetTimeSeconds();
		ProfilerGPU::OnPresentTime((float)((presentEnd - presentStart) * 1000.0));
#endif

		m_IsVSyncUsed = anyVSync;
		m_IsRendering = false;
		RenderTargetPool::Flush();
	}

	bool GPUDevice::Init()
	{
		// Clamp texture limits (eg. if driver reports higher value)
		limits.MaximumTexture1DSize = Math::Min(limits.MaximumTexture1DSize, GPU_MAX_TEXTURE_SIZE);
		limits.MaximumTexture2DSize = Math::Min(limits.MaximumTexture2DSize, GPU_MAX_TEXTURE_SIZE);
		limits.MaximumTexture3DSize = Math::Min(limits.MaximumTexture3DSize, GPU_MAX_TEXTURE_SIZE);
		limits.MaximumTextureCubeSize = Math::Min(limits.MaximumTextureCubeSize, GPU_MAX_TEXTURE_SIZE);
		limits.MaximumTexture1DArraySize = Math::Min(limits.MaximumTexture1DArraySize, GPU_MAX_TEXTURE_ARRAY_SIZE);
		limits.MaximumTexture2DArraySize = Math::Min(limits.MaximumTexture2DArraySize, GPU_MAX_TEXTURE_ARRAY_SIZE);
		limits.MaximumMipLevelsCount = Math::Min(limits.MaximumMipLevelsCount, GPU_MAX_TEXTURE_MIP_LEVELS);

		m_TasksManager = New<GPUTasksSystem>();
		m_TasksManager->SetExecutor(CreateTasksExecutor());
		LOG_INFO("Graphic", "Total graphics memory: {0}", m_TotalGraphicsMemory/*Utilities::BytesToText(m_TotalGraphicsMemory)*/);
		if (!limits.HasCompute)
		{
			LOG_WARNING("Graphic", "Compute Shaders are not supported");
		}

		return true;
	}

	void GPUDevice::Dispose()
	{

	}

	uint64 GPUDevice::GetMemoryUsage() const
	{
		uint64 result = 0;
		m_ResourcesLock.Lock();
		for (int32 i = 0; i < m_Resources.Count(); i++)
			result += m_Resources[i]->GetMemoryUsage();
		m_ResourcesLock.Unlock();
		return result;
	}

	GPUTasksSystem* GPUDevice::GetTasksSystem() const
	{
		return m_TasksManager;
	}

	GPUTasksExecutor* GPUDevice::CreateTasksExecutor()
	{
		return New<DefaultGPUTasksExecutor>();
	}

	GPUTasksContext* GPUDevice::CreateTasksContext()
	{
		return New<GPUTasksContext>(this);
	}

	GPUResourcePropertyBase::~GPUResourcePropertyBase()
	{
		const auto e = _resource;
		if (e)
		{
			_resource = nullptr;
			e->Releasing.Unbind<GPUResourcePropertyBase, &GPUResourcePropertyBase::OnReleased>(this);
		}
	}

	void GPUResourcePropertyBase::OnSet(GPUResource* resource)
	{
		auto e = _resource;
		if (e != resource)
		{
			if (e)
				e->Releasing.Unbind<GPUResourcePropertyBase, &GPUResourcePropertyBase::OnReleased>(this);
			_resource = e = resource;
			if (e)
				e->Releasing.Bind<GPUResourcePropertyBase, &GPUResourcePropertyBase::OnReleased>(this);
		}
	}

	void GPUResourcePropertyBase::OnReleased()
	{
		auto e = _resource;
		if (e)
		{
			_resource = nullptr;
			e->Releasing.Unbind<GPUResourcePropertyBase, &GPUResourcePropertyBase::OnReleased>(this);
		}
	}

	GPUResourcePropertyBase::GPUResourcePropertyBase(GPUResource* resource)
	{
		_resource = resource;
	}

	// GPUResource
	uint64 GPUResource::GetMemoryUsage() const
	{
		return m_MemoryUsage;
	}

	void GPUResource::ReleaseGPU()
	{
		if (m_MemoryUsage != 0)
		{
			Releasing();
			OnReleaseGPU();
			m_MemoryUsage = 0;
		}
	}

	GPUResource::GPUResource()
	{
	}

	void GPUResource::OnReleaseGPU()
	{

	}

	String GPUResource::ToString() const
	{
		return String();
	}

	StringView GPUResource::GetName() const
	{
#if GPU_ENABLE_RESOURCE_NAMING
		return StringView(m_NamePtr, m_NameSize);
#endif
		return StringView::Empty;
	}

	void GPUResource::SetName(const StringView& name)
	{
#if GPU_ENABLE_RESOURCE_NAMING
		if (m_NameCapacity < name.Length() + 1)
		{
			if (m_NamePtr)
				Platform::Free(m_NamePtr);
			m_NameCapacity = name.Length() + 1;
			m_NamePtr = (Char*)Platform::Allocate(m_NameCapacity * sizeof(Char), 16);
		}
		m_NameSize = name.Length();
		if (name.HasChars())
		{
			Platform::MemoryCopy(m_NamePtr, name.Get(), m_NameSize * sizeof(Char));
			m_NamePtr[m_NameSize] = 0;
		}
#endif

	}

	void GPUResource::OnDeleteObject()
	{
		ReleaseGPU();

		Object::OnDeleteObject();
	}

	// GPUPipelineState
	GPUPipelineState* GPUPipelineState::New()
	{
		return GPUDevice::instance->CreatePipelineState();
	}

	GPUPipelineState::GPUPipelineState(): complexity(0)
	{
	}

	bool GPUPipelineState::Init(const Description& desc)
	{
		// Cache description in debug builds
//#if BUILD_DEBUG
		debugDesc = desc;
//#endif

		// Cache shader stages usage flags for pipeline state
		m_Meta.instructionsCount = 0;
		m_Meta.usedCBsMask = 0;
		m_Meta.usedSRsMask = 0;
		m_Meta.usedUAsMask = 0;
#define CHECK_STAGE(stage) \
    if (desc.stage) { \
        m_Meta.usedCBsMask |= desc.stage->GetBindings().usedCBsMask; \
        m_Meta.usedSRsMask |= desc.stage->GetBindings().usedSRsMask; \
        m_Meta.usedUAsMask |= desc.stage->GetBindings().usedUAsMask; \
    }
		CHECK_STAGE(VS);
		CHECK_STAGE(HS);
		CHECK_STAGE(DS);
		CHECK_STAGE(GS);
		CHECK_STAGE(PS);
#undef CHECK_STAGE

//#if USE_EDITOR
		// Estimate somehow performance cost of this pipeline state for the content profiling
		const int32 textureLookupCost = 20;
		const int32 tessCost = 300;
		complexity = Math::CountBits(m_Meta.usedSRsMask) * textureLookupCost;
		if (desc.PS)
			complexity += desc.PS->GetBindings().instructionsCount;
		if (desc.HS || desc.DS)
			complexity += tessCost;
		if (desc.DepthWriteEnable)
			complexity += 5;
		if (desc.DepthEnable)
			complexity += 5;
		if (desc.BlendMode.BlendEnable)
			complexity += 20;
//#endif

		return true;
	}

	GPUResourceType GPUPipelineState::GetResType() const
	{
		return GPUResourceType::PipelineState;
	}
}
