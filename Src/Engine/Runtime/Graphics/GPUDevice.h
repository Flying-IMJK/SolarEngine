#pragma once

#include "Runtime/Core/Thread/Threading.h"
#include "Runtime/Core/Types/Strings/StringView.h"

#include "Runtime/API.h"
#include "Runtime/Graphics/Base/GPUEnums.h"
#include "GlobalSettings_GPU.h"
#include "Runtime/Graphics/Base/GPULimits.h"

namespace SE
{
	class GPUPipelineState;
	class GPUContext;
	class GPUTexture;
	class GPUShader;
	class GPUBuffer;
	class GPUSampler;
	class GPUSwapChain;
	class GPUResource;
	class GPUConstantBuffer;
	class GPUAdapter;
	class GPUTimerQuery;
	class GPUTasksSystem;
	class GPUTasksContext;
	class GPUTasksExecutor;

	class SE_API_RUNTIME GPUDevice
	{
	public:

		/// <summary>
		/// Graphics Device states that describe its lifetime.
		/// </summary>
		enum class DeviceState
		{
			Missing = 0,
			Created,
			Ready,
			Removed,
			Disposing,
			Disposed
		};

		static GPUDevice* instance;
		static bool Create(GPUGlobalSettings settings);

	public:
		GPUDevice(GPUGlobalSettings settings);


		virtual bool Init();

		/**
		 * Clean all allocated data by device
		 */
		virtual void Dispose();

		/**
		 * Wait for GPU end doing submitted work
		 */
		virtual void WaitForGPU() = 0;

		virtual void PreDispose();

		/// <summary>
		/// Gets current device state.
		/// </summary>
		inline DeviceState GetState() const
		{
			return m_DeviceState;
		}

		/// <summary>
		/// Returns true if device is during rendering state, otherwise false.
		/// </summary>
		inline bool IsRendering() const
		{
			return m_IsRendering;
		}


	public:
		virtual void DrawBegin();

		virtual void Draw(Function<void(GPUContext*)> renderCall);

		virtual void DrawEnd();

	public:

		/// <summary>
		/// Gets the device renderer type.
		/// </summary>
		FORCE_INLINE GPURendererType GetRendererType() const
		{
			return m_RenderType;
		}

		/// <summary>
		/// Gets device shader profile type.
		/// </summary>
		FORCE_INLINE ShaderProfile GetShaderProfile() const
		{
			return m_ShaderProfile;
		}

		/// <summary>
		/// Gets device feature level type.
		/// </summary>
		FORCE_INLINE FeatureLevel GetFeatureLevel() const
		{
			return _featureLevel;
		}

		// Create a SwapChain.
		virtual GPUSwapChain* CreateSwapChain(Window* window) = 0;

		virtual GPUPipelineState* CreatePipelineState() = 0;

		virtual GPUTexture* CreateTexture(const StringView& name = StringView::Empty) = 0;

		virtual GPUShader* CreateShader(const StringView& name = StringView::Empty) = 0;

//		virtual GPUTimerQuery* CreateTimerQuery() = 0;

		virtual GPUBuffer* CreateBuffer(const StringView& name = StringView::Empty) = 0;

		virtual GPUSampler* CreateSampler() = 0;

		virtual GPUTimerQuery* CreateTimerQuery() = 0;

		virtual GPUConstantBuffer* CreateConstantBuffer(uint32 size, const StringView& name = StringView::Empty) = 0;

		virtual GPUContext* GetMainContext() = 0;

		/// <summary>
		/// Gets the GPU asynchronous work manager.
		/// </summary>
		GPUTasksSystem* GetTasksSystem() const;

		/// <summary>
		/// Creates the GPU tasks context.
		/// </summary>
		/// <returns>The GPU tasks context.</returns>
		virtual GPUTasksContext* CreateTasksContext();

		GPUTasksExecutor* CreateTasksExecutor();

		uint64 GetMemoryUsage() const;
		uint64 GetTotalGraphicsMemory() const
		{
			return m_TotalGraphicsMemory;
		}

		void AddResource(GPUResource* resource);
		void RemoveResource(GPUResource* resource);

		FormatFeatures GetPixelFormatFeatures(const PixelFormat format) const
		{
			return FeaturesPerFormat[(int32)format];
		}


		GPULimits GetGPULimits() const
		{
			return limits;
		};

	public:
		CriticalSection locker;

	protected:
		GPURendererType m_RenderType;
		ShaderProfile m_ShaderProfile;
		FeatureLevel _featureLevel;

		bool m_IsRendering;
		bool m_IsVSyncUsed;
		int32 m_DrawGpuEventIndex;

		DeviceState m_DeviceState;
		GPUGlobalSettings m_GPUSetting;

		List<GPUResource*> m_Resources;
		CriticalSection m_ResourcesLock;
		GPUTasksSystem* m_TasksManager;

		/// <summary>
		/// The GPU limits.
		/// </summary>
		GPULimits limits;

		/// <summary>
		/// The total amount of graphics memory in bytes.
		/// </summary>
		uint64 m_TotalGraphicsMemory;

		/// <summary>
		/// The supported features for the specified format (index is the pixel format value).
		/// </summary>
		FormatFeatures FeaturesPerFormat[static_cast<int32>(PixelFormat::Max)];
	};

	/// <summary>
	/// Utility structure to safety graphics device locking.
	/// </summary>
	struct SE_API_RUNTIME GPUDeviceLock
	{
		NON_COPYABLE(GPUDeviceLock)
		GPUDevice* const Device;

		GPUDeviceLock(GPUDevice* device)
			: Device(device)
		{
			Device->locker.Lock();
		}

		~GPUDeviceLock()
		{
			Device->locker.Unlock();
		}
	};

}
