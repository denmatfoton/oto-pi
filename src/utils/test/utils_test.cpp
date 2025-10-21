#include "MathUtils.h"

#include <gtest/gtest.h>

using namespace std;

// Test OvershootInterpolator with different template parameters
using TestOvershootInterpolator = OvershootInterpolator<10, 1000, int64_t>;

TEST(OvershootInterpolator, BasicSetAndPredict)
{
    TestOvershootInterpolator interpolator;
    
    // Set overshoot for rate=100, overshoot=50
    interpolator.SetOvershoot(100, 50);
    
    // Predict for the same rate should return approximately the same overshoot
    int predicted = interpolator.PredictOvershoot(100);
    EXPECT_NEAR(predicted, 50, 5);
}

TEST(OvershootInterpolator, ZeroRatePrediction)
{
    TestOvershootInterpolator interpolator;
    
    // Set some values
    interpolator.SetOvershoot(100, 50);
    interpolator.SetOvershoot(200, 150);
    
    // Predict for rate=0 should return 0
    int predicted = interpolator.PredictOvershoot(0);
    EXPECT_EQ(predicted, 0);
}

TEST(OvershootInterpolator, QuadraticScaling)
{
    TestOvershootInterpolator interpolator;
    
    // Set overshoot for rate=100, overshoot=100
    interpolator.SetOvershoot(100, 100);
    
    // Predict for rate=200 should be ~400 (quadratic: 200^2 / 100^2 * 100 = 400)
    int predicted = interpolator.PredictOvershoot(200);
    EXPECT_NEAR(predicted, 400, 50);
    
    // Predict for rate=50 should be ~25 (quadratic: 50^2 / 100^2 * 100 = 25)
    predicted = interpolator.PredictOvershoot(50);
    EXPECT_NEAR(predicted, 25, 10);
}

TEST(OvershootInterpolator, NegativeRates)
{
    TestOvershootInterpolator interpolator;
    
    // Set overshoot for negative rate
    interpolator.SetOvershoot(-100, 50);
    
    // Predict for negative rate
    int predicted = interpolator.PredictOvershoot(-100);
    EXPECT_NEAR(predicted, 50, 5);
    
    // Predict for different negative rate (quadratic scaling)
    predicted = interpolator.PredictOvershoot(-200);
    EXPECT_NEAR(predicted, 200, 50);
}

TEST(OvershootInterpolator, SymmetricBehavior)
{
    TestOvershootInterpolator interpolator;
    
    // Set overshoot for positive and negative rates
    interpolator.SetOvershoot(100, 100);
    interpolator.SetOvershoot(-100, 100);
    
    // Predictions should be symmetric
    int predictedPos = interpolator.PredictOvershoot(150);
    int predictedNeg = interpolator.PredictOvershoot(-150);
    
    EXPECT_NEAR(predictedPos, predictedNeg, 10);
}

TEST(OvershootInterpolator, OutOfRangePrediction)
{
    TestOvershootInterpolator interpolator;
    
    // Set overshoot within range
    interpolator.SetOvershoot(100, 100);
    
    // Predict for rate beyond maxRate (1000)
    int predicted = interpolator.PredictOvershoot(1500);
    EXPECT_GT(predicted, 0);  // Should still predict something
    
    // Predict for rate below -maxRate
    predicted = interpolator.PredictOvershoot(-1500);
    EXPECT_GT(predicted, 0);
}

TEST(OvershootInterpolator, MultipleSetsSameRate)
{
    TestOvershootInterpolator interpolator;
    
    // Set overshoot for same rate twice
    interpolator.SetOvershoot(100, 100);
    interpolator.SetOvershoot(100, 200);
    
    // Should average the values: (100 + 200) / 2 = 150
    int predicted = interpolator.PredictOvershoot(100);
    EXPECT_NEAR(predicted, 150, 10);
}

TEST(OvershootInterpolator, InterpolationBetweenPoints)
{
    TestOvershootInterpolator interpolator;
    
    // Set two points
    interpolator.SetOvershoot(100, 100);
    interpolator.SetOvershoot(400, 1600);
    
    // Predict for a rate between them (200)
    // Quadratic from 100: 200^2 / 100^2 * 100 = 400
    // Quadratic from 400: 200^2 / 400^2 * 1600 = 400
    int predicted = interpolator.PredictOvershoot(200);
    EXPECT_NEAR(predicted, 400, 50);
}

TEST(OvershootInterpolator, EdgeCaseExactlyAtMaxRate)
{
    TestOvershootInterpolator interpolator;
    
    // Set overshoot exactly at maxRate (1000)
    interpolator.SetOvershoot(1000, 500);
    
    // Predict for maxRate
    int predicted = interpolator.PredictOvershoot(1000);
    EXPECT_NEAR(predicted, 500, 10);
}

