#pragma once

#include "Runtime/Core/Platform/Window.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRTypes.h"
#include "Runtime/API.h"

namespace SE
{
	class WindowRootControl;
	class RenderTask;
	class GPUSwapChain;

	class SE_API_RUNTIME GraphicWindow : public Window
	{
	public:
		FORCE_INLINE GPUSwapChain* GetSwapChain()
		{
			return m_SwapChain;
		}
		FORCE_INLINE RenderTask* GetRenderTask()
		{
			return m_RenderTask;
		}

		GraphicWindow(const CreateWindowSettings &setting);

		GraphicWindow(GraphicWindow & window);

		bool GetRenderingEnabled() const;

		void SetRenderingEnabled(bool value);

	private:
		GPUSwapChain* m_SwapChain;
		RenderTask* m_RenderTask;

	private:
		bool InitSwapChain() override;
		void ResizeSwapChain(float width, float height) override;
		void ReleaseSwapChain() override;
		void FullscreenSwapChain(bool isFullscreen) override;
	};
}
