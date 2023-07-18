#include "Zone.h"

#include <gtest/gtest.h>

TEST(Irrigation, Zones)
{
    const char fileName[] = "test_zone.bin";

    {
        Zone zone1(ZoneType::Points);

        zone1.AddPoint(1, 2);
        zone1.AddPoint(4, -7);
        zone1.AddPoint(3, -1);

        EXPECT_EQ(zone1.SaveToFile(fileName), 0);
    }

    {
        Zone zone2;
        EXPECT_EQ(zone2.LoadFromFile(fileName), 0);

        EXPECT_EQ(zone2.GetType(), ZoneType::Points);

        auto points = zone2.GetPoints();
        EXPECT_EQ(points.size(), 3);
        EXPECT_EQ(points[0], Coord(1, 2));
        EXPECT_EQ(points[1], Coord(4, -7));
        EXPECT_EQ(points[2], Coord(3, -1));
    }
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
