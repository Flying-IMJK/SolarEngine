#include "Runtime/Core/Platform/WindowsManager.h"

#include "Runtime/Core/Systems.h"
#include "Runtime/Core/Profiler/ProfilerCPU.h"

namespace SE
{
	CriticalSection WindowsManager::WindowsLocker;
	List<Window*> WindowsManager::Windows;

	Window* WindowsManager::GetByNativePtr(void* handle)
	{
		Window* result = nullptr;
		WindowsLocker.Lock();
		for (int32 i = 0; i < Windows.Count(); i++)
		{
			if (Windows[i]->GetNativePtr() == handle)
			{
				result = Windows[i];
				break;
			}
		}
		WindowsLocker.Unlock();
		return result;
	}

	void WindowsManager::Register(Window* win)
	{
		WindowsLocker.Lock();
		Windows.Add(win);
		WindowsLocker.Unlock();
	}

	void WindowsManager::Unregister(Window* win)
	{
		WindowsLocker.Lock();
		Windows.Remove(win);
		WindowsLocker.Unlock();
	}

	Window* WindowsManager::GetForegroundWindow()
	{
		Window* foregroundWindow = nullptr;
		WindowsLocker.Lock();
		for (int i = 0; i < Windows.Count(); i++)
		{
			Window* window = Windows[i];
			if (window->IsForegroundWindow())
			{
				foregroundWindow = window;
				break;
			}
		}
		WindowsLocker.Unlock();
		return foregroundWindow;
	}

	class WindowsManagerSystem : public ISystem
	{
		ENGINE_SYSTEM(WindowsManagerSystem)
	public:

		WindowsManagerSystem()
			: ISystem(SE_TEXT("Windows Manager"), -30)
		{
		}

		void OnUpdate() override;
		void OnDispose() override;
	};

	ENGINE_SYSTEM_REGISTER(WindowsManagerSystem);

	void WindowsManagerSystem::OnUpdate()
	{
		PROFILE_CPU();

		// Update windows
		WindowsManager::WindowsLocker.Lock();
		List<Window*, InlinedAllocation<32>> windows;
		windows.Add(WindowsManager::Windows);
		for (Window* win : windows)
		{
			if (win->IsVisible())
				win->OnUpdate();
		}
		WindowsManager::WindowsLocker.Unlock();
	}

	void WindowsManagerSystem::OnDispose()
	{
		// Close remaining windows
		WindowsManager::WindowsLocker.Lock();
		List<Window*, InlinedAllocation<32>> windows;
		windows.Add(WindowsManager::Windows);
		for (Window* win : windows)
		{
			win->Close(ClosingReason::EngineExit);
		}
		WindowsManager::WindowsLocker.Unlock();
	}
}
