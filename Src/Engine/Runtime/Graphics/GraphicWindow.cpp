
#include "GraphicWindow.h"

#include <Runtime/Core/Platform/IGuiData.h>
#include <Runtime/Core/Profiler/ProfilerCPU.h>
#include <Runtime/Core/Scripting/ManagedCLR/CLRClass.h>
#include <Runtime/Core/Scripting/ManagedCLR/CLRException.h>
#include <Runtime/Core/Scripting/ManagedCLR/CLRCore.h>
#include <Runtime/Core/Scripting/ManagedCLR/CLRMethod.h>
#include <Runtime/Core/Scripting/ManagedCLR/CLRUtils.h>
#include <Runtime/Core/Scripting/Scripting.h>
#include <Runtime/Graphics/GPUDevice.h>
#include <Runtime/Graphics/GPUSwapChain.h>
#include <Runtime/Render/RenderTask.h>
#include <Runtime/Utilities/Time.h>

namespace SE
{
	GraphicWindow::GraphicWindow(const CreateWindowSettings& setting) : Window(setting), m_SwapChain(nullptr), m_RenderTask(nullptr)
	{

	}

	GraphicWindow::GraphicWindow(GraphicWindow& window) : Window(window), m_SwapChain(nullptr), m_RenderTask(nullptr)
	{

	}

	
	bool GraphicWindow::GetRenderingEnabled() const
	{
		return m_RenderTask != nullptr && m_RenderTask->Enabled;
	}

	void GraphicWindow::SetRenderingEnabled(bool value)
	{
		if (m_RenderTask != nullptr)
			m_RenderTask->Enabled = value;
	}

	bool GraphicWindow::InitSwapChain()
	{
		if (m_SwapChain == nullptr)
		{
			m_SwapChain = GPUDevice::instance->CreateSwapChain((Window*)this);
			if (!m_SwapChain)
				return false;
		}
		if (!m_SwapChain->Resize(static_cast<int32>(m_ClientSize.X), static_cast<int32>(m_ClientSize.Y)))
			return false;

		if (m_Settings.Fullscreen)
			m_SwapChain->SetFullscreen(true);

		// Setup render task
		if (m_RenderTask == nullptr)
		{
#if !SE_EDITOR
			if (IsMain())
			{
				// Override main task output (render directly to the window backbuffer)
				ENGINE_ASSERT(MainRenderTask::Instance);
				ENGINE_ASSERT(MainRenderTask::Instance->SwapChain == nullptr);
				RenderTask* = MainRenderTask::Instance;
//				MainRenderTask::Instance->Deleted.Bind<WindowBase, &WindowBase::OnMainRenderTaskDelete>(this);
			}
			else
#endif
			{
				m_RenderTask = New<RenderTask>();
			}
			m_RenderTask->SwapChain = m_SwapChain;
			m_RenderTask->Enabled = true;
			m_RenderTask->Order = 100;
		}

		return true;
	}

	void GraphicWindow::ResizeSwapChain(float width, float height)
	{
		if (m_SwapChain)
		{
			m_SwapChain->Resize(width, height);
		}
	}
	void GraphicWindow::ReleaseSwapChain()
	{
		DeleteObjectSafe(m_SwapChain);
		DeleteObjectSafe(m_RenderTask);
	}

	void GraphicWindow::FullscreenSwapChain(bool isFullscreen)
	{
		if (m_SwapChain)
		{
			m_SwapChain->SetFullscreen(isFullscreen);
		}
	}
}


