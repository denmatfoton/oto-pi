#pragma once

#include "Zone.h"

#include <future>
#include <memory>

class Logger;
class NozzleControlCalibrated;
enum class HwResult : int;

namespace Irrigation {

class Sprinkler
{
public:
    Sprinkler() = default;

    int Init();

    void SetLogger(Logger* pLogger);
    NozzleControlCalibrated& GetNozzleControl() { return *m_spNozzle; }
    
    //std::future<void> ApplyWaterAsync(const Zone& zone, float density);
    HwResult ApplyWater(const Zone& zone, float density);

    // Zone recording
    void StartZoneRecording(ZoneType type);
    void AddZonePoint();
    const Zone* RecordedZone() const { return m_spNewZone.get(); }
    std::unique_ptr<Zone>&& TakeRecordedZone() { return std::move(m_spNewZone); }

private:
    HwResult ApplyArea(const std::vector<Coord>& points, float density);
    HwResult ApplyLine(const std::vector<Coord>& points, float density);
    HwResult ApplyPoints(const std::vector<Coord>& points, float density);

    std::unique_ptr<NozzleControlCalibrated> m_spNozzle;
    std::unique_ptr<Zone> m_spNewZone;

    Logger* m_pLogger = nullptr;
};

}
