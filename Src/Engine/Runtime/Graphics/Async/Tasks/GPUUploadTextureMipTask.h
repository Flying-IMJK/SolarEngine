#pragma once

#include "Runtime/Graphics/Async/GPUTask.h"
#include "Runtime/Core/Types/Collections/DataContainer.h"
#include "Runtime/Graphics/GPUResourceProperty.h"

namespace SE
{
	class GPUTexture;

	/// <summary>
	/// GPU texture mip upload task.
	/// </summary>
	class GPUUploadTextureMipTask : public GPUTask
	{
	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="GPUUploadTextureMipTask"/> class.
		/// </summary>
		/// <param name="texture">The target texture.</param>
		/// <param name="mipIndex">The target texture mip data.</param>
		/// <param name="data">The data to upload.</param>
		/// <param name="rowPitch">The data row pitch.</param>
		/// <param name="slicePitch">The data slice pitch.</param>
		/// <param name="copyData">True if copy data to the temporary buffer, otherwise the input data will be used directly. Then ensure it is valid during the copy operation period (for the next few frames).</param>
		GPUUploadTextureMipTask(GPUTexture* texture, int32 mipIndex, Span<byte> data, int32 rowPitch, int32 slicePitch, bool copyData);

		// [GPUTask]
		bool HasReference(void* resource) const override;

	private:
		void OnResourceReleased()
		{
			Cancel();
		}

	protected:
		// [GPUTask]
		Result run(GPUTasksContext* context) override;
		void OnSync() override;
		void OnEnd() override;

		GPUTextureReference m_Texture;
		int32 _mipIndex, _rowPitch, _slicePitch;
		BytesContainer _data;
	};
}