#pragma once
#include "Botama.h"

#ifdef _MSC_VER
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)
#ifdef _WIN64
#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_BitScanReverse64)
#endif
#define USING_INTRINSICS
#elif defined(__GNUC__)
static inline unsigned char _BitScanForward(unsigned long* Index, U32 Mask)
{
    U32 Ret;
    __asm__
    (
        "bsf %[Mask], %[Ret]"
        : [Ret] "=r" (Ret)
        : [Mask] "mr" (Mask)
    );
    *Index = (unsigned long)Ret;
    return Mask ? 1 : 0;
}
static inline unsigned char _BitScanReverse(unsigned long* Index, U32 Mask)
{
    U32 Ret;
    __asm__
    (
        "bsr %[Mask], %[Ret]"
        : [Ret] "=r" (Ret)
        : [Mask] "mr" (Mask)
    );
    *Index = (unsigned long)Ret;
    return Mask ? 1 : 0;
}
#ifdef _WIN64
static inline unsigned char _BitScanForward64(unsigned long* Index, U64 Mask)
{
    U64 Ret;
    __asm__
    (
        "bsfq %[Mask], %[Ret]"
        : [Ret] "=r" (Ret)
        : [Mask] "mr" (Mask)
    );
    *Index = (unsigned long)Ret;
    return Mask ? 1 : 0;
}
static inline unsigned char _BitScanReverse64(unsigned long* Index, U64 Mask)
{
    U64 Ret;
    __asm__
    (
        "bsrq %[Mask], %[Ret]"
        : [Ret] "=r" (Ret)
        : [Mask] "mr" (Mask)
    );
    *Index = (unsigned long)Ret;
    return Mask ? 1 : 0;
}
#endif
#define USING_INTRINSICS
#endif

static inline U64 swap32(U64 in) {
    return in >> 32 | in << 32;
}