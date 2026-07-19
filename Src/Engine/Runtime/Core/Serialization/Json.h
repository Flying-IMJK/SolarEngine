#pragma once

#undef RAPIDJSON_ERROR_CHARTYPE
#undef RAPIDJSON_ERROR_STRING
#undef RAPIDJSON_ASSERT
#undef RAPIDJSON_NEW
#undef RAPIDJSON_DELETE

// TODO: config RAPIDJSON_SSE2 for rapidjson
#define RAPIDJSON_ERROR_CHARTYPE SE::Char
#define RAPIDJSON_ERROR_STRING(x) SE_TEXT(x)
#define RAPIDJSON_ASSERT(x) ENGINE_ASSERT(x)
#define RAPIDJSON_NEW(x) SE::New<x>
#define RAPIDJSON_DELETE(x) SE::Delete(x)
#define RAPIDJSON_NOMEMBERITERATORCLASS

#include "Runtime/Core/Types/Strings/String.h"
#include "Runtime/Core/Types/Strings/StringView.h"
#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

namespace SE::Json
{
    // The memory allocator implementation for rapidjson library that uses default engine Allocator.
    class SE_API_RUNTIME FlaxAllocator
    {
    public:
        static const bool kNeedFree = true;

        void* Malloc(size_t size)
        {
            // Behavior of malloc(0) is implementation defined so for size=0 return nullptr.
            // By default Flax doesn't use Allocate(0) so it's not important for the engine itself.
            if (size)
                return PlatformAllocator::Allocate((uint64)size);
            return nullptr;
        }

        void* Realloc(void* originalPtr, size_t originalSize, size_t newSize)
        {
            return PlatformAllocator::Realloc(originalPtr, (uint64)originalSize, (uint64)newSize);
        }

        static void Free(void* ptr)
        {
			PlatformAllocator::Free(ptr);
        }
    };

    // String buffer with UTF8 encoding
	typedef rapidjson::GenericStringBuffer<rapidjson::UTF8<>, FlaxAllocator> StringBuffer;

    // GenericDocument with UTF8 encoding
	typedef rapidjson::GenericDocument<rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<FlaxAllocator>, FlaxAllocator> Document;

    // GenericValue with UTF8 encoding
	typedef rapidjson::GenericValue<rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<FlaxAllocator>> Value;

    typedef rapidjson::GenericArray<false, rapidjson::GenericValue<rapidjson::UTF8<>>> Array;

    // JSON writer to the stream
    template<typename OutputStream, typename SourceEncoding = rapidjson::UTF8<>, typename TargetEncoding = rapidjson::UTF8<>, unsigned writeFlags = rapidjson::kWriteDefaultFlags>
    using Writer = rapidjson::Writer<OutputStream, SourceEncoding, TargetEncoding, FlaxAllocator, writeFlags>;

    // Pretty JSON writer to the stream
    template<typename OutputStream, typename SourceEncoding = rapidjson::UTF8<>, typename TargetEncoding = rapidjson::UTF8<>, unsigned writeFlags = rapidjson::kWriteDefaultFlags>
    using PrettyWriter = rapidjson::PrettyWriter<OutputStream, SourceEncoding, TargetEncoding, FlaxAllocator, writeFlags>;

}
