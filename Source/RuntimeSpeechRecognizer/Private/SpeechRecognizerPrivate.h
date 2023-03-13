// Georgy Treshchev 2023.

#pragma once

#ifndef restrict
#define restrict
#endif

#ifndef M_PI
#define M_PI (3.1415926535897932f)
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Required because sometimes even if __cpp_lib_hardware_interference_size is present, hardware_destructive_interference_size is not always present
#if defined(__cpp_lib_hardware_interference_size)
#undef __cpp_lib_hardware_interference_size
#endif 

#if !PLATFORM_LITTLE_ENDIAN
#define GGML_BIG_ENDIAN
#endif

THIRD_PARTY_INCLUDES_START

#include "whisper.h"
#include "whisper.cpp"

extern "C"
{
#include "ggml.h"
#include "ggml.c"
}

THIRD_PARTY_INCLUDES_END

#undef CACHE_LINE_SIZE
#undef restrict
#undef static_assert
#undef MIN
#undef MAX
#undef max
#undef min
#undef UNUSED
#undef SWAP
