#pragma once

#include "Zone.h"

#include <future>
#include <memory>

class NozzleControl;

namespace Irrigation {

class Sprinkler
{
public:
    Sprinkler() = default;

    int Init();
    
    std::future<void> ApplyWaterAsync(const Zone& zone, float density);
    void ApplyWater(const Zone& zone, float density);

    // Zone recording
    int StartZoneRecording(ZoneType type);
    void AddZonePoint();
    const Zone* RecordedZone() const { return m_spNewZone.get(); }
    std::unique_ptr<Zone>&& TakeRecordedZone() { return std::move(m_spNewZone); }

    NozzleControl& GetNozzleControl() { return *m_spNozzle; }

private:
    void ApplyArea(const std::vector<Coord>& points, float density);
    void ApplyLine(const std::vector<Coord>& points, float density);
    void ApplyPoints(const std::vector<Coord>& points, float density);

    std::unique_ptr<NozzleControl> m_spNozzle;
    std::unique_ptr<Zone> m_spNewZone;
};

}
