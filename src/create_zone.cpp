#include <CursesWrapper.h>
#include <Logger.h>
#include <MathUtils.h>
#include <NozzleControl.h>
#include <Sprinkler.h>

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

int ProcessInput(Irrigation::Sprinkler& sprinkler, const char* zoneFileName, string& message)
{
    int res = 0;
    NozzleControlCalibrated& nozzle = sprinkler.GetNozzleControl();

    static constexpr int angleIncr = 10;
    //static constexpr int pressureIncr = 20000;
    static constexpr int defaultDuty = 100;

    switch (getch())
    {
        case KEY_UP:
            nozzle.TurnValve(MotorDirection::Open, chrono::milliseconds(20), defaultDuty);
            break;

        case KEY_DOWN:
            nozzle.TurnValve(MotorDirection::Close, chrono::milliseconds(20), defaultDuty);
            break;

        case KEY_RIGHT:
            nozzle.RotateDiffDurationBased(angleIncr);
            break;

        case KEY_LEFT:
            nozzle.RotateDiffDurationBased(-angleIncr);
            break;

        case 'a':
        {
            int angle = 10;
            cin >> angle;
            nozzle.RotateDiffDurationBased(angle);
            break;
        }

        case 'q':
            res = 1;
            break;

        case 'o':
            nozzle.OpenValveAsync();
            break;

        case 'c':
            nozzle.CloseValveAsync();
            break;

        case 'n':
            nozzle.CalibrateNozzle(5000, 2'000'000, 1.3);
            message = "Nozzle calibration completed";
            break;

        case 'v':
            nozzle.CalibrateValve(20);
            message = "Nozzle calibration completed";
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
    Logger logger("create_zone.log");
    sprinkler.SetLogger(&logger);

    sprinkler.StartZoneRecording(zoneType);
    NozzleControl& nozzle = sprinkler.GetNozzleControl();

    {
        CursesWrapper cursesWrapper;
        if (cursesWrapper.Init() < 0)
        {
            return -1;
        }

        string message;

        SequenceTrendAnalyzer<16> pressureAnalyzer;

        while (true)
        {
            clear();

            mvprintw(0, 0, "Nozzle position: %d", nozzle.GetPositionFetchIfStale());
            int curPressure = nozzle.GetPressureFetchIfStale();
            pressureAnalyzer.Push(curPressure);
            mvprintw(1, 0, "Nozzle pressure: %d", curPressure);
            mvprintw(2, 0, "Nozzle avg pressure: %d", pressureAnalyzer.AvgValue());
            
            if (sprinkler.RecordedZone() != nullptr)
            {
                PrintZonePoints(*sprinkler.RecordedZone(), 3, 0);
            }

            mvprintw(LINES - 1, 0, "%s", message.c_str());
            refresh();

            if (ProcessInput(sprinkler, zoneFileName, message) != 0)
            {
                break;
            }
	    
	        this_thread::sleep_for(100ms);
        }
    }

    return 0;
}
