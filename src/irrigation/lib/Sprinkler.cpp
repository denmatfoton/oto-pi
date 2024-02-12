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

/*future<void> Sprinkler::ApplyWaterAsync(const Zone& zone, float density)
{
    return async(&Sprinkler::ApplyWater, this, zone, density);
}*/

HwResult Sprinkler::ApplyWater(const Zone& zone, float density)
{
    IfFailRetResult(m_spNozzle->OpenValve());

    HwResult result = HwResult::Failure;

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

    return result;
}

HwResult Sprinkler::ApplyArea(const vector<HwCoord>& points, float /*density*/)
{
    LogInfo("Apply area started");

    PolygonApplicator applicator(points);
    applicator.SetRIncrement(20.f);
    applicator.Begin();

    while (true)
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

    return HwResult::UnexpectedValue;
}

HwResult Sprinkler::ApplyPoints(const vector<HwCoord>& points, float density)
{
    LogInfo("Apply points started");

    static constexpr float c_defaultPointDurationMs = 5000.f;

    for (const HwCoord& point : points)
    {
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
