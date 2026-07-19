
#include "GraphicWindow.h"

#include "Runtime/Core/Platform/IGuiData.h"
#include "Runtime/Core/Profiler/ProfilerCPU.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRClass.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRException.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRCore.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRMethod.h"
#include "Runtime/Core/Scripting/ManagedCLR/CLRUtils.h"
#include "Runtime/Core/Scripting/Scripting.h"
#include "Runtime/Graphics/GPUDevice.h"
#include "Runtime/GRaphics/GPUSwapChain.h"
#include "Runtime/Render/RenderTask.h"
#include "Runtime/Render/2D/Render2D.h"
#include "Runtime/UI/GUI/DragData.h"
#include "Runtime/UI/GUI/WindowRootControl.h"
#include "Runtime/Utilities/Time.h"

namespace SE
{
	GraphicWindow::GraphicWindow(const CreateWindowSettings& setting) : Window(setting), m_SwapChain(nullptr), m_RenderTask(nullptr), m_GUI(nullptr)
	{
		if (!UsesManagedGui())
			m_GUI = New<WindowRootControl>(this);

		Bind();
	}

	GraphicWindow::GraphicWindow(GraphicWindow& window) : Window(window), m_SwapChain(nullptr), m_RenderTask(nullptr), m_GUI(nullptr)
	{
		if (!UsesManagedGui())
			m_GUI = New<WindowRootControl>(this);

		Bind();
	}

	bool GraphicWindow::UsesManagedGui() const
	{
		return m_Settings.Backend == CreateWindowSettings::GuiBackend::Managed;
	}

	bool GraphicWindow::EnsureManagedGui()
	{
		if (!UsesManagedGui())
			return false;
		if (m_ManagedGuiInitialized)
			return true;
		if (!Scripting::IsEveryAssemblyLoaded())
			return false;

		float dpiScale = GetDpiScale();
		Float2 logicalSize = GetClientSize() / dpiScale;
		void* params[] = { (void*)&logicalSize, (void*)&dpiScale };
		if (!InvokeManagedGui("Internal_InitializeGui", 2, params))
			return false;

		m_ManagedGuiInitialized = true;
		return true;
	}

	bool GraphicWindow::InvokeManagedGui(const char* methodName, int32 paramsCount, void** params, CLRObject** result)
	{
		if (!Scripting::IsEveryAssemblyLoaded())
			return false;

		CLRObject* managedInstance = GetOrCreateManagedInstance();
		CLRClass* managedClass = GetClass();
		if (!managedInstance || !managedClass)
			return false;

		CLRMethod* method = managedClass->GetMethod(methodName, paramsCount);
		if (!method)
		{
			LOG_ERROR("Scripting", "Missing managed GUI callback {0}.", String(methodName));
			return false;
		}

		CLRObject* exception = nullptr;
		CLRObject* invokeResult = method->Invoke(managedInstance, params, &exception);
		if (exception)
		{
			CLRException(exception).Log(Log::Severity::Error, SE_TEXT("GraphicWindow.ManagedGui"));
			return false;
		}

		if (result)
			*result = invokeResult;
		return true;
	}

	DragDropEffect GraphicWindow::InvokeManagedGuiDrag(const char* methodName, IGuiData* data, const Float2& mousePosition)
	{
		if (!EnsureManagedGui() || !data)
			return DragDropEffect::None;

		List<String> values;
		bool isText = data->GetType() == IGuiData::Type::Text;
		if (isText)
			values.Add(data->GetAsText());
		else
			data->GetAsFiles(&values);

		CLRArray* managedValues = CLRCore::Array::New(CLRCore::TypeCache::String, values.Count());
		CLRString** managedValuesPtr = CLRCore::Array::GetAddress<CLRString*>(managedValues);
		for (int32 index = 0; index < values.Count(); index++)
			managedValuesPtr[index] = CLRUtils::ToString(values[index]);

		Float2 logicalPosition = mousePosition / GetDpiScale();
		void* params[] = { (void*)&logicalPosition, (void*)&isText, managedValues };
		CLRObject* managedResult = nullptr;
		if (!InvokeManagedGui(methodName, 3, params, &managedResult) || !managedResult)
			return DragDropEffect::None;

		return static_cast<DragDropEffect>(CLRUtils::Unbox<int32>(managedResult));
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
		if (UsesManagedGui() && !EnsureManagedGui())
			return;

		Matrix3x3 scale;
		Matrix3x3::Scaling(GetDpiScale(), scale);
		Render2D::PushTransform(scale);
		if (UsesManagedGui())
			InvokeManagedGui("Internal_OnGuiDraw", 0, nullptr);
		else
			m_GUI->Draw();
		Render2D::PopTransform();
	}

