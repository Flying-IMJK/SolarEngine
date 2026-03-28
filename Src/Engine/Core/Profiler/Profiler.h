#pragma once

#include "ProfilerCPU.h"
#include "ProfilerGPU.h"

#ifdef SE_PROFILER

// Helper macros to profile CPU and GPU (GPU event must have name specified, CPU event has function name)
#define PROFILE_GPU_CPU(name) \
	PROFILE_GPU(name); \
	PROFILE_CPU()
#define PROFILE_GPU_CPU_NAMED(name) \
	PROFILE_GPU(name); \
	PROFILE_CPU_NAMED(name)

#else

#define PROFILE_GPU_CPU(name)
#define PROFILE_GPU_CPU_NAMED(name)

#endif
