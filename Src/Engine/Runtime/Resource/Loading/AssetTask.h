#pragma once

#include "Runtime/Core/Thread/Task.h"
#include "Runtime/Core/TypeSystem/IType.h"

namespace SE
{
	/// <summary>
	/// 资源加载Task
	/// </summary>
	class AssetTask : public Threading::Task
	{
		friend class AssetLoadingThread;

	public:
		/// <summary>
		/// Describes work result value
		/// </summary>
		SE_ENUM(Reflect)
		enum class Result
		{
			Ok,
			AssetLoadError,
			MissingReferences,
			LoadDataError,
			TaskFailed
		};

		// [Task]
		String ToString() const override;

	protected:
		virtual Result Process() = 0;

		// [Task]
		void Enqueue() override;
		bool Run() override;
	};
}