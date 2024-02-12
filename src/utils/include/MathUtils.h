#pragma once

#include <cmath>
#include <vector>

static constexpr float c_fpi = static_cast<float>(M_PI);
static constexpr float c_f2pi = 2.f * static_cast<float>(M_PI);
static constexpr float c_fpi_2 = static_cast<float>(M_PI_2);

inline bool EqualFloat(float a, float b)
{
	static constexpr float epsilon = 0.001f;
	return fabsf(a - b) < epsilon;
}

inline float RoundUp(float x, float chunk)
{
	return ceilf(x / chunk) * chunk;
}

inline float Sgn(float a)
{
	return 1.f - 2.f * std::signbit(a);
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
        int lastPos = (m_start + m_size - 1) % n;
        int previousValue = IsEmpty() ? 0 : m_valuesSum[lastPos];

        int curPos = (m_start + m_size) % n;
        m_valuesSum[curPos] = value + previousValue;
        if (m_size < n)
        {
            ++m_size;
        }
        else
        {
            m_start = (m_start + 1) % n;
        }
    }

    int AvgValue()
    {
        if (IsEmpty()) return 0;
        if (m_size == 1) return m_valuesSum[m_start];
        return AvgRange(0, m_size - 1);
    }

    int AvgRange(int s, int k)
    {
        int first = (m_start + s) % n;
        int last = (m_start + s + k) % n;
        return (m_valuesSum[last] - m_valuesSum[first]) / k;
    }

    int CurTrend(int k)
    {
        if (m_size < 2) return 0;
        int avgFirst = AvgRange(0, k);
        int avgLast = AvgRange(m_size - k - 1, k);
        return avgLast - avgFirst;
    }

    bool IsFull() { return m_size == n; }
    bool IsEmpty() { return m_size == 0; }
    int Size() { return m_size; }
    int Capacity() { return n; }

private:
    int m_valuesSum[n] = {};
    int m_size = 0;
    int m_start = 0;
};

class Interpolator
{
public:
    void SetValues(std::vector<std::pair<int, int>>&& values)
    {
        m_values = std::move(values);
    }
    int Predict(int x) const;

private:
    std::vector<std::pair<int, int>> m_values;
};
