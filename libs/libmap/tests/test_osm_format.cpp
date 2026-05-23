#include "libmap/osm_format.hpp"
#include "libmap/map_tile.hpp"
#include "libmap/map_world.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

// Path injected by CMake so tests find the fixture regardless of CWD.
#ifndef LIBMAP_TEST_FIXTURE_DIR
#  define LIBMAP_TEST_FIXTURE_DIR "."
#endif

namespace grid::libmap {
namespace {

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string slurp(const std::string& path)
{
    std::ifstream f(path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static bool hasAnySurface(const MapWorld& world, SurfaceType want)
{
    for (size_t z = 0; z < world.height(); ++z)
        for (size_t x = 0; x < world.width(); ++x)
            if (world.terrain().get(x, z).surface == want)
                return true;
    return false;
}

static const std::string kFixtureFile =
    std::string(LIBMAP_TEST_FIXTURE_DIR) + "/small.osm";

// ── Tests ─────────────────────────────────────────────────────────────────────

TEST(OsmFormatTest, ReadFromFileSucceeds)
{
    OsmFormat fmt;
    auto world = fmt.readFromFile(kFixtureFile);
    ASSERT_TRUE(world.has_value()) << "readFromFile returned nullopt";
}

TEST(OsmFormatTest, DimensionsAreReasonable)
{
    OsmFormat fmt;
    auto world = fmt.readFromFile(kFixtureFile);
    ASSERT_TRUE(world.has_value());
    // Bounding box is ~385 m wide × ~500 m tall with cellSize=10 m.
    EXPECT_GE(world->width(),  20u);
    EXPECT_GE(world->height(), 20u);
    EXPECT_LE(world->width(),  200u);
    EXPECT_LE(world->height(), 200u);
}

TEST(OsmFormatTest, GeoOriginIsSet)
{
    OsmFormat fmt;
    auto world = fmt.readFromFile(kFixtureFile);
    ASSERT_TRUE(world.has_value());
    EXPECT_TRUE(world->hasGeoOrigin());
}

TEST(OsmFormatTest, RoadCellsPresent)
{
    OsmFormat fmt;
    auto world = fmt.readFromFile(kFixtureFile);
    ASSERT_TRUE(world.has_value());
    EXPECT_TRUE(hasAnySurface(*world, SurfaceType::Road))
        << "Expected at least one Road cell from highway=primary/motorway ways";
}

TEST(OsmFormatTest, WaterCellsPresent)
{
    OsmFormat fmt;
    auto world = fmt.readFromFile(kFixtureFile);
    ASSERT_TRUE(world.has_value());
    EXPECT_TRUE(hasAnySurface(*world, SurfaceType::Water))
        << "Expected at least one Water cell from natural=water polygon";
}

TEST(OsmFormatTest, BuildingCellsPresent)
{
    OsmFormat fmt;
    auto world = fmt.readFromFile(kFixtureFile);
    ASSERT_TRUE(world.has_value());
    EXPECT_TRUE(hasAnySurface(*world, SurfaceType::Building))
        << "Expected at least one Building cell from building=yes polygon";
}

TEST(OsmFormatTest, ForestCellsPresent)
{
    OsmFormat fmt;
    auto world = fmt.readFromFile(kFixtureFile);
    ASSERT_TRUE(world.has_value());
    EXPECT_TRUE(hasAnySurface(*world, SurfaceType::Forest))
        << "Expected at least one Forest cell from landuse=forest polygon";
}

TEST(OsmFormatTest, CellSizeStoredInWorld)
{
    OsmFormat::Options opts;
    opts.cellSize = 5.0;
    OsmFormat fmt(opts);
    auto world = fmt.readFromFile(kFixtureFile);
    ASSERT_TRUE(world.has_value());
    EXPECT_DOUBLE_EQ(world->cellSize(), 5.0);
    // Smaller cells → larger grid.
    OsmFormat fmt10;
    auto world10 = fmt10.readFromFile(kFixtureFile);
    ASSERT_TRUE(world10.has_value());
    EXPECT_GT(world->width(),  world10->width());
    EXPECT_GT(world->height(), world10->height());
}

TEST(OsmFormatTest, ReadFromStringMatchesFile)
{
    const std::string xml = slurp(kFixtureFile);
    ASSERT_FALSE(xml.empty());

    OsmFormat fmt;
    auto fromFile   = fmt.readFromFile(kFixtureFile);
    auto fromString = fmt.readFromString(xml);
    ASSERT_TRUE(fromFile.has_value());
    ASSERT_TRUE(fromString.has_value());
    EXPECT_EQ(fromFile->width(),  fromString->width());
    EXPECT_EQ(fromFile->height(), fromString->height());
}

TEST(OsmFormatTest, MissingFileReturnsNullopt)
{
    OsmFormat fmt;
    auto world = fmt.readFromFile("/no/such/file.osm");
    EXPECT_FALSE(world.has_value());
}

TEST(OsmFormatTest, InvalidXmlReturnsNullopt)
{
    OsmFormat fmt;
    auto world = fmt.readFromString("this is not xml <<<");
    EXPECT_FALSE(world.has_value());
}

TEST(OsmFormatTest, NoOsmRootReturnsNullopt)
{
    OsmFormat fmt;
    auto world = fmt.readFromString(R"(<root><node id="1" lat="0" lon="0"/></root>)");
    EXPECT_FALSE(world.has_value());
}

TEST(OsmFormatTest, NoNodesReturnsNullopt)
{
    OsmFormat fmt;
    auto world = fmt.readFromString(R"(<osm version="0.6"></osm>)");
    EXPECT_FALSE(world.has_value());
}

TEST(OsmFormatTest, WaysWithUnknownTagsAreSkipped)
{
    // A way with no recognised tags should not produce any non-Land cells.
    OsmFormat fmt;
    const std::string xml = R"(
<osm version="0.6">
  <node id="1" lat="51.5000" lon="-0.1000"/>
  <node id="2" lat="51.5001" lon="-0.1000"/>
  <way id="1">
    <nd ref="1"/>
    <nd ref="2"/>
    <tag k="some_unknown_key" v="value"/>
  </way>
</osm>)";
    auto world = fmt.readFromString(xml);
    ASSERT_TRUE(world.has_value());
    EXPECT_FALSE(hasAnySurface(*world, SurfaceType::Road));
    EXPECT_FALSE(hasAnySurface(*world, SurfaceType::Water));
    EXPECT_FALSE(hasAnySurface(*world, SurfaceType::Building));
}

} // namespace
} // namespace grid::libmap
