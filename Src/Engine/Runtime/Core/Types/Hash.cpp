#include "Hash.h"
#include "Collections/List.h"
#define XXH_INLINE_ALL
#include "../ThirdParty/xxhash/xxhash.h"

//-------------------------------------------------------------------------

namespace SE::Hash
{
    namespace XXHash
    {
        constexpr static uint32 const g_hashSeed = 'EE8';
    }

    uint32 XXHash::GetHash32(void const *pData, uint64 size)
    {
        return XXH32(pData, size, g_hashSeed);
    }

	uint32 XXHash::GetHash32(List<uint8> &data)
	{
		return GetHash32(data.Get(), (int64)data.Count());
	}

    uint64 XXHash::GetHash64(void const *pData, uint64 size)
    {
        return XXH64(pData, size, g_hashSeed);
    }

	uint64 XXHash::GetHash64(List<uint8> &data)
	{
		return GetHash64(data.Get(), data.Count());
	}
}