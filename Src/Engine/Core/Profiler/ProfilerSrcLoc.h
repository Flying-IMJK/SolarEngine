
#pragma once

#ifdef SE_PROFILER

#include "Core/Types/Variable.h"

struct SE_API_CORE SourceLocationData
{
    const char* name;
    const char* function;
    const char* file;
    uint32 line;
    uint32 color;
};

#endif
