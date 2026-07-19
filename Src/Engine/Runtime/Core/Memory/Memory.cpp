#include "Memory.h"

namespace SE
{
	void* PlatformAllocator::Allocate(uint64 size, uint64 alignment)
	{
		return Platform::Allocate(size, alignment);
	}

	void* PlatformAllocator::Realloc(void* ptr, uint64 newSize)
	{
		if (newSize == 0)
		{
			Free(ptr);
			return nullptr;
		}
		if (!ptr)
			return Allocate(newSize);
		void* result = Allocate(newSize);
		if (result)
		{
			Platform::MemoryCopy(result, ptr, newSize);
			Free(ptr);
		}
		return result;
	}

	void* PlatformAllocator::ReallocAligned(void* ptr, uint64 newSize, uint64 alignment)
	{
		if (newSize == 0)
		{
			Free(ptr);
			return nullptr;
		}
		if (!ptr)
			return Allocate(newSize, alignment);
		void* result = Allocate(newSize, alignment);
		if (result)
		{
			Platform::MemoryCopy(result, ptr, newSize);
			Free(ptr);
		}
		return result;
	}

	void* PlatformAllocator::Realloc(void* ptr, uint64 oldSize, uint64 newSize)
	{
		if (newSize == 0)
		{
			Free(ptr);
			return nullptr;
		}
		if (!ptr)
			return Allocate(newSize);
		if (newSize <= oldSize)
			return ptr;
		void* result = Allocate(newSize);
		if (result)
		{
			Platform::MemoryCopy(result, ptr, oldSize);
			Free(ptr);
		}
		return result;
	}

	void PlatformAllocator::Free(void* ptr)
	{
		Platform::Free(ptr);
	}

	const Char* PlatformAllocator::Name()
	{
		return SE_TEXT("Crt");
	}

}
