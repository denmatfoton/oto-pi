#pragma once

#include <PolarCoordinates.h>

#include <vector>

namespace Irrigation {

enum class ZoneType : int
{
    Area,
    Line,
    Points,
};

class Zone
{
public:
    Zone() {}
    Zone(ZoneType type) : m_type(type) {}

    ZoneType GetType() const { return m_type; }
    const std::vector<HwCoord>& GetPoints() const { return m_points; }

    void AddPoint(int r, int fi)
    {
        m_points.emplace_back(r, fi);
    }

    int SaveToFile(const char* fileName) const;
    int LoadFromFile(const char* fileName);

private:
    ZoneType m_type = ZoneType::Area;
    std::vector<HwCoord> m_points;
};

}
