#pragma once

#include "platform/PlatformConfig.h"
#include "java/Type.h"

// World-size choices are a platform policy, not a terrain-generator concern.
// PSP can stream an infinite world, but the 864x864 legacy preset has no useful
// memory advantage over that streaming path. Keep the compact Old preset as a
// compatibility/split-screen option and make Infinite the normal experience.
inline constexpr int_t platformDefaultWorldSizeType()
{
    return 0; // Infinite
}

inline constexpr int_t platformCycleWorldSizeType(int_t current, int_t direction)
{
#if PLATFORM_PSP
    (void)direction;
    return current == 0 ? 1 : 0; // Infinite <-> Old (256x256)
#else
    const int_t delta = direction >= 0 ? 1 : 2;
    return (current + delta) % 3;
#endif
}