	void GraphicWindow::UpdateInternal()
	{
		float deltaTime = Time::Update.UnscaledDeltaTime.GetTotalSeconds();

		if (UsesManagedGui())
		{
			if (!EnsureManagedGui())
				return;
			void* params[] = { (void*)&deltaTime };
			InvokeManagedGui("Internal_OnGuiUpdate", 1, params);
		}
		else
		{
			m_GUI->Update(deltaTime);
		}
	}

	void GraphicWindow::ResizeInternal(Float2 size)
	{
		float dpiScale = GetDpiScale();
		Float2 logicalSize(size.x / dpiScale, size.y / dpiScale);
		if (UsesManagedGui())
		{
			if (!EnsureManagedGui())
				return;
			void* params[] = { (void*)&logicalSize, (void*)&dpiScale };
			InvokeManagedGui("Internal_OnGuiResize", 2, params);
		}
		else
		{
			m_GUI->Size = logicalSize;
		}
	}

	void GraphicWindow::CharInputInternal(Char c)
	{
		if (UsesManagedGui())
		{
			if (!EnsureManagedGui())
				return;
			void* params[] = { (void*)&c };
			InvokeManagedGui("Internal_OnGuiCharInput", 1, params);
		}
		else
		{
			m_GUI->OnCharInput(c);
		}
	}

	void GraphicWindow::KeyDownInternal(KeyboardKeys key)
	{
		if (UsesManagedGui())
		{
			if (!EnsureManagedGui())
				return;
			int32 keyCode = static_cast<int32>(key);
			void* params[] = { (void*)&keyCode };
			InvokeManagedGui("Internal_OnGuiKeyDown", 1, params);
		}
		else
		{
			m_GUI->OnKeyDown(key);
		}
	}

	void GraphicWindow::KeyUpInternal(KeyboardKeys key)
	{
		if (UsesManagedGui())
		{
			if (!EnsureManagedGui())
				return;
			int32 keyCode = static_cast<int32>(key);
			void* params[] = { (void*)&keyCode };
			InvokeManagedGui("Internal_OnGuiKeyUp", 1, params);
		}
		else
		{
			m_GUI->OnKeyUp(key);
		}
	}

	void GraphicWindow::MouseDownInternal(const Float2& mousePosition, MouseButton button)
	{
		if (UsesManagedGui())
		{
			if (!EnsureManagedGui())
				return;
			Float2 logicalPosition = mousePosition / GetDpiScale();
			int32 buttonCode = static_cast<int32>(button);
			void* params[] = { (void*)&logicalPosition, (void*)&buttonCode };
			InvokeManagedGui("Internal_OnGuiMouseDown", 2, params);
		}
		else
		{
			m_GUI->OnMouseDown(mousePosition, button);
		}
	}

	void GraphicWindow::MouseUpInternal(const Float2& mousePosition, MouseButton button)
	{
		if (UsesManagedGui())
		{
			if (!EnsureManagedGui())
				return;
			Float2 logicalPosition = mousePosition / GetDpiScale();
			int32 buttonCode = static_cast<int32>(button);
			void* params[] = { (void*)&logicalPosition, (void*)&buttonCode };
			InvokeManagedGui("Internal_OnGuiMouseUp", 2, params);
		}
		else
		{
			m_GUI->OnMouseUp(mousePosition, button);
		}
	}

	void GraphicWindow::MouseDoubleClickInternal(const Float2& mousePosition, MouseButton button)
	{
		if (UsesManagedGui())
		{
			if (!EnsureManagedGui())
				return;
			Float2 logicalPosition = mousePosition / GetDpiScale();
			int32 buttonCode = static_cast<int32>(button);
			void* params[] = { (void*)&logicalPosition, (void*)&buttonCode };
			InvokeManagedGui("Internal_OnGuiMouseDoubleClick", 2, params);
		}
		else
		{
			m_GUI->OnMouseDoubleClick(mousePosition, button);
		}
	}

	void GraphicWindow::MouseWheelInternal(const Float2& mousePosition, float delta)
	{
		if (UsesManagedGui())
		{
			if (!EnsureManagedGui())
				return;
			Float2 logicalPosition = mousePosition / GetDpiScale();
			void* params[] = { (void*)&logicalPosition, (void*)&delta };
			InvokeManagedGui("Internal_OnGuiMouseWheel", 2, params);
		}
		else
		{
			m_GUI->OnMouseWheel(mousePosition, delta);
		}
	}

