#pragma once

#include "Core/Types/Object.h"
#include "Core/Utilities/Singleton.hpp"
#include "Runtime/API.h"

namespace SE
{
	/// <summary>
	/// Base class for renderer components called render pass.
	/// Each render pass supports proper resources initialization and disposing.
	/// </summary>
	/// <seealso cref="Object" />
	class SE_API_RUNTIME RendererPassBase : public Object
	{
	protected:

		bool m_HasValidResources;

		/// <summary>
		/// Init
		/// </summary>
		RendererPassBase()
		{
			m_HasValidResources = false;
		}

	public:

		/// <summary>
		/// Initialize service.
		/// </summary>
		virtual bool Init()
		{
			return true;
		}

		/// <summary>
		/// Cleanup service data.
		/// </summary>
		virtual void Dispose()
		{
			// Clear flag
			m_HasValidResources = false;
		}

		/// <summary>
		/// Determines whether can render this pass. Checks if pass is ready and has valid resources loaded.
		/// </summary>
		/// <returns><c>true</c> if can render pass; otherwise, <c>false</c>.</returns>
		bool IsReady()
		{
			return CheckIfSkipPass();
		}

	protected:

		bool CheckIfSkipPass()
		{
			if (m_HasValidResources)
				return false;

			const bool setupSuccess = SetupResources();
			m_HasValidResources = !setupSuccess;
			return setupSuccess;
		}

		void InvalidateResources()
		{
			// Clear flag
			m_HasValidResources = false;
		}

		virtual bool SetupResources()
		{
			return false;
		}
	};

	template<class T>
	class RendererPass : public AutoSingleton<T>, public RendererPassBase
	{
	};

	#define REPORT_INVALID_SHADER_PASS_CB_SIZE(shader, index, dataType) LOG_FATAL("Render", "Shader {0} has incorrect constant buffer {1} size: {2} bytes. Expected: {3} bytes", shader->ToString(), index, shader->GetCB(index)->GetSize(), sizeof(dataType));
}
