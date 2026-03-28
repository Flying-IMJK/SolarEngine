#pragma once
#ifdef PLATFORM_WIN32

//-------------------------------------------------------------------------

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif


//-------------------------------------------------------------------------
// Enable specific warnings
//-------------------------------------------------------------------------

#pragma warning(default:4800)
#pragma warning(default:4389)

#define PLATFORM_LINE_TERMINATOR "\r\n"

#endif