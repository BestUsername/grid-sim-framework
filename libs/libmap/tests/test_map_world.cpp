#include "libmap/map_world.hpp"

#include <gtest/gtest.h>
#include <stdexcept>

using namespace grid::libmap;

TEST(MapWorldTest, DefaultConstruct)
{
    MapWorld w;
    EXPECT_EQ(w.width(),  0u);
    EXPECT_EQ(w.height(), 0u);
    EXPECT_FALSE(w.hasGeoOrigin());
    EXPECT_DOUBLE_EQ(w.cellSize(), 1.0);
}

TEST(MapWorldTest, ConstructWithSize)
{
    MapWorld w(10, 20, 2.5);
    EXPECT_EQ(w.width(),  10u);
    EXPECT_EQ(w.height(), 20u);
    EXPECT_DOUBLE_EQ(w.cellSize(), 2.5);
}

TEST(MapWorldTest, TerrainLayerDefaultsToLand)
{
    MapWorld w(4, 4);
    EXPECT_EQ(w.terrain().get(0, 0).surface, SurfaceType::Land);
}

TEST(MapWorldTest, TerrainAccessible)
{
    MapWorld w(5, 5);
    TerrainTile tile;
    tile.surface = SurfaceType::Water;
    w.terrain().set(2, 2, tile);
    EXPECT_EQ(w.terrain().get(2, 2).surface, SurfaceType::Water);
    // Other cells are untouched
    EXPECT_EQ(w.terrain().get(0, 0).surface, SurfaceType::Land);
}

TEST(MapWorldTest, SetGeoOrigin)
{
    MapWorld w(10, 10);
    EXPECT_FALSE(w.hasGeoOrigin());
    w.setGeoOrigin({51.5, -0.1, 5.0});
    EXPECT_TRUE(w.hasGeoOrigin());
    EXPECT_DOUBLE_EQ(w.geoOrigin().lat, 51.5);
    EXPECT_DOUBLE_EQ(w.geoOrigin().metersPerCell, 5.0);
}

TEST(MapWorldTest, ProjectionThrowsWithoutOrigin)
{
    MapWorld w(10, 10);
    EXPECT_THROW(w.projection(), std::runtime_error);
}

TEST(MapWorldTest, ProjectionWorksWithOrigin)
{
    MapWorld w(10, 10);
    w.setGeoOrigin({48.85, 2.35, 1.0});
    EXPECT_NO_THROW(w.projection());
}

TEST(MapWorldTest, NameRoundTrip)
{
    MapWorld w;
    w.setName("TestMap");
    EXPECT_EQ(w.name(), "TestMap");
}

TEST(MapWorldTest, SetCellSize)
{
    MapWorld w(4, 4, 1.0);
    w.setCellSize(10.0);
    EXPECT_DOUBLE_EQ(w.cellSize(), 10.0);
}
