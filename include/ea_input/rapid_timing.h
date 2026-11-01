#pragma once

#include <algorithm>
#include <cstdint>

namespace RapidTiming
{
// period is a complete ON/OFF cycle in frames. For odd periods, the
// front phase has the extra ON frame; the back phase is its complement.
inline bool globalPhaseOn(uint32_t frame, int period, bool back)
{
    const auto frames = static_cast<uint32_t>(std::max(2, period));
    const bool frontOn = frame % frames < (frames + 1) / 2;
    return back ? !frontOn : frontOn;
}

inline bool synchronizedOn(uint32_t frame, uint32_t startFrame, int period)
{
    return globalPhaseOn(frame - startFrame, period, false);
}
}
