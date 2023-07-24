
#include "NozzleControl.h"
#include "Sprinkler.h"

#include <iostream>

using namespace std;

Irrigation::ZoneType GetZoneType(char c)
{
    switch (c)
    {
    case 'a':
        return Irrigation::ZoneType::Area;
    case 'l':
        return Irrigation::ZoneType::Line;
    case 'p':
        return Irrigation::ZoneType::Points;
    }
    return Irrigation::ZoneType::Points;
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cerr << "Incorrect number of arguments." << endl;
        return -1;
    }

    const char* zoneFileName = argv[1];
    auto zoneType = GetZoneType(argv[2][0]);

    Irrigation::Sprinkler sprinkler;
    IfFailRet(sprinkler.Init());

    IfFailRet(sprinkler.StartZoneRecording(zoneType));
    //NozzleControl& nozzle = sprinkler.GetNozzleControl();

    while (true)
    {
        sprinkler.AddZonePoint();
    }

    auto spZone = sprinkler.GetNewZone();
    spZone->SaveToFile(zoneFileName);

    return 0;
}
