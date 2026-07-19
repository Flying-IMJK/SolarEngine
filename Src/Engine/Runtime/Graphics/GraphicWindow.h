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

		FORCE_INLINE WindowRootControl* GetGUI()
		{
			return m_GUI;
		}

		/// <summary>
		/// Returns true when this window owns a managed C# GUI tree instead of the native compatibility tree.
		/// </summary>
		bool UsesManagedGui() const;

		GraphicWindow(const CreateWindowSettings &setting);

		GraphicWindow(GraphicWindow & window);

		bool GetRenderingEnabled() const;

		void SetRenderingEnabled(bool value);

	private:
		GPUSwapChain* m_SwapChain;
		RenderTask* m_RenderTask;
		/// <summary>
		/// The window GUI root object.
		/// </summary>
		WindowRootControl* m_GUI;
		bool m_ManagedGuiInitialized = false;

	private:
		bool InitSwapChain() override;
		void ResizeSwapChain(float width, float height) override;
		void ReleaseSwapChain() override;
		void FullscreenSwapChain(bool isFullscreen) override;

		void Bind();
		bool EnsureManagedGui();
		bool InvokeManagedGui(const char* methodName, int32 paramsCount, void** params, CLRObject** result = nullptr);
		DragDropEffect InvokeManagedGuiDrag(const char* methodName, IGuiData* data, const Float2& mousePosition);

		void DrawInternal();
		void UpdateInternal();
		void ResizeInternal(Float2 size);
		void CharInputInternal(Char c);
		void KeyDownInternal(KeyboardKeys key);
		void KeyUpInternal(KeyboardKeys key);
		void MouseDownInternal(const Float2& mousePosition, MouseButton button);
		void MouseUpInternal(const Float2& mousePosition, MouseButton button);
		void MouseDoubleClickInternal(const Float2& mousePosition, MouseButton button);
		void MouseWheelInternal(const Float2& mousePosition, float delta);
		void MouseMoveInternal(const Float2& mousePosition);
		void MouseLeaveInternal();
		void TouchDownInternal(const Float2& pointerPosition, int32 pointerIndex);
		void TouchMoveInternal(const Float2& pointerPosition, int32 pointerIndex);
		void TouchUpInternal(const Float2& pointerPosition, int32 pointerIndex);
		void DragEnterInternal(IGuiData* data, const Float2& mousePosition, DragDropEffect& result);
		void DragOverInternal(IGuiData* data, const Float2& mousePosition, DragDropEffect& result);
		void DragDropInternal(IGuiData* data, const Float2& mousePosition, DragDropEffect& result);
		void DragLeaveInternal();
		void ShowInternal();
		void ClosedInternal();

		void GotFocus();
		void LostFocus();

	};
}
