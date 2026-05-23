#include <gtest/gtest.h>
#include "libsim/shapes.hpp"

#include <numbers>
#include <cmath>

using namespace grid::libsim;

using COORD = VectX<double, 2>;

class CircleTest : public ::testing::Test {
protected:
    COORD center{0.0, 0.0};
    double radius = 5.0;
    Circle<COORD> circle{center, radius};
};

TEST_F(CircleTest, AreaIsCorrect) {
    double expected = std::numbers::pi * 5.0 * 5.0;
    EXPECT_DOUBLE_EQ(circle.area(), expected);
}

TEST_F(CircleTest, PerimeterIsCorrect) {
    double expected = 2 * std::numbers::pi * 5.0;
    EXPECT_DOUBLE_EQ(circle.perimeter(), expected);
}

TEST_F(CircleTest, ContainsPointInside) {
    COORD inside{1.0, 1.0};
    EXPECT_TRUE(circle.containsPoint(inside));
}

TEST_F(CircleTest, ContainsCenter) {
    EXPECT_TRUE(circle.containsPoint(center));
}

TEST_F(CircleTest, ContainsPointOnBoundary) {
    COORD onEdge{5.0, 0.0};
    EXPECT_TRUE(circle.containsPoint(onEdge));
}

TEST_F(CircleTest, DoesNotContainPointOutside) {
    COORD outside{6.0, 0.0};
    EXPECT_FALSE(circle.containsPoint(outside));
}

TEST_F(CircleTest, DoesNotContainPointFarAway) {
    COORD farAway{100.0, 100.0};
    EXPECT_FALSE(circle.containsPoint(farAway));
}

TEST_F(CircleTest, UnitCircleArea) {
    Circle<COORD> unit{center, 1.0};
    EXPECT_DOUBLE_EQ(unit.area(), std::numbers::pi);
}

TEST_F(CircleTest, ZeroRadiusArea) {
    Circle<COORD> zero{center, 0.0};
    EXPECT_DOUBLE_EQ(zero.area(), 0.0);
    EXPECT_DOUBLE_EQ(zero.perimeter(), 0.0);
}

// ===========================================================================
// Rectangle tests
// ===========================================================================

class RectangleTest : public ::testing::Test {
protected:
    COORD center{0.0, 0.0};
    COORD halfExtents{5.0, 3.0};
    Rectangle<COORD> rect{center, halfExtents};
};

TEST_F(RectangleTest, AreaIsCorrect) {
    // width = 10, height = 6, area = 60
    EXPECT_DOUBLE_EQ(rect.area(), 60.0);
}

TEST_F(RectangleTest, PerimeterIsCorrect) {
    // width = 10, height = 6, perimeter = 2*(10+6) = 32
    EXPECT_DOUBLE_EQ(rect.perimeter(), 32.0);
}

TEST_F(RectangleTest, ContainsCenter) {
    EXPECT_TRUE(rect.containsPoint(center));
}

TEST_F(RectangleTest, ContainsPointInside) {
    COORD inside{2.0, 1.0};
    EXPECT_TRUE(rect.containsPoint(inside));
}

TEST_F(RectangleTest, ContainsPointOnEdge) {
    COORD onEdge{5.0, 0.0};
    EXPECT_TRUE(rect.containsPoint(onEdge));
}

TEST_F(RectangleTest, ContainsCorner) {
    COORD corner{5.0, 3.0};
    EXPECT_TRUE(rect.containsPoint(corner));
}

TEST_F(RectangleTest, DoesNotContainPointOutside) {
    COORD outside{6.0, 0.0};
    EXPECT_FALSE(rect.containsPoint(outside));
}

TEST_F(RectangleTest, DoesNotContainPointFarAway) {
    COORD farAway{100.0, 100.0};
    EXPECT_FALSE(rect.containsPoint(farAway));
}

TEST_F(RectangleTest, UnitSquareArea) {
    Rectangle<COORD> unit{center, COORD{0.5, 0.5}};
    EXPECT_DOUBLE_EQ(unit.area(), 1.0);
}

TEST_F(RectangleTest, ZeroSizeArea) {
    Rectangle<COORD> zero{center, COORD{0.0, 0.0}};
    EXPECT_DOUBLE_EQ(zero.area(), 0.0);
    EXPECT_DOUBLE_EQ(zero.perimeter(), 0.0);
}
