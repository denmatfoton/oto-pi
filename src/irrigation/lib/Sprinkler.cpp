#include "Sprinkler.h"

#include <Logger.h>
#include <NozzleControl.h>
#include "PolygonApplicator.h"

using namespace std;

namespace Irrigation {

static constexpr int c_defaultDutyPercent = 100;

int Sprinkler::Init() {
    m_spNozzle.reset(new NozzleControlCalibrated());
    return m_spNozzle->Init();
}

void Sprinkler::SetLogger(Logger* pLogger)
{
    m_pLogger = pLogger;
    m_spNozzle->SetLogger(pLogger);
}

HwResult Sprinkler::StartWateringAsync(Zone&& zone, float density, CompletionCallback callback)
{
    bool expected = false;
    if (!m_isWatering.compare_exchange_strong(expected, true))
    {
        // Already watering
        if (callback)
        {
            callback(HwResult::Abort);
        }
        return HwResult::Abort;
    }

    m_wateringFuture = async(launch::async, [this, zone = move(zone), density, callback]() mutable {
        HwResult result = StartWatering(move(zone), density);
        m_isWatering = false;
        if (callback)
        {
            callback(result);
        }
        return result;
    });
    
    return HwResult::Success;
}

HwResult Sprinkler::StopWatering()
{
    if (!m_isWatering)
    {
        return HwResult::Success;
    }

    m_isWatering = false;

    if (m_wateringFuture.valid())
    {
        m_wateringFuture.wait();
    }

    return HwResult::Success;
}

HwResult Sprinkler::StartWatering(Zone&& zone, float density)
{
    // m_isWatering should already be set by StartWateringAsync
    // This is just a defensive check if called directly
    bool expected = false;
    if (!m_isWatering.compare_exchange_strong(expected, true))
    {
        return HwResult::Abort;
    }

    HwResult result = m_spNozzle->OpenValve();
    if (result != HwResult::Success)
    {
        m_isWatering = false;
        return result;
    }

    switch (zone.GetType())
    {
    case ZoneType::Area:
        result = ApplyArea(zone.GetPoints(), density);
        break;
    case ZoneType::Line:
        result = ApplyLine(zone.GetPoints(), density);
        break;
    case ZoneType::Points:
        result = ApplyPoints(zone.GetPoints(), density);
        break;
    }
    
    m_spNozzle->CloseValve(true);
    m_isWatering = false;

    return result;
}

HwResult Sprinkler::ApplyArea(const vector<HwCoord>& points, float /*density*/)
{
    LogInfo("Apply area started");

    PolygonApplicator applicator(points);
    applicator.SetRIncrement(20.f);
    applicator.Begin();

    while (m_isWatering)
    {
        HwArc arc = applicator.NextArc();
        if (!arc.IsValid()) break;

        int curFi = m_spNozzle->GetPositionFetchIfStale();

        MotorDirection rotationDirection = MotorDirection::Right;
        int startAngle = arc.fi0;
        int endAngle = arc.fi1;

        if (HwCoord::AngleAbsDiff(curFi, arc.fi0) > HwCoord::AngleAbsDiff(curFi, arc.fi1))
        {
            rotationDirection = MotorDirection::Left;
            swap(startAngle, endAngle);
        }

        auto rotateFuture = m_spNozzle->RotateToAsync(startAngle, c_defaultDutyPercent);
        auto pressureFuture = m_spNozzle->SetPressureAsync(arc.r, c_defaultDutyPercent);

        rotateFuture.wait();
        pressureFuture.wait();
        
        IfFailRetResult(rotateFuture.get());
        IfFailRetResult(pressureFuture.get());

        // TODO: calculate duty based on density and current water pressure (R position).
        int dutyPercent = 100;

        rotateFuture = m_spNozzle->RotateToDirectionAsync(rotationDirection, endAngle, dutyPercent);

        rotateFuture.wait();
        IfFailRetResult(rotateFuture.get());
    }

    return HwResult::Success;
}

HwResult Sprinkler::ApplyLine(const vector<HwCoord>& /*points*/, float /*density*/)
{
    LogInfo("Apply line started");

    return HwResult::UnexpectedValue; // TODO: implement line applicator
}

HwResult Sprinkler::ApplyPoints(const vector<HwCoord>& points, float density)
{
    LogInfo("Apply points started");

    static constexpr float c_defaultPointDurationMs = 5000.f;

    for (const HwCoord& point : points)
    {
        if (!m_isWatering)
        {
            break;
        }

        auto rotateFuture = m_spNozzle->RotateToAsync(point.fi, c_defaultDutyPercent);
        auto pressureFuture = m_spNozzle->SetPressureAsync(point.r, c_defaultDutyPercent);

        chrono::milliseconds duration(static_cast<int>(c_defaultPointDurationMs * density));

        rotateFuture.wait();
        pressureFuture.wait();

        IfFailRetResult(rotateFuture.get());
        IfFailRetResult(pressureFuture.get());

        this_thread::sleep_for(duration);
    }
    
    return HwResult::Success;
}

void Sprinkler::StartZoneRecording(ZoneType type)
{
    m_spNewZone.reset(new Zone(type));
}

void Sprinkler::AddZonePoint()
{
    if (m_spNewZone == nullptr)
    {
        return;
    }
    int r = m_spNozzle->GetPressureFetchIfStale();
    int fi = m_spNozzle->GetPositionFetchIfStale();
    m_spNewZone->AddPoint(r, fi);
}

}
