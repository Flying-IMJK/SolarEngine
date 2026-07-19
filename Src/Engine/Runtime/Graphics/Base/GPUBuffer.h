#pragma once

#include "GPUBufferDescription.h"
#include "Runtime/Core/Thread/Threading.h"
#include "Runtime/Core/Types/Collections/DataContainer.h"

namespace SE
{

	/// <summary>
	/// Defines a view for the <see cref="GPUBuffer"/>. Used to bind buffer to the shaders (for input as shader resource or for input/output as unordered access).
	/// </summary>
	SE_CLASS(API, Sealed, NoSpawn)
	class SE_API_RUNTIME GPUBufferView : public GPUResourceView
	{
		SCRIPTING_TYPE_NO_SPAWN(GPUBufferView);

	protected:
		GPUBufferView();
	};

	SE_CLASS(API, Sealed)
	class SE_API_RUNTIME GPUBuffer : public GPUResource
	{
		SCRIPTING_TYPE_NO_SPAWN(GPUBuffer);
		static GPUBuffer* Spawn(const SpawnParams& params);
		static GPUBuffer* New();

	protected:
		GPUBufferDescription m_Desc;
		GPUBuffer();

	public:
		inline bool IsAllocated() const
		{
			return m_Desc.Size > 0;
		}

		inline uint32 GetSize() const
		{
			return m_Desc.Size;
		}

		inline uint32 GetStride() const
		{
			return m_Desc.Stride;
		}

		inline PixelFormat GetFormat() const
		{
			return m_Desc.Format;
		}

		inline uint32 GetElementsCount() const
		{
			ENGINE_ASSERT(m_Desc.Stride > 0);
			return m_Desc.Size / m_Desc.Stride;
		}

		inline EnumFlags<GPUBufferFlags> GetFlags() const
		{
			return m_Desc.Flags;
		}

		/// <summary>
		/// Checks if buffer is a staging buffer (supports CPU readback).
		/// </summary>
		inline bool IsStaging() const
		{
			return m_Desc.Usage == GPUResourceUsage::StagingReadback || m_Desc.Usage == GPUResourceUsage::StagingUpload;
		}

		/// <summary>
		/// Checks if buffer is a staging buffer (supports CPU readback).
		/// </summary>
		inline bool IsDynamic() const
		{
			return m_Desc.Usage == GPUResourceUsage::Dynamic;
		}

		/// <summary>
		/// Gets a value indicating whether this buffer is a shader resource.
		/// </summary>
		inline bool IsShaderResource() const
		{
			return m_Desc.IsShaderResource();
		}

		/// <summary>
		/// Gets a value indicating whether this buffer is a unordered access.
		/// </summary>
		inline bool IsUnorderedAccess() const
		{
			return m_Desc.IsUnorderedAccess();
		}
		
		const GPUBufferDescription& GetDescription() const
		{
			return m_Desc;
		}

	public:

		bool Init(const GPUBufferDescription& desc);

		/// <summary>
		/// Tries to resize the buffer (warning: contents will be lost).
		/// </summary>
		/// <param name="newSize">The new size (in bytes).</param>
		/// <returns>True if cannot resize buffer, otherwise false.</returns>
		bool Resize(uint32 newSize);

		/// <summary>
		/// Creates new staging readback buffer with the same dimensions and properties as a source buffer (but without a data transferred; warning: caller must delete object).
		/// </summary>
		/// <returns>Staging readback buffer.</returns>
		GPUBuffer* ToStagingReadback() const;

		/// <summary>
		/// Creates new staging upload buffer with the same dimensions and properties as a source buffer (but without a data transferred; warning: caller must delete object).
		/// </summary>
		/// <returns>Staging upload buffer.</returns>
		GPUBuffer* ToStagingUpload() const;

		/// <summary>
		/// 通过map/memcpy/unmap顺序设置缓冲区数据。始终支持动态缓冲区(其他类型的支持取决于图形后端实现)
		/// </summary>
		/// <param name="data">设置数据</param>
		/// <param name="size">数据大小 (字节).</param>
		void SetData(const void* data, uint32 size);

		bool GetData(BytesContainer& output);

		/// <summary>
		/// Stops current thread execution to gather buffer data from the GPU. Cannot be called from main thread if the buffer is not a dynamic nor staging readback.
		/// </summary>
		/// <param name="result">The result data.</param>
		/// <returns>True if cannot download data, otherwise false.</returns>
		bool DownloadData(BytesContainer& result);

		/// <summary>
		/// Creates GPU async task that will gather buffer data from the GPU.
		/// </summary>
		/// <param name="result">The result data.</param>
		/// <returns>Download data task (not started yet).</returns>
		Threading::Task* DownloadDataAsync(BytesContainer& result);

		/// <summary>
		/// Gets the view for the whole buffer.
		/// </summary>
		virtual GPUBufferView* View() const = 0;

		/// <summary>
		/// 通过映射资源的内容获取指向资源的CPU指针。拒绝GPU访问该资源。
		/// </summary>
		/// <remarks>获取的指针后，需要调用Unmap来释放资源。</remarks>
		/// <param name="mode">The map operation mode.</param>
		/// <returns>GPU数据映射CPU缓冲区的指针，如果失败则为空。</returns>
		virtual void* Map(GPUResourceMapMode mode) = 0;

		/// <summary>
		/// 使指向资源的映射指针失效，并恢复GPU对该资源的访问。
		/// </summary>
		virtual void Unmap() = 0;

	public:
		GPUResourceType GetResType() const final override;

	protected:
		virtual bool OnInit() = 0;
	};
}
