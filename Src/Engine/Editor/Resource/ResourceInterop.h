#pragma once

#include "Editor/API.h"
#include "Runtime/Core/Types/Variable.h"

#include <cstdint>

// This is intentionally a narrow ABI boundary. Managed resource presentation code
// owns editor state and UI, while asset decoding/importing continues to execute in
// the native runtime where the importer registry and GPU resources are available.
extern "C"
{
    SE_API_EDITOR int32_t ResourceInterop_ImportTexture(const SE::Char* inputPath, const SE::Char* outputPath);
    SE_API_EDITOR int32_t ResourceInterop_ImportModel(const SE::Char* inputPath, const SE::Char* outputPath);
}
