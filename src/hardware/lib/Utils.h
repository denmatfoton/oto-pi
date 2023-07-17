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

template<int n>
class SequenceTrendAnalyzer
{
public:
    SequenceTrendAnalyzer()
    {}

    void Clear()
    {
        m_size = 0;
    }

    void Push(int value)
    {
        int pos = (m_start + m_size) % n;
        m_values[pos] = value;
        if (m_size < n)
        {
            ++m_size;
        }
        else
        {
            m_start = (m_start + 1) % n;
        }
    }

    int CurTrend()
    {
        int end = (m_start + m_size - 1) % n;
        return (m_values[end] - m_values[m_start]) / m_size;
    }

    bool IsFull() { return m_size == n; }
    int Size() { return m_size; }
    int Capacity() { return n; }

private:
    int m_values[n] = {};
    int m_size = 0;
    int m_start = 0;
};
