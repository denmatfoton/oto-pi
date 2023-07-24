#include "Sprinkler.h"

#include <NozzleControl.h>

#include <iostream>

using namespace std;

namespace Irrigation {

int Sprinkler::Init() {
    m_spNozzle.reset(new NozzleControl());
    return m_spNozzle->Init();
}

future<void> Sprinkler::ApplyWaterAsync(const Zone& zone, float density)
{
    return async(&Sprinkler::ApplyWater, this, zone, density);
}

void Sprinkler::ApplyWater(const Zone& zone, float density)
{
    switch (zone.GetType())
    {
    case ZoneType::Area:
        ApplyArea(zone.GetPoints(), density);
        break;
    case ZoneType::Line:
        ApplyLine(zone.GetPoints(), density);
        break;
    case ZoneType::Points:
        ApplyPoints(zone.GetPoints(), density);
        break;
    }
}

void Sprinkler::ApplyArea(const std::vector<Coord>& /*points*/, float /*density*/)
{

}

void Sprinkler::ApplyLine(const std::vector<Coord>& /*points*/, float /*density*/)
{
    
}

void Sprinkler::ApplyPoints(const std::vector<Coord>& /*points*/, float /*density*/)
{
    
}

int Sprinkler::StartZoneRecording(ZoneType type)
{
    m_spNewZone.reset(new Zone(type));
    auto fut = m_spNozzle->OpenValve();
    fut.wait();
    return fut.get() == I2cStatus::Success ? 0 : -1;
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
