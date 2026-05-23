#include "libmap/map_layer.hpp"

#include <gtest/gtest.h>
#include <stdexcept>

using grid::libmap::MapLayer;

TEST(MapLayerTest, DefaultConstructIsEmpty)
{
    MapLayer<int> layer;
    EXPECT_EQ(layer.width(),  0u);
    EXPECT_EQ(layer.height(), 0u);
    EXPECT_TRUE(layer.empty());
}

TEST(MapLayerTest, ConstructWithFill)
{
    MapLayer<int> layer(3, 4, 7);
    EXPECT_EQ(layer.width(),  3u);
    EXPECT_EQ(layer.height(), 4u);
    EXPECT_FALSE(layer.empty());
    for (size_t z = 0; z < 4; ++z)
        for (size_t x = 0; x < 3; ++x)
            EXPECT_EQ(layer.at(x, z), 7);
}

TEST(MapLayerTest, SetAndGet)
{
    MapLayer<int> layer(5, 5, 0);
    layer.set(2, 3, 42);
    EXPECT_EQ(layer.get(2, 3), 42);
    EXPECT_EQ(layer.get(1, 3),  0);
}

TEST(MapLayerTest, GetOutOfBoundsReturnsDefault)
{
    MapLayer<int> layer(3, 3, 1);
    EXPECT_EQ(layer.get(10, 10), 0);  // default int{}
}

TEST(MapLayerTest, AtThrowsOutOfRange)
{
    MapLayer<int> layer(3, 3, 0);
    EXPECT_THROW(layer.at(3, 0), std::out_of_range);
    EXPECT_THROW(layer.at(0, 3), std::out_of_range);
}

TEST(MapLayerTest, InBounds)
{
    MapLayer<int> layer(4, 5, 0);
    EXPECT_TRUE(layer.inBounds(0, 0));
    EXPECT_TRUE(layer.inBounds(3, 4));
    EXPECT_FALSE(layer.inBounds(4, 0));
    EXPECT_FALSE(layer.inBounds(0, 5));
}

TEST(MapLayerTest, SetOutOfBoundsIsNoOp)
{
    MapLayer<int> layer(2, 2, 99);
    layer.set(5, 5, 0);  // must not throw or corrupt data
    EXPECT_EQ(layer.get(0, 0), 99);
}

TEST(MapLayerTest, CellsVectorSize)
{
    MapLayer<float> layer(6, 7, 0.0f);
    EXPECT_EQ(layer.cells().size(), 6u * 7u);
}

TEST(MapLayerTest, ModifyViaCellsRef)
{
    MapLayer<int> layer(2, 2, 0);
    layer.cells()[0] = 123;
    EXPECT_EQ(layer.at(0, 0), 123);
}
