#pragma once

#include <cstdint>

#include <chrono>

inline int SwapBytesWord(int w)
{
    return ((w >> 8) & 0xFF) | ((w & 0xFF) << 8);
}

inline uint32_t TimeSinceEpochMs()
{
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count() & 0xffffffffLL);
}
