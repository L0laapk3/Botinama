#pragma once

#ifdef _MSC_VER
#include <intrin.h>
#ifdef _WIN64
#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)
#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_BitScanReverse64)
#define USING_INTRINSICS
#endif
#elif defined(__GNUC__)
static inline unsigned char _BitScanForward(unsigned long* Index, uint32_t Mask)
{
    uint32_t Ret;
    __asm__
    (
        "bsf %[Mask], %[Ret]"
        : [Ret] "=r" (Ret)
        : [Mask] "mr" (Mask)
    );
    *Index = (unsigned long)Ret;
    return Mask ? 1 : 0;
}
static inline unsigned char _BitScanReverse(unsigned long* Index, uint32_t Mask)
{
    uint32_t Ret;
    __asm__
    (
        "bsr %[Mask], %[Ret]"
        : [Ret] "=r" (Ret)
        : [Mask] "mr" (Mask)
    );
    *Index = (unsigned long)Ret;
    return Mask ? 1 : 0;
}
#if defined(__LP64__)
static inline unsigned char _BitScanForward64(unsigned long* Index, uint64_t Mask)
{
    uint64_t Ret;
    __asm__
    (
        "bsfq %[Mask], %[Ret]"
        : [Ret] "=r" (Ret)
        : [Mask] "mr" (Mask)
    );
    *Index = (unsigned long)Ret;
    return Mask ? 1 : 0;
}
static inline unsigned char _BitScanReverse64(unsigned long* Index, uint64_t Mask)
{
    uint64_t Ret;
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