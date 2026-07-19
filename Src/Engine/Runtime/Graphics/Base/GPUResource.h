#pragma once

#include "Runtime/Core/Types/UID.h"
#include "Runtime/Core/Scripting/ScriptingObject.h"
#include "Runtime/Core/Types/Delegate.h"

#include "Runtime/API.h"
// GPU_ENABLE_RESOURCE_NAMING 这个宏没有引入 子类继承GPUResource ，而子类定义了宏而基类没有定义,导致子类转换到基类时出现内存不一致问题
#include "Runtime/Graphics/GlobalSettings_GPU.h"

namespace SE
{
	class GPUContextVulkan;
	class GPUResource;

	SE_ENUM(Reflect)
	enum class GPUResourceType
	{
		// GPU render target texture
		RenderTarget = 0,
		// GPU texture
		Texture,
		// GPU cube texture (cubemap)
		CubeTexture,
		// GPU volume texture (3D)
		VolumeTexture,
		// GPU buffer
		Buffer,
		// GPU shader
		Shader,
		// GPU pipeline state object (PSO)
		PipelineState,
		// GPU binding descriptor
		Descriptor,
		// GPU timer query
		Query,
		// GPU texture sampler
		Sampler,

		MAX
	};


	SE_CLASS(API, Abstract, NoSpawn, Attributes="HideInEditor")
	class SE_API_RUNTIME GPUResourceView : public ScriptingObject
	{
		SCRIPTING_TYPE_NO_SPAWN(GPUResourceView);

	protected:
		static double DummyLastRenderTime;

		GPUResourceView();
		explicit GPUResourceView(const SpawnParams& params);

	public:
		virtual ~GPUResourceView() = default;
		// Points to the cache used by the resource for the resource visibility/usage detection. Written during rendering when resource view is used.
		double* LastRenderTime;

		/// <summary>
		/// Gets the native pointer to the underlying view. It's a platform-specific handle.
		/// </summary>
		virtual void* GetNativePtr() const = 0;
	};

	SE_CLASS(API, Abstract, NoSpawn)
	class SE_API_RUNTIME GPUResource : public ScriptingObject
	{
		SCRIPTING_TYPE_NO_SPAWN(GPUResource);
	protected:
		uint64 m_MemoryUsage = 0;
#if GPU_ENABLE_RESOURCE_NAMING
		Char* m_NamePtr = nullptr;
		int32 m_NameSize = 0, m_NameCapacity = 0;
#endif

	public:
		GPUResource();
		explicit GPUResource(const SpawnParams& params);
		~GPUResource() override;

		// Points to the cache used by the resource for the resource visibility/usage detection. Written during rendering when resource is used.
		double lastRenderTime = -1;

		/// <summary>
		/// Action fired when resource GPU state gets released. All objects and async tasks using this resource should release references to this object nor use its data.
		/// </summary>
		Action Releasing;

		virtual GPUResourceType GetResType() const = 0;

		/// <summary>
		/// Gets amount of GPU memory used by this resource (in bytes). It's a rough estimation. GPU memory may be fragmented, compressed or sub-allocated so the actual memory pressure from this resource may vary (also depends on the current graphics backend).
		/// </summary>
		uint64 GetMemoryUsage() const;

		/// <summary>
		/// Releases GPU resource data.
		/// </summary>
		void ReleaseGPU();

		virtual String ToString() const override;

		StringView GetName() const;

		void SetName(const StringView& name);

		void OnDeleteObject() override;

	protected:
		/// <summary>
		/// Releases GPU resource data (implementation).
		/// </summary>
		virtual void OnReleaseGPU();
	};


	/// <summary>
	/// 描述渲染后端的图形设备资源
	/// </summary>
	/// <remarks>
	/// DeviceType为GPU设备的类型，BaseType必须为要继承的GPUResource类型。
	/// </remarks>
	template<class DeviceType, class BaseType>
	class SE_API_RUNTIME GPUResourceBase : public BaseType
	{
	protected:
		DeviceType* m_Device;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="GPUResourceBase"/> class.
		/// </summary>
		/// <param name="device">The graphics device.</param>
		/// <param name="name">The resource name.</param>
		GPUResourceBase(DeviceType* device, const StringView& name) noexcept
			: BaseType()
			, m_Device(device)
		{
#if GPU_ENABLE_RESOURCE_NAMING
			if (name.HasChars())
				GPUResource::SetName(name);
#endif
			device->AddResource(this);
		}

		/// <summary>
		/// Finalizes an instance of the <see cref="GPUResourceBase"/> class.
		/// </summary>
		virtual ~GPUResourceBase()
		{
			if (m_Device)
				m_Device->RemoveResource(this);
		}

	public:
		/// <summary>
		/// Gets the graphics device.
		/// </summary>
		inline DeviceType* GetDevice() const
		{
			return m_Device;
		}
	};
}
