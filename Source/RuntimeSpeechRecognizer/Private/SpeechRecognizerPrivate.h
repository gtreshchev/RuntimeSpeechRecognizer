// Georgy Treshchev 2024.

#pragma once

#include "HAL/Platform.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#define NOMINMAX				// Macros min(a,b) and max(a,b)
#endif

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
/*#if defined(__cpp_lib_hardware_interference_size)
#undef __cpp_lib_hardware_interference_size
#endif */

#if !PLATFORM_LITTLE_ENDIAN
#define GGML_BIG_ENDIAN
#endif

// Microsoft Visual Studio
#if defined(_MSC_VER) && !defined(__clang__)

#if (defined(_M_IX86_FP) && _M_IX86_FP >= 2) || defined(_M_X64)
#ifndef __SSE2__
#define __SSE2__ 1
#endif
#ifndef __SSSE3__
#define __SSSE3__ 1
#endif
#endif

#if !defined(__AVX__) && (_MSC_VER >= 1700 && defined(__SSE2__))
#define __AVX__ 1
#endif
#if !defined(__AVX2__) && ((_MSC_VER >= 1800 && defined(__SSE2__)) \
|| (defined(PLATFORM_ALWAYS_HAS_AVX_2) && PLATFORM_ALWAYS_HAS_AVX_2))
#define __AVX2__ 1
#endif

#if !defined(__AVX512F__) && defined(PLATFORM_ALWAYS_HAS_AVX_512) && PLATFORM_ALWAYS_HAS_AVX_512
#define __AVX512F__ 1
#endif

/* AVX512 requires VS 15.3 */
#if !defined(__AVX512F__) && (_MSC_VER >= 1911 && defined(__AVX__))
// Commented out because it has some platform-specific issues
//#define __AVX512F__ 1
#endif

/* AVX512VL not available until VS 15.5 */
#if defined(__AVX512F__) && _MSC_VER >= 1912
#define __AVX512VL__ 1
#endif

/* VBMI added in 15.7 */
#if defined(__AVX512F__) && _MSC_VER >= 1914
#define __AVX512VBMI__ 1
#endif

#endif

// TODO: Add similar checks for other compilers

#if !defined(__ARM_NEON) && defined(__ARM_NEON__)
#define __ARM_NEON 1
#endif

#if !defined(__ARM_FEATURE_FMA) && defined(__arm__)
#define __ARM_FEATURE_FMA 1
#endif

THIRD_PARTY_INCLUDES_START

#include "whisper.h"
#include "whisper.cpp"

#ifdef GGML_USE_VULKAN
//#define GGML_ABORT(...) return;
#include "ggml-vulkan.h"
#include "ggml-vulkan.cpp"
#endif

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include "Windows/AllowWindowsPlatformAtomics.h"
#undef NOATOM
#undef NOKERNEL
#endif
extern "C"
{
#include "ggml.h"
#include "ggml.c"
//#include "ggml-alloc.h"
#include "ggml-alloc.c"
//#include "ggml-quants.h"
#include "ggml-quants.c"
//#include "ggml-backend.h"
#include "ggml-backend.cpp"
//#include "ggml-aarch64.h"
#include "ggml-aarch64.c"
}
#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformAtomics.h"
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#if GGML_USE_BLAS
#include "ggml-blas.h"
#include "ggml-blas.cpp"
#endif

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
