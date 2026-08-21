#include "libmap/ascii_format.hpp"

#include <gtest/gtest.h>
#include <sstream>

using namespace grid::libmap;

static const char* kSimpleMap =
    "4 3\n"
    "~~~~\n"
    "....\n"
    "^^^^\n";

TEST(AsciiFormatTest, ReadFromString)
{
    AsciiMapFormat fmt;
    auto result = fmt.readFromString(kSimpleMap);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->width(),  4u);
    EXPECT_EQ(result->height(), 3u);
    EXPECT_EQ(result->terrain().get(0, 0).surface, SurfaceType::Water);
    EXPECT_EQ(result->terrain().get(0, 1).surface, SurfaceType::Land);
    EXPECT_EQ(result->terrain().get(0, 2).surface, SurfaceType::Mountain);
}

TEST(AsciiFormatTest, WriteToString)
{
    AsciiMapFormat fmt;
    MapWorld world(4, 3);
    for (size_t x = 0; x < 4; ++x) {
        world.terrain().set(x, 0, {SurfaceType::Water,    0.f, {}});
        world.terrain().set(x, 1, {SurfaceType::Land,     0.f, {}});
        world.terrain().set(x, 2, {SurfaceType::Mountain, 0.f, {}});
    }
    EXPECT_EQ(fmt.writeToString(world), kSimpleMap);
}

TEST(AsciiFormatTest, RoundTrip)
{
    AsciiMapFormat fmt;
    auto original = fmt.readFromString(kSimpleMap);
    ASSERT_TRUE(original.has_value());

    std::string serialised = fmt.writeToString(*original);
    auto restored = fmt.readFromString(serialised);
    ASSERT_TRUE(restored.has_value());

    ASSERT_EQ(restored->width(),  original->width());
    ASSERT_EQ(restored->height(), original->height());
    for (size_t z = 0; z < original->height(); ++z)
        for (size_t x = 0; x < original->width(); ++x)
            EXPECT_EQ(restored->terrain().get(x, z).surface,
                      original->terrain().get(x, z).surface)
                << "Mismatch at (" << x << ", " << z << ")";
}

TEST(AsciiFormatTest, InvalidHeaderReturnsNullopt)
{
    AsciiMapFormat fmt;
    EXPECT_FALSE(fmt.readFromString("not a map").has_value());
    EXPECT_FALSE(fmt.readFromString("0 5\n").has_value());
}

TEST(AsciiFormatTest, UnknownCharDefaultsToLand)
{
    AsciiMapFormat fmt;
    auto result = fmt.readFromString("2 1\nX@");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->terrain().get(0, 0).surface, SurfaceType::Land);
    EXPECT_EQ(result->terrain().get(1, 0).surface, SurfaceType::Land);
}

TEST(AsciiFormatTest, ShortRowPaddedWithLand)
{
    AsciiMapFormat fmt;
    // Row 1 is shorter than declared width
    auto result = fmt.readFromString("4 1\n~~");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->terrain().get(0, 0).surface, SurfaceType::Water);
    EXPECT_EQ(result->terrain().get(1, 0).surface, SurfaceType::Water);
    EXPECT_EQ(result->terrain().get(2, 0).surface, SurfaceType::Land);
    EXPECT_EQ(result->terrain().get(3, 0).surface, SurfaceType::Land);
}

TEST(AsciiFormatTest, PreservesBattleGridSlopeAndHillTiles)
{
    AsciiMapFormat fmt;
    auto result = fmt.readFromString("5 1\nNSEWH");
    ASSERT_TRUE(result.has_value());
    const auto& terrain = result->terrain();
    EXPECT_EQ(terrain.get(0, 0).tags.at("battlegrid:slope"), "N");
    EXPECT_EQ(terrain.get(1, 0).tags.at("battlegrid:slope"), "S");
    EXPECT_EQ(terrain.get(2, 0).tags.at("battlegrid:slope"), "E");
    EXPECT_EQ(terrain.get(3, 0).tags.at("battlegrid:slope"), "W");
    EXPECT_FLOAT_EQ(terrain.get(0, 0).elevation, 0.25f);
    EXPECT_FLOAT_EQ(terrain.get(4, 0).elevation, 0.5f);
    EXPECT_EQ(fmt.writeToString(*result), "5 1\nNSEWH\n");
}

TEST(AsciiFormatTest, MissingRowReturnsNullopt)
{
    AsciiMapFormat fmt;
    EXPECT_FALSE(fmt.readFromString("3 3\n...\n...").has_value());
}

TEST(AsciiFormatTest, ReadFromNonexistentFileReturnsNullopt)
{
    AsciiMapFormat fmt;
    EXPECT_FALSE(fmt.readFromFile("/no/such/file.map").has_value());
}
