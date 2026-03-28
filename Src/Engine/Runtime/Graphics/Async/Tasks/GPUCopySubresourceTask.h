#pragma once
#include "Runtime/Graphics/GPUResourceProperty.h"
#include "Runtime/Graphics/Async/GPUTask.h"

namespace SE
{
	/// <summary>
	/// GPU subresource copy task.
	/// </summary>
	class GPUCopySubresourceTask final : public GPUTask
	{
	private:
		GPUResourceReference m_SrcResource;
		GPUResourceReference m_DstResource;
		uint32 m_SrcSubresource, m_DstSubresource;

	public:
		/// <summary>
		/// Init
		/// </summary>
		/// <param name="src">The source resource.</param>
		/// <param name="dst">The destination resource.</param>
		/// <param name="srcSubresource">The source subresource index.</param>
		/// <param name="dstSubresource">The destination subresource index.</param>
		GPUCopySubresourceTask(GPUResource* src, GPUResource* dst, uint32 srcSubresource, uint32 dstSubresource)
			: GPUTask(Type::CopyResource)
			, m_SrcResource(src)
			, m_DstResource(dst)
			, m_SrcSubresource(srcSubresource)
			, m_DstSubresource(dstSubresource)
		{
			m_SrcResource.Released.Bind<GPUCopySubresourceTask, &GPUCopySubresourceTask::OnResourceReleased>(this);
			m_DstResource.Released.Bind<GPUCopySubresourceTask, &GPUCopySubresourceTask::OnResourceReleased>(this);
		}

	private:
		void OnResourceReleased()
		{
			Cancel();
		}

	public:
		// [GPUTask]
		bool HasReference(void* resource) const override
		{
			return m_SrcResource == resource || m_DstResource == resource;
		}

	protected:
		// [GPUTask]
		Result run(GPUTasksContext* context) override
		{
			if (!m_SrcResource || !m_DstResource)
				return Result::MissingResources;
			context->GPU->CopySubresource(m_DstResource, m_DstSubresource, m_SrcResource, m_SrcSubresource);
			return Result::Ok;
		}

		void OnEnd() override
		{
			m_SrcResource.Unlink();
			m_DstResource.Unlink();

			// Base
			GPUTask::OnEnd();
		}
	};

}
