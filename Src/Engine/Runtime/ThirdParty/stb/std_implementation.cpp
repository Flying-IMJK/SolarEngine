#pragma once

#include "Runtime/Core/Memory/Memory.h"
#include "Runtime/Core/Math/Math.h"
#include "Runtime/Core/Logging/Logging.h"

//-------------------------------------------------------------------------
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

//-------------------------------------------------------------------------
#define STBI_MALLOC SE::PlatformAllocator::Allocate
#define STBI_REALLOC SE::PlatformAllocator::Realloc
#define STBI_FREE SE::PlatformAllocator::Free
#define STBI_NO_PSD
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_FAILURE_STRINGS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


#define STBIR_ASSERT(x) ENGINE_ASSERT(x)
#define STBIR_MALLOC(sz, c) SE::PlatformAllocator::Allocate(sz)
#define STBIR_FREE(p, c) SE::PlatformAllocator::Free(p)
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"


#define STBIW_ASSERT(x) ENGINE_ASSERT(x)
#define STBIW_MALLOC(sz) SE::PlatformAllocator::Allocate(sz)
#define STBIW_REALLOC(p, newsz) SE::PlatformAllocator::Realloc(p, newsz)
#define STBIW_REALLOC_SIZED(p, oldsz, newsz) SE::PlatformAllocator::Realloc(p, oldsz, newsz)
#define STBIW_FREE(p) SE::PlatformAllocator::Free(p)
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STBD_ABS(i) SE::Math::Abs(i)
#define STBD_FABS(x) SE::Math::Abs(x)
#define STB_DXT_IMPLEMENTATION
#include "stb_dxt.h"