TEST(OvershootInterpolator, EdgeCaseExactlyAtMinusMaxRate)
{
    TestOvershootInterpolator interpolator;
    
    // Set overshoot exactly at -maxRate (-1000)
    interpolator.SetOvershoot(-1000, 500);
    
    // Predict for -maxRate
    int predicted = interpolator.PredictOvershoot(-1000);
    EXPECT_NEAR(predicted, 500, 10);
}

TEST(OvershootInterpolator, EmptyInterpolator)
{
    TestOvershootInterpolator interpolator;
    
    // Predict without setting any values
    int predicted = interpolator.PredictOvershoot(100);
    EXPECT_EQ(predicted, 0);
}

TEST(OvershootInterpolator, ExactBucketBoundary)
{
    TestOvershootInterpolator interpolator;
    
    // With n=10, maxRate=1000, resolution=100
    // Rates at exact boundaries: -1000, -900, -800, ..., 0, ..., 800, 900, 1000
    
    // Set overshoot at exact boundary
    interpolator.SetOvershoot(0, 0);
    interpolator.SetOvershoot(100, 50);
    
    // Predict at exact boundary
    int predicted = interpolator.PredictOvershoot(100);
    EXPECT_NEAR(predicted, 50, 5);
}

TEST(OvershootInterpolator, SmallRateValues)
{
    TestOvershootInterpolator interpolator;
    
    // Set overshoot for small rates
    interpolator.SetOvershoot(10, 5);
    
    // Predict for small rate
    int predicted = interpolator.PredictOvershoot(10);
    EXPECT_NEAR(predicted, 5, 2);
    
    // Predict for even smaller rate
    predicted = interpolator.PredictOvershoot(5);
    EXPECT_NEAR(predicted, 1, 1);
}

TEST(OvershootInterpolator, LargeOvershootValues)
{
    using LargeOvershootInterpolator = OvershootInterpolator<10, 1000, int64_t>;
    LargeOvershootInterpolator interpolator;
    
    // Set large overshoot values
    interpolator.SetOvershoot(500, 10000);
    
    // Predict should handle large values correctly
    int predicted = interpolator.PredictOvershoot(500);
    EXPECT_NEAR(predicted, 10000, 500);
}

TEST(OvershootInterpolator, AveragingBehavior)
{
    TestOvershootInterpolator interpolator;
    
    // Set same rate multiple times with different overshoots
    interpolator.SetOvershoot(200, 100);
    interpolator.SetOvershoot(200, 300);
    interpolator.SetOvershoot(200, 200);
    
    // Should progressively average: 100, (100+300)/2=200, (200+200)/2=200
    int predicted = interpolator.PredictOvershoot(200);
    EXPECT_NEAR(predicted, 200, 20);
}

// Test SequenceTrendAnalyzer
TEST(SequenceTrendAnalyzer, BasicPushAndAverage)
{
    SequenceTrendAnalyzer<5> analyzer;
    
    analyzer.Push(10);
    analyzer.Push(20);
    analyzer.Push(30);
    
    EXPECT_EQ(analyzer.Size(), 3);
    EXPECT_FALSE(analyzer.IsFull());
    EXPECT_FALSE(analyzer.IsEmpty());
}

TEST(SequenceTrendAnalyzer, TrendDetection)
{
    SequenceTrendAnalyzer<10> analyzer;
    
    // Push increasing values
    for (int i = 0; i < 10; ++i)
    {
        analyzer.Push(i * 10);
    }
    
    EXPECT_TRUE(analyzer.IsFull());
    
    // Trend should be positive
    int trend = analyzer.CurTrend(5);
    EXPECT_GT(trend, 0);
}

TEST(SequenceTrendAnalyzer, DecreasingTrend)
{
    SequenceTrendAnalyzer<10> analyzer;
    
    // Push decreasing values
    for (int i = 0; i < 10; ++i)
    {
        analyzer.Push(100 - i * 10);
    }
    
    // Trend should be negative
    int trend = analyzer.CurTrend(5);
    EXPECT_LT(trend, 0);
}

TEST(SequenceTrendAnalyzer, CircularBuffer)
{
    SequenceTrendAnalyzer<5> analyzer;
    
    // Push more than capacity
    for (int i = 0; i < 10; ++i)
    {
        analyzer.Push(i);
    }
    
    // Should still be full (overwriting old values)
    EXPECT_TRUE(analyzer.IsFull());
    EXPECT_EQ(analyzer.Size(), 5);
}

TEST(SequenceTrendAnalyzer, Clear)
{
    SequenceTrendAnalyzer<5> analyzer;
    
    analyzer.Push(10);
    analyzer.Push(20);
    
    analyzer.Clear();
    
    EXPECT_TRUE(analyzer.IsEmpty());
    EXPECT_EQ(analyzer.Size(), 0);
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
