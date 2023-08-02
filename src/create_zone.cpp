#include "CursesWraper.h"
#include "NozzleControl.h"
#include "Sprinkler.h"

#include <iostream>
#include <thread>

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

int ProcessInput(Irrigation::Sprinkler& sprinkler, const char* zoneFileName, string& message)
{
    int res = 0;
    NozzleControl& nozzle = sprinkler.GetNozzleControl();

    static constexpr int angleIncr = 10;
    static constexpr int pressureIncr = 20000;
    static constexpr int defaultDuty = 100;

    switch (getch())
    {
        case KEY_UP:
            nozzle.SetPressureDiff(pressureIncr, defaultDuty);
            x = max(x - 1, 0);
            break;

        case KEY_DOWN:
            nozzle.SetPressureDiff(-pressureIncr, defaultDuty);
            break;

        case KEY_RIGHT:
            nozzle.RotateDiff(angleIncr, defaultDuty);
            break;

        case KEY_LEFT:
            nozzle.RotateDiff(-angleIncr, defaultDuty);
            break;

        case 'q':
            res = 1;
            break;

        case 'o':
            nozzle.OpenValve();
            break;

        case 'c':
            nozzle.CloseValve();
            break;

        case 'p':
            sprinkler.AddZonePoint();
            break;

        case 's':
            sprinkler.TakeRecordedZone()->SaveToFile(zoneFileName);
            message = "Zone saved to ";
            message += zoneFileName;
            break;
    }

    return res;
}

void PrintZonePoints(const Irrigation::Zone& zone, int x, int y)
{
    auto& points = zone.GetPoints();
    mvprintw(x, y, "Zone points: ");

    bool first = true;
    for (auto point : points)
    {
        if (!first)
        {
            printw(", ");
        }
        printw("(%d, %d)", point.fi, point.r);
        first = false;
    }
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
    NozzleControl& nozzle = sprinkler.GetNozzleControl();

    {
        CursesWrapper cursesWrapper;
        if (cursesWrapper.Init() < 0)
        {
            return -1;
        }

        bool isActive = true;
        string message;

        while (isActive)
        {
            clear();

            mvprintw(0, 0, "Nozzle position: %d", nozzle.GetPositionFetchIfStale());
            mvprintw(1, 0, "Nozzle pressure: %d", nozzle.GetPressureFetchIfStale());
            
            if (sprinkler.RecordedZone() != nullptr)
            {
                PrintZonePoints(*sprinkler.RecordedZone(), 3, 0);
            }

            mvprintw(LINES - 1, 0, message.c_str());
            refresh();

            if (ProcessInput(sprinkler, zoneFileName, message) != 0)
            {
                break;
            }
        }
    }

    return 0;
}
