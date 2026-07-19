#include "ThreadRegistry.h"
#include "Runtime/Core/Types/Collections/Dictionary.h"
#include "Runtime/Core/Platform/CriticalSection.h"

namespace SE::Threading
{
	Dictionary<uint64, Thread*> registry(64);
	CriticalSection locker;

	Thread* ThreadRegistry::GetThread(uint64 id)
	{
		Thread* result = nullptr;

		locker.Lock();
		registry.TryGet(id, result);
		locker.Unlock();

		return result;
	}

	int32 ThreadRegistry::Count()
	{
		locker.Lock();
		int32 count = registry.Count();
		locker.Unlock();

		return count;
	}

	void ThreadRegistry::KillEmAll()
	{
		locker.Lock();
		for (auto i = registry.begin(); i.IsNotEnd(); ++i)
		{
			i->Value->Kill(false);
		}
		locker.Unlock();

		// Now album Kill'Em All from Metallica...
	}

	void ThreadRegistry::Add(Thread* thread)
	{
		ENGINE_ASSERT(thread && thread->GetID() != 0);
		locker.Lock();
		ENGINE_ASSERT(!registry.ContainsKey(thread->GetID()) && !registry.ContainsValue(thread));
		registry.Add(thread->GetID(), thread);
		locker.Unlock();
	}

	void ThreadRegistry::Remove(Thread* thread)
	{
		ENGINE_ASSERT(thread && thread->GetID() != 0);
		locker.Lock();
#if ENABLE_ASSERTION_LOW_LAYERS
		Thread** currentValue = Registry.TryGet(thread->GetID());
    	ASSERT_LOW_LAYER(!currentValue || *currentValue == thread);
#endif
		registry.Remove(thread->GetID());
		locker.Unlock();
	}

}