	void GraphicWindow::MouseMoveInternal(const Float2& mousePosition)
	{
		if (UsesManagedGui())
		{
			if (!EnsureManagedGui())
				return;
			Float2 logicalPosition = mousePosition / GetDpiScale();
			void* params[] = { (void*)&logicalPosition };
			InvokeManagedGui("Internal_OnGuiMouseMove", 1, params);
		}
		else
		{
			m_GUI->OnMouseMove(mousePosition);
		}
	}

	void GraphicWindow::MouseLeaveInternal()
	{
		if (UsesManagedGui())
		{
			if (EnsureManagedGui())
				InvokeManagedGui("Internal_OnGuiMouseLeave", 0, nullptr);
		}
		else
		{
			m_GUI->OnMouseLeave();
		}
	}

	void GraphicWindow::TouchDownInternal(const Float2& pointerPosition, int32 pointerIndex)
	{
		if (UsesManagedGui())
		{
			if (!EnsureManagedGui())
				return;
			Float2 logicalPosition = pointerPosition / GetDpiScale();
			void* params[] = { (void*)&logicalPosition, (void*)&pointerIndex };
			InvokeManagedGui("Internal_OnGuiTouchDown", 2, params);
		}
		else
		{
			m_GUI->OnTouchDown(pointerPosition, pointerIndex);
		}
	}

	void GraphicWindow::TouchMoveInternal(const Float2& pointerPosition, int32 pointerIndex)
	{
		if (UsesManagedGui())
		{
			if (!EnsureManagedGui())
				return;
			Float2 logicalPosition = pointerPosition / GetDpiScale();
			void* params[] = { (void*)&logicalPosition, (void*)&pointerIndex };
			InvokeManagedGui("Internal_OnGuiTouchMove", 2, params);
		}
		else
		{
			m_GUI->OnTouchMove(pointerPosition, pointerIndex);
		}
	}

	void GraphicWindow::TouchUpInternal(const Float2& pointerPosition, int32 pointerIndex)
	{
		if (UsesManagedGui())
		{
			if (!EnsureManagedGui())
				return;
			Float2 logicalPosition = pointerPosition / GetDpiScale();
			void* params[] = { (void*)&logicalPosition, (void*)&pointerIndex };
			InvokeManagedGui("Internal_OnGuiTouchUp", 2, params);
		}
		else
		{
			m_GUI->OnTouchUp(pointerPosition, pointerIndex);
		}
	}

	void GraphicWindow::DragEnterInternal(IGuiData* data, const Float2& mousePosition, DragDropEffect& result)
	{
		if (result != DragDropEffect::None) return;
		if (UsesManagedGui())
		{
			result = InvokeManagedGuiDrag("Internal_OnGuiDragEnter", data, mousePosition);
			return;
		}

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
		if (UsesManagedGui())
		{
			result = InvokeManagedGuiDrag("Internal_OnGuiDragMove", data, mousePosition);
			return;
		}

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
		if (UsesManagedGui())
		{
			result = InvokeManagedGuiDrag("Internal_OnGuiDragDrop", data, mousePosition);
			return;
		}

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
		if (UsesManagedGui())
		{
			if (EnsureManagedGui())
				InvokeManagedGui("Internal_OnGuiDragLeave", 0, nullptr);
		}
		else
		{
			m_GUI->OnDragLeave();
		}
	}

	void GraphicWindow::ShowInternal()
	{
		if (UsesManagedGui())
		{
			if (EnsureManagedGui())
				InvokeManagedGui("Internal_OnGuiShown", 0, nullptr);
		}
		else
		{
			m_GUI->UnlockChildrenRecursive();
			m_GUI->PerformLayout();
		}
	}

	void GraphicWindow::ClosedInternal()
	{
		if (UsesManagedGui())
		{
			if (m_ManagedGuiInitialized)
				InvokeManagedGui("Internal_DisposeGui", 0, nullptr);
			m_ManagedGuiInitialized = false;
		}
		else
		{
			Delete(m_GUI);
		}
	}
	void GraphicWindow::GotFocus()
	{
		if (UsesManagedGui() && EnsureManagedGui())
		{
			bool focused = true;
			void* params[] = { (void*)&focused };
			InvokeManagedGui("Internal_OnGuiFocusChanged", 1, params);
		}
	}
	void GraphicWindow::LostFocus()
	{
		if (UsesManagedGui() && EnsureManagedGui())
		{
			bool focused = false;
			void* params[] = { (void*)&focused };
			InvokeManagedGui("Internal_OnGuiFocusChanged", 1, params);
		}
	}
}


