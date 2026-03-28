#pragma once

#include "../GPUTask.h"
#include "Runtime/Graphics/GPUResourceProperty.h"

namespace SE
{
	/// <summary>
	/// GPU buffer upload task.
	/// </summary>
	class GPUUploadBufferTask final : public GPUTask
	{
	protected:
		BufferReference _buffer;
		int32 _offset;
		BytesContainer _data;

	public:
		/// <summary>
		/// Initializes a new instance of the <see cref="GPUUploadBufferTask"/> class.
		/// </summary>
		/// <param name="buffer">The target buffer.</param>
		/// <param name="offset">The target buffer offset to copy data to. The default value is 0.</param>
		/// <param name="data">The data to upload.</param>
		/// <param name="copyData">True if copy data to the temporary buffer, otherwise the input data will be used directly. Then ensure it is valid during the copy operation period (for the next few frames).</param>
		GPUUploadBufferTask(GPUBuffer* buffer, int32 offset, Span<byte> data, bool copyData);

	private:
		void OnResourceReleased();

	public:
		// [GPUTask]
		bool HasReference(void* resource) const override;

	protected:
		// [GPUTask]
		Result run(GPUTasksContext* context) override;
		void OnEnd() override;
	};
}
