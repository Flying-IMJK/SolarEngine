#include "Runtime/Core/Systems.h"
#include "Runtime/Core/Types/Collections/Sorting.h"
#include "Runtime/Core/Types/Strings/StringView.h"
#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/Profiler/Profiler.h"
#include "Runtime/Core/Types/Strings/StringConverter.h"

//-------------------------------------------------------------------------

namespace SE
{
	ISystem::ISystem(const Char* name, int32 order) : name(name), order(order)
	{

	}

	static bool G_CompareSystem(ISystem* const& a, ISystem* const& b)
	{
		return a->order < b->order;
	}

	List<ISystem*> Systems::m_Systems;
	Dictionary<uint32, Systems::SystemInfo> Systems::m_SystemCache;

	void Systems::ShutDown()
	{
		ZoneScoped;

		StringBuilder stringBuilder;

		// Dispose system from back to front
		auto& systemList = m_Systems;
		for (int32 i = systemList.Count() - 1; i >= 0; i--)
		{
			const auto pSystem = systemList[i]; 
            if (pSystem->IsInitialized)
			{
#if SE_PROFILER
				ZoneScoped;

				stringBuilder.Clear();
				stringBuilder.Append(pSystem->name);
				stringBuilder.Append(SE_TEXT("::OnDispose"));

				ZoneName(stringBuilder.ToStringAnsi().Get(), stringBuilder.Length());
#endif
				pSystem->IsInitialized = false;
				pSystem->OnDispose();
			}
		}
	}

	void Systems::Initialize()
	{
		ZoneScoped;

		auto& systemList = m_Systems;

		m_Systems.Clear();

		for (auto& item : m_SystemCache)
		{
			Systems::SystemInfo& info = item.Value;
			// 系统没有创建
			if (info.systemInstance == nullptr)
			{
				if (info.systemRegister == nullptr)
				{
					LOG_ERROR("System", "Create System {0} failed, SystemRegister is null");
					continue;
				}
				info.systemInstance = info.systemRegister->Create();
			}

			m_Systems.Add(info.systemInstance);
		}

		Sort();

		// Init system from front to back
		for (int32 i = 0; i < systemList.Count(); i++)
		{
			const auto pSystem = systemList[i];
			const StringView name(pSystem->name);

#if SE_PROFILER
			ZoneScoped;
			int32 nameBufferLength = 0;
			Char nameBuffer[100];
			for (int32 j = 0; j < name.Length(); j++)
			{
				if (name[j] != ' ')
				{
					nameBuffer[nameBufferLength++] = name[j];
				}
			}
			Platform::MemoryCopy(nameBuffer + nameBufferLength, SE_TEXT("::OnInit"), 7 * sizeof(Char));
			nameBufferLength += 7;
			//ZoneName(nameBuffer, nameBufferLength);
#endif

			LOG_INFO("System", "Initialize {0}", name);
			pSystem->IsInitialized = true;
			if (!pSystem->OnInit())
			{
				Platform::Fatal(String::Format(SE_TEXT("Failed to initialize {0}."), name));
			}

		}

		LOG_INFO("System", "Engine System are ready!");
	}

	void Systems::Sort()
	{
		auto& systemList = m_Systems;
		Function<bool(ISystem* const&, ISystem* const&)> sortCall = G_CompareSystem;
		Sorting::QuickSort(systemList.Get(), systemList.Count(), sortCall);
	}

	void Systems::Update()
	{
		auto& systemList = m_Systems;
		for (int32 i = 0; i < systemList.Count(); i++)
		{
			const auto pSystem = systemList[i];
			pSystem->OnUpdate();
		}
	}

	void Systems::LateUpdate()
	{
		auto& systemList = m_Systems;
		for (int32 i = 0; i < systemList.Count(); i++)
		{
			const auto pSystem = systemList[i];
			pSystem->OnLateUpdate();
		}
	}

	void Systems::Render()
	{
		auto& systemList = m_Systems;
		for (int32 i = 0; i < systemList.Count(); i++)
		{
			const auto pSystem = systemList[i];
			pSystem->OnRender();
		}
	}

	void Systems::Register(StringView name, SystemRegister* pRegister)
	{
		ENGINE_ASSERT(pRegister != nullptr && !m_SystemCache.ContainsKey(pRegister->GetID()));

		ISystem* system = nullptr;
		if (m_Systems.Count() > 0 && m_Systems.First()->IsInitialized)
		{
			system = pRegister->Create();
			m_Systems.Add(system);
			Sort();
		}

		Systems::SystemInfo systemInfo;
		systemInfo.systemRegister = pRegister;
		systemInfo.systemInstance = system;
		systemInfo.name = name;

		m_SystemCache.Add(pRegister->GetID(), systemInfo);
	}

	void Systems::Unregister(SystemRegister* pRegister)
	{
		ENGINE_ASSERT(pRegister != nullptr);
		Systems::SystemInfo item;
		if (m_SystemCache.TryGet(pRegister->GetID(), item))
		{
			if (item.systemInstance)
			{
				m_Systems.Remove(item.systemInstance);
				item.systemInstance->OnDispose();
			}

			m_SystemCache.Remove(pRegister->GetID());
		}
	}

}