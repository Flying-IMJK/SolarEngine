
#include "GraphicWindow.h"

#include "Core/Platform/IGuiData.h"
#include "Core/Profiler/ProfilerCPU.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/GRaphics/GPUSwapChain.h"
#include "Runtime/Render/RenderTask.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/DragData.h"
#include "Runtime/UI/GUI/WindowRootControl.h"
#include "Runtime/Utilities/Time.h"

namespace SE
{
	GraphicWindow::GraphicWindow(const CreateWindowSettings& setting) : Window(setting), m_SwapChain(nullptr), m_RenderTask(nullptr)
	{
		m_GUI = New<WindowRootControl>(this);

		Bind();
	}

	GraphicWindow::GraphicWindow(GraphicWindow& window) : Window(window), m_SwapChain(nullptr), m_RenderTask(nullptr)
	{
		m_GUI = New<WindowRootControl>(this);

		Bind();
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
		if (!m_SwapChain->Resize(static_cast<int32>(m_ClientSize.x), static_cast<int32>(m_ClientSize.y)))
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
				ASSERT(MainRenderTask::Instance);
				ASSERT(MainRenderTask::Instance->SwapChain == nullptr);
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

	void GraphicWindow::Bind()
	{
		DrawEvent.BindUnique<GraphicWindow, &GraphicWindow::DrawInternal>(this);
		UpdateEvent.BindUnique<GraphicWindow, &GraphicWindow::UpdateInternal>(this);
		ResizedEvent.BindUnique<GraphicWindow, &GraphicWindow::ResizeInternal>(this);
		CharInputEvent.BindUnique<GraphicWindow, &GraphicWindow::CharInputInternal>(this);
		KeyDownEvent.BindUnique<GraphicWindow, &GraphicWindow::KeyDownInternal>(this);
		KeyUpEvent.BindUnique<GraphicWindow, &GraphicWindow::KeyUpInternal>(this);
		MouseDownEvent.BindUnique<GraphicWindow, &GraphicWindow::MouseDownInternal>(this);
		MouseUpEvent.BindUnique<GraphicWindow, &GraphicWindow::MouseUpInternal>(this);
		MouseDoubleClickEvent.BindUnique<GraphicWindow, &GraphicWindow::MouseDoubleClickInternal>(this);
		MouseWheelEvent.BindUnique<GraphicWindow, &GraphicWindow::MouseWheelInternal>(this);
		MouseMoveEvent.BindUnique<GraphicWindow, &GraphicWindow::MouseMoveInternal>(this);
		MouseLeaveEvent.BindUnique<GraphicWindow, &GraphicWindow::MouseLeaveInternal>(this);
		TouchDownEvent.BindUnique<GraphicWindow, &GraphicWindow::TouchDownInternal>(this);
		TouchUpEvent.BindUnique<GraphicWindow, &GraphicWindow::TouchUpInternal>(this);
		TouchMoveEvent.BindUnique<GraphicWindow, &GraphicWindow::TouchMoveInternal>(this);
		DragEnterEvent.BindUnique<GraphicWindow, &GraphicWindow::DragEnterInternal>(this);
		DragOverEvent.BindUnique<GraphicWindow, &GraphicWindow::DragOverInternal>(this);
		DragDropEvent.BindUnique<GraphicWindow, &GraphicWindow::DragDropInternal>(this);
		DragLeaveEvent.BindUnique<GraphicWindow, &GraphicWindow::DragLeaveInternal>(this);
		ClosedEvent.BindUnique<GraphicWindow, &GraphicWindow::ClosedInternal>(this);
		ShownEvent.BindUnique<GraphicWindow, &GraphicWindow::ShowInternal>(this);
		GetFocusEvent.BindUnique<GraphicWindow, &GraphicWindow::GotFocus>(this);
		LostFocusEvent.BindUnique<GraphicWindow, &GraphicWindow::LostFocus>(this);
	}

	void GraphicWindow::DrawInternal()
	{
		Matrix3x3 scale;
		Matrix3x3::Scaling(GetDpiScale(), scale);
		Render2D::PushTransform(scale);
		m_GUI->Draw();
		Render2D::PopTransform();
	}

	void GraphicWindow::UpdateInternal()
	{
		const float deltaTime = Time::Update.UnscaledDeltaTime.GetTotalSeconds();

		m_GUI->Update(deltaTime);
	}

	void GraphicWindow::ResizeInternal(Float2 size)
	{
		float dpiScale = GetDpiScale();
		m_GUI->Size = Float2(size.x / dpiScale, size.y / dpiScale);
	}

	void GraphicWindow::CharInputInternal(Char c)
	{
		m_GUI->OnCharInput(c);
	}

	void GraphicWindow::KeyDownInternal(KeyboardKeys key)
	{
		m_GUI->OnKeyDown(key);
	}

	void GraphicWindow::KeyUpInternal(KeyboardKeys key)
	{
		m_GUI->OnKeyUp(key);
	}

	void GraphicWindow::MouseDownInternal(const Float2& mousePosition, MouseButton button)
	{
		m_GUI->OnMouseDown(mousePosition, button);
	}

	void GraphicWindow::MouseUpInternal(const Float2& mousePosition, MouseButton button)
	{
		m_GUI->OnMouseUp(mousePosition, button);
	}

	void GraphicWindow::MouseDoubleClickInternal(const Float2& mousePosition, MouseButton button)
	{
		m_GUI->OnMouseDoubleClick(mousePosition, button);
	}

	void GraphicWindow::MouseWheelInternal(const Float2& mousePosition, float delta)
	{
		m_GUI->OnMouseWheel(mousePosition, delta);
	}

	void GraphicWindow::MouseMoveInternal(const Float2& mousePosition)
	{
		m_GUI->OnMouseMove(mousePosition);
	}

	void GraphicWindow::MouseLeaveInternal()
	{
		m_GUI->OnMouseLeave();
	}

	void GraphicWindow::TouchDownInternal(const Float2& pointerPosition, int32 pointerIndex)
	{
		m_GUI->OnTouchDown(pointerPosition, pointerIndex);
	}

	void GraphicWindow::TouchMoveInternal(const Float2& pointerPosition, int32 pointerIndex)
	{
		m_GUI->OnTouchMove(pointerPosition, pointerIndex);
	}

	void GraphicWindow::TouchUpInternal(const Float2& pointerPosition, int32 pointerIndex)
	{
		m_GUI->OnTouchUp(pointerPosition, pointerIndex);
	}

	void GraphicWindow::DragEnterInternal(IGuiData* data, const Float2& mousePosition, DragDropEffect& result)
	{
		if (result != DragDropEffect::None) return;

		if (data->GetType() == IGuiData::Type::Text)
		{
			DragDataText dataText(data->GetAsText());

			result = m_GUI->OnDragEnter(mousePosition, &dataText);
		}
		else
		{
			DragDataFiles fileDragData;
			data->GetAsFiles(&fileDragData.Files);

			result = m_GUI->OnDragEnter(mousePosition, &fileDragData);
		}
	}

	void GraphicWindow::DragOverInternal(IGuiData* data, const Float2& mousePosition, DragDropEffect& result)
	{
		if (result != DragDropEffect::None) return;

		if (data->GetType() == IGuiData::Type::Text)
		{
			DragDataText dataText(data->GetAsText());

			result = m_GUI->OnDragMove(mousePosition, &dataText);
		}
		else
		{
			DragDataFiles fileDragData;
			data->GetAsFiles(&fileDragData.Files);

			result = m_GUI->OnDragMove(mousePosition, &fileDragData);
		}
	}

	void GraphicWindow::DragDropInternal(IGuiData* data, const Float2& mousePosition, DragDropEffect& result)
	{
		if (result != DragDropEffect::None) return;

		if (data->GetType() == IGuiData::Type::Text)
		{
			DragDataText dataText(data->GetAsText());

			result = m_GUI->OnDragDrop(mousePosition, &dataText);
		}
		else
		{
			DragDataFiles fileDragData;
			data->GetAsFiles(&fileDragData.Files);

			result = m_GUI->OnDragDrop(mousePosition, &fileDragData);
		}
	}

	void GraphicWindow::DragLeaveInternal()
	{
		m_GUI->OnDragLeave();
	}

	void GraphicWindow::ShowInternal()
	{
		m_GUI->UnlockChildrenRecursive();
		m_GUI->PerformLayout();
	}

	void GraphicWindow::ClosedInternal()
	{
		Delete(m_GUI);
	}
	void GraphicWindow::GotFocus()
	{
	}
	void GraphicWindow::LostFocus()
	{
	}
}


