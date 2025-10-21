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

template<int n, int maxRate, typename multiplicationT>
class OvershootInterpolator
{
    static_assert(n > 0, "OvershootInterpolator size must be positive");
    static_assert(maxRate > 0, "maxRate must be positive");

    static constexpr int c_rateResolution = maxRate / n;
    static_assert(c_rateResolution * n == maxRate, "maxRate must be divisible by n");
public:

    void SetOvershoot(int rate, int overshoot)
    {
        if (rate < -maxRate)
        {
            UpdateOvershoot(0, ExtrapolateOvershoot(rate, overshoot, -maxRate));
            return;
        }
        if (rate > maxRate)
        {
            UpdateOvershoot(2 * n, ExtrapolateOvershoot(rate, overshoot, maxRate));
            return;
        }

        int rateAdjusted = rate + maxRate;
        int leftIdx = rateAdjusted / c_rateResolution;
        if (rateAdjusted % c_rateResolution == 0)
        {
            UpdateOvershoot(leftIdx, overshoot);
            return;
        }

        int leftRate = (leftIdx - n) * c_rateResolution;
        UpdateOvershoot(leftIdx, ExtrapolateOvershoot(rate, overshoot, leftRate));

        int rightRate = leftRate + c_rateResolution;
        UpdateOvershoot(leftIdx + 1, ExtrapolateOvershoot(rate, overshoot, rightRate));
    }

    int PredictOvershoot(int rate) const
    {
        if (rate == 0)
        {
            return 0;
        }

        int rateAdjusted = rate + maxRate;
        int l = rateAdjusted / c_rateResolution;
        int rem = rateAdjusted % c_rateResolution;
        int r = rem == 0 ? l : l + 1;
        if (rate < -maxRate)
        {
            l = r = 0;
        }
        if (rate > maxRate)
        {
            l = r = 2 * n;
        }

        int leftRate = (l - n) * c_rateResolution;
        int rightRate = (r - n) * c_rateResolution;

        int leftExtrapolate = 0;
        int rightExtrapolate = 0;

        for (; l >= 0 || r <= 2 * n; --l, ++r)
        {
            if (l >= 0 && m_overshoot[l] != 0)
            {
                leftExtrapolate = ExtrapolateOvershoot(leftRate, m_overshoot[l], rate);
            }
            if (r <= 2 * n && m_overshoot[r] != 0)
            {
                rightExtrapolate = ExtrapolateOvershoot(rightRate, m_overshoot[r], rate);
            }
            if (leftExtrapolate != 0 && rightExtrapolate != 0)
            {
                return (leftExtrapolate + rightExtrapolate) / 2;
            }
            if (leftExtrapolate != 0)
            {
                return leftExtrapolate;
            }
            if (rightExtrapolate != 0)
            {
                return rightExtrapolate;
            }

            leftRate -= c_rateResolution;
            rightRate += c_rateResolution;
        }

        return 0;
    }

private:
    static int ExtrapolateOvershoot(int knownRate, int knownOvershoot, int rate)
    {
        if (knownRate == 0)
        {
            return 0;
        }
        return static_cast<multiplicationT>(rate) * rate * knownOvershoot / (knownRate * knownRate);
    }

    void UpdateOvershoot(int idx, int overshoot)
    {
        if (m_overshoot[idx] == 0)
        {
            m_overshoot[idx] = overshoot;
        }
        else
        {
            m_overshoot[idx] = (m_overshoot[idx] + overshoot) / 2;
        }
    }

    int m_overshoot[n * 2 + 1] = {};
};
