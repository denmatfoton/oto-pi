#pragma once

#include <vector>

enum class ZoneType : int
{
    Area,
    Line,
    Points,
};

struct Coord
{
    Coord() = default;
    Coord(int _r, int _fi) : r(_r), fi(_fi) {}

    friend bool operator==(const Coord& lhs, const Coord& rhs)
    {
        return lhs.r == rhs.r && lhs.fi == rhs.fi;
    }

    int r = 0;
    int fi = 0;
};

class Zone
{
public:
    Zone() {}
    Zone(ZoneType type) : m_type(type) {}

    ZoneType GetType() const { return m_type; }
    const std::vector<Coord>& GetPoints() const { return m_points; }

    void AddPoint(int r, int fi)
    {
        m_points.emplace_back(r, fi);
    }

    int SaveToFile(const char* fileName) const;
    int LoadFromFile(const char* fileName);

private:
    ZoneType m_type = ZoneType::Area;
    std::vector<Coord> m_points;
};
