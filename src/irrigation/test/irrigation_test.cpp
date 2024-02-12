#include "PolygonApplicator.h"
#include "Zone.h"

#include <gtest/gtest.h>

#include <vector>

using namespace std;

TEST(Irrigation, Zones)
{
    const char fileName[] = "test_zone.bin";

    {
        Irrigation::Zone zone1(Irrigation::ZoneType::Points);

        zone1.AddPoint(1, 2);
        zone1.AddPoint(4, -7);
        zone1.AddPoint(3, -1);

        EXPECT_EQ(zone1.SaveToFile(fileName), 0);
    }

    {
        Irrigation::Zone zone2;
        EXPECT_EQ(zone2.LoadFromFile(fileName), 0);

        EXPECT_EQ(zone2.GetType(), Irrigation::ZoneType::Points);

        auto points = zone2.GetPoints();
        EXPECT_EQ(points.size(), 3);
        EXPECT_EQ(points[0], HwCoord(1, 2));
        EXPECT_EQ(points[1], HwCoord(4, -7));
        EXPECT_EQ(points[2], HwCoord(3, -1));
    }
}

TEST(Irrigation, PolygonApplication)
{
    vector<HwCoord> points = { {50, 1500}, {70, 1000}, {60, 500}, {100, 500},
                               {140, 800}, {140, 1200}, {90, 1500} };
    Irrigation::PolygonApplicator applicator(points);
    applicator.SetRIncrement(5.f);
    applicator.Begin();

    vector<HwArc> arcsExpected = { {55, 1500, 1225}, {60, 1500, 1123},  {65, 1500, 1053},
                                   {70, 1500, 1000}, {60, 754, 500},    {65, 912, 500},
                                   {70, 1000, 500},  {75, 1500, 500},   {80, 1500, 500},
                                   {85, 1500, 500},  {90, 1500, 500},   {95, 1443, 500},
                                   {100, 1398, 500}, {105, 1361, 567},  {110, 1329, 619},
                                   {115, 1301, 661}, {120, 1276, 696},  {125, 1254, 727},
                                   {130, 1234, 754}, {135, 1216, 1098}, {135, 901, 778} };

    auto it = arcsExpected.begin();
    for (; ; ++it)
    {
        HwArc arc = applicator.NextArc();
        if (!arc.IsValid()) break;
        
        if (it == arcsExpected.end())
        {
            FAIL() << "Expected values finished";
        }

        EXPECT_EQ(arc, *it);
    }

    EXPECT_EQ(it, arcsExpected.end());
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
