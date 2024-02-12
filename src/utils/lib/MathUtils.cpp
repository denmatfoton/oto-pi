#include "MathUtils.h"

#include <algorithm>

using namespace std;

int Interpolator::Predict(int x) const
{
    if (m_values.empty())
    {
        return 0;
    }

    if (x >= m_values.back().first)
    {
        return x * m_values.back().second / m_values.back().first;
    }
    
    auto it = lower_bound(cbegin(m_values), cend(m_values), make_pair(x, 0));
    auto p0 = *it;
    ++it;
    auto p1 = *it;
    return (x - p0.first) * (p1.second - p0.second) / (p1.first - p0.first) + p0.second;
}
