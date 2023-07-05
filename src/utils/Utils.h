#pragma once

inline int SwapBytesWord(int w)
{
    return ((w >> 8) & 0xFF) | ((w & 0xFF) << 8);
}
