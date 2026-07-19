
#pragma once

#ifdef SE_PROFILER

#include "Runtime/Core/Types/Variable.h"

struct SE_API_RUNTIME SourceLocationData
{
    const char* name;
    const char* function;
    const char* file;
    uint32 line;
    uint32 color;
};

#endif
