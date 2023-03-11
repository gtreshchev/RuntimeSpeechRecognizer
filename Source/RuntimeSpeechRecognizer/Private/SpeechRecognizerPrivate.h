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

// Uncomment this code if facing compilation issues with hardware_destructive_interference_size/hardware_constructive_interference_size
/* #if defined(__cpp_lib_hardware_interference_size)
#undef __cpp_lib_hardware_interference_size
#endif */

namespace std
{
#if defined(__GCC_DESTRUCTIVE_SIZE) && defined(__GCC_CONSTRUCTIVE_SIZE)
	inline constexpr size_t hardware_destructive_interference_size = __GCC_DESTRUCTIVE_SIZE;
	inline constexpr size_t hardware_constructive_interference_size = __GCC_CONSTRUCTIVE_SIZE;
#endif
}

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
