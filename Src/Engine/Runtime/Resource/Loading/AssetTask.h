#pragma once

#include "Core/Thread/Task.h"
#include "Core/TypeSystem/IType.h"

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
		SE_ENUM(Result)
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