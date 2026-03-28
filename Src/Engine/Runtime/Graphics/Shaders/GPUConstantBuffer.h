#pragma once

#include "Runtime/Graphics/Base/GPUResource.h"

namespace SE
{
	/// <summary>
	/// Constant Buffer object used to pass parameters to the shaders on GPU.
	/// </summary>
	class SE_API_RUNTIME GPUConstantBuffer : public GPUResource
	{
	protected:
		uint32 m_Size = 0;

	public:
		/// <summary>
		/// Gets the buffer size (in bytes).
		/// </summary>
		inline uint32 GetSize() const
		{
			return m_Size;
		}

	public:
		// [GPUResource]
		GPUResourceType GetResType() const override
		{
			return GPUResourceType::Buffer;
		}
	};
}
