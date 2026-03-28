#pragma once

#include "Core/API.h"
#include "Core/Templates.h"
#include "Variable.h"


namespace SE
{
	namespace Hash
	{
		// XXHash
		//-------------------------------------------------------------------------
		// This is the default hashing algorithm for the engine

		namespace XXHash
		{
			SE_API_CORE uint32 GetHash32(void const *pData, uint64 size);

			SE_API_CORE uint32 HashCombine32(void const *pData, uint64 size, uint32 hash);

			SE_API_CORE uint32 GetHash32(List<uint8> &data);

			//-------------------------------------------------------------------------

			SE_API_CORE uint64 GetHash64(void const *pData, uint64 size);

			SE_API_CORE uint64 GetHash64(List<uint8> &data);
		}

		// FNV1a
		//-------------------------------------------------------------------------
		// This is a const expression hash
		// Should not be used for anything other than code only features i.e. custom RTTI etc...

		namespace FNV1a
		{
			constexpr uint32 const g_constValue32 = 0x811c9dc5;
			constexpr uint32 const g_defaultOffsetBasis32 = 0x1000193;
			constexpr uint64 const g_constValue64 = 0xcbf29ce484222325;
			constexpr uint64 const g_defaultOffsetBasis64 = 0x100000001b3;

			constexpr static inline uint32 GetHash32(char const *const str, const uint32 val = g_constValue32)
			{
				return (str[0] == '\0') ? val : GetHash32(&str[1], ((uint64)val ^ uint32(str[0])) * g_defaultOffsetBasis32);
			}

			constexpr static inline uint64 GetHash64(char const *const str, const uint64 val = g_constValue64)
			{
				return (str[0] == '\0') ? val : GetHash64(&str[1], ((uint64)val ^ uint64(str[0])) * g_defaultOffsetBasis64);
			}
		}

	}

    // Default hashing functions
    //-------------------------------------------------------------------------
	inline SE_API_CORE uint32 GetHash(uint64 value)
	{
		return Hash::XXHash::GetHash32(&value, 1);
	}

    inline SE_API_CORE uint32 GetHash(void const *pPtr, uint64 size)
    {
        return Hash::XXHash::GetHash32(pPtr, size);
    }

	inline SE_API_CORE uint32 GetHash(const void* key)
	{
		static const int64 shift = 3;
		return (uint32)((int64)(key) >> shift);
	}

	template<typename EnumType>
	inline typename TEnableIf<TIsEnum<EnumType>::Value, uint32>::Type GetHash(const EnumType key)
	{
		return GetHash((__underlying_type(EnumType))key);
	}

	inline uint32 HashCombine(uint32 &hash, uint64 v)
	{
		return hash ^ (GetHash(v) + 0x9e3779b9 + (hash << 6) + (hash >> 2));
	}

	inline uint32 HashCombine(uint32 &hash, void const *pPtr, uint64 size)
	{
		return hash ^ (GetHash(pPtr, size) + 0x9e3779b9 + (hash << 6) + (hash >> 2));
	}

	template<typename T>
	inline uint32 HashCombine(uint32 &hash, T *pPtr)
	{
		hash ^= GetHash(pPtr, sizeof(T)) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	}
};