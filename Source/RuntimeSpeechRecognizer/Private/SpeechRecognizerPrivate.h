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

// CPU Architecture Detection
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #define ARCH_X86_FAMILY 1
#elif defined(__arm__) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
    #define ARCH_ARM_FAMILY 1
#endif

// Compiler Detection
#if defined(__clang__)
    #define COMPILER_CLANG 1
#elif defined(__GNUC__) || defined(__GNUG__)
    #define COMPILER_GCC 1
#elif defined(_MSC_VER)
    #define COMPILER_MSVC 1
#endif

// x86 Family Features
#ifdef ARCH_X86_FAMILY
    // SSE/SSE2 Detection
    #if defined(__SSE2__) || (defined(COMPILER_MSVC) && (defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)))
        #ifndef __SSE2__
            #define __SSE2__ 1
        #endif
    #endif

    // SSSE3 Detection
    #if defined(__SSSE3__) || (defined(COMPILER_MSVC) && defined(__SSE2__))
        #ifndef __SSSE3__
            #define __SSSE3__ 1
        #endif
    #endif

    // AVX Detection
    #if defined(__AVX__) || \
        (defined(COMPILER_MSVC) && _MSC_VER >= 1700 && defined(__SSE2__)) || \
        (defined(COMPILER_GCC) && defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)))
        #ifndef __AVX__
            #define __AVX__ 0
        #endif
    #endif

    // AVX2 Detection
    #if defined(__AVX2__) || \
        (defined(COMPILER_MSVC) && _MSC_VER >= 1800 && defined(__SSE2__)) || \
        (defined(PLATFORM_ALWAYS_HAS_AVX_2) && PLATFORM_ALWAYS_HAS_AVX_2)
        #ifndef __AVX2__
            #define __AVX2__ 0
        #endif
    #endif

    // AVX-512 Detection
    #if defined(__AVX512F__) || \
        (defined(PLATFORM_ALWAYS_HAS_AVX_512) && PLATFORM_ALWAYS_HAS_AVX_512)
        #ifndef __AVX512F__
            #define __AVX512F__ 1
        #endif

        // AVX-512VL Detection (subset of AVX-512)
        #if defined(__AVX512VL__) || \
            (defined(COMPILER_MSVC) && _MSC_VER >= 1912)
            #ifndef __AVX512VL__
                #define __AVX512VL__ 1
            #endif
        #endif

        // AVX-512VBMI Detection
        #if defined(__AVX512VBMI__) || \
            (defined(COMPILER_MSVC) && _MSC_VER >= 1914)
            #ifndef __AVX512VBMI__
                #define __AVX512VBMI__ 1
            #endif
        #endif
    #endif

    // FMA Detection
    #if defined(__FMA__) || \
        (defined(COMPILER_MSVC) && defined(__AVX2__))
        #ifndef __FMA__
            #define __FMA__ 1
        #endif
    #endif
#endif

// ARM Family Features
#ifdef ARCH_ARM_FAMILY
    // NEON Detection
    #if defined(__ARM_NEON) || defined(__ARM_NEON__)
        #ifndef __ARM_NEON__
            #define __ARM_NEON__ 1
        #endif
        #ifndef __ARM_NEON
            #define __ARM_NEON 1
        #endif
    #endif

    // ARM FMA Detection
    #if defined(__ARM_FEATURE_FMA) || \
        (defined(__ARM_FP) && defined(__ARM_NEON__))
        #ifndef __ARM_FEATURE_FMA
            #define __ARM_FEATURE_FMA 1
        #endif
    #endif

    // ARM SVE Detection
    #ifdef __ARM_FEATURE_SVE
        #ifndef __ARM_FEATURE_SVE
            #define __ARM_FEATURE_SVE 1
        #endif
    #endif
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
