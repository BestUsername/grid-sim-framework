#include <gtest/gtest.h>
#include "libsim/types.hpp"

using namespace grid::libsim;

using COORD = VectX<int, 2>;
using RANGE = RangeXD<int, 2>;

class RangeXDTest : public ::testing::Test {
protected:
    // Range covering x:[0, 10], y:[0, 10]
    RANGE range{{{0, 10}, {0, 10}}};
};

TEST_F(RangeXDTest, ContainsPointInside) {
    COORD point{5, 5};
    EXPECT_TRUE(range.contains(point));
}

TEST_F(RangeXDTest, ContainsOrigin) {
    COORD point{0, 0};
    EXPECT_TRUE(range.contains(point));
}

TEST_F(RangeXDTest, ContainsMaxCorner) {
    COORD point{10, 10};
    EXPECT_TRUE(range.contains(point));
}

TEST_F(RangeXDTest, ContainsEdgePoints) {
    EXPECT_TRUE(range.contains(COORD{0, 5}));
    EXPECT_TRUE(range.contains(COORD{10, 5}));
    EXPECT_TRUE(range.contains(COORD{5, 0}));
    EXPECT_TRUE(range.contains(COORD{5, 10}));
}

TEST_F(RangeXDTest, DoesNotContainPointOutsideX) {
    COORD point{11, 5};
    EXPECT_FALSE(range.contains(point));
}

TEST_F(RangeXDTest, DoesNotContainPointOutsideY) {
    COORD point{5, 11};
    EXPECT_FALSE(range.contains(point));
}

TEST_F(RangeXDTest, DoesNotContainNegativePoint) {
    COORD point{-1, 5};
    EXPECT_FALSE(range.contains(point));
}

TEST_F(RangeXDTest, DoesNotContainPointBothOutside) {
    COORD point{-1, -1};
    EXPECT_FALSE(range.contains(point));
}

TEST(RangeXD3DTest, Contains3DPoint) {
    using COORD3 = VectX<int, 3>;
    using RANGE3 = RangeXD<int, 3>;
    RANGE3 range{{{0, 10}, {0, 10}, {0, 10}}};

    EXPECT_TRUE(range.contains(COORD3{5, 5, 5}));
    EXPECT_FALSE(range.contains(COORD3{5, 5, 11}));
}
