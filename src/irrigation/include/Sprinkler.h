#pragma once

#include "Zone.h"

#include <future>
#include <memory>
#include <functional>

class Logger;
class NozzleControlCalibrated;
enum class HwResult : int;

namespace Irrigation {

using CompletionCallback = std::function<void(HwResult)>;

class Sprinkler
{
public:
    Sprinkler() = default;

    int Init();

    void SetLogger(Logger* pLogger);
    NozzleControlCalibrated& GetNozzleControl() { return *m_spNozzle; }
    
    HwResult StartWateringAsync(Zone&& zone, float density, CompletionCallback callback = nullptr);
    HwResult StartWatering(Zone&& zone, float density);
    HwResult StopWatering();

    // Zone recording
    void StartZoneRecording(ZoneType type);
    void AddZonePoint();
    const Zone* RecordedZone() const { return m_spNewZone.get(); }
    std::unique_ptr<Zone>&& TakeRecordedZone() { return std::move(m_spNewZone); }

private:
    HwResult ApplyArea(const std::vector<HwCoord>& points, float density);
    HwResult ApplyLine(const std::vector<HwCoord>& points, float density);
    HwResult ApplyPoints(const std::vector<HwCoord>& points, float density);

    std::unique_ptr<NozzleControlCalibrated> m_spNozzle;
    std::unique_ptr<Zone> m_spNewZone;

    Logger* m_pLogger = nullptr;

    future<HwResult> m_wateringFuture;
    std::atomic<bool> m_isWatering = false;
};

}
