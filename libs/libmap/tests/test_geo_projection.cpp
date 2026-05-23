#include "libmap/geo_projection.hpp"

#include <gtest/gtest.h>
#include <cmath>

using grid::libmap::GeoOrigin;
using grid::libmap::GeoProjection;

TEST(GeoProjectionTest, OriginMapsToZero)
{
    GeoProjection proj(GeoOrigin{51.5, -0.1, 1.0});
    auto [x, z] = proj.toLocal(51.5, -0.1);
    EXPECT_NEAR(x, 0.0, 1e-9);
    EXPECT_NEAR(z, 0.0, 1e-9);
}

TEST(GeoProjectionTest, EastIsPositiveX)
{
    GeoProjection proj(GeoOrigin{0.0, 0.0, 1.0});
    auto [x, z] = proj.toLocal(0.0, 0.1);
    EXPECT_GT(x, 0.0);
    EXPECT_NEAR(z, 0.0, 0.01);
}

TEST(GeoProjectionTest, SouthIsPositiveZ)
{
    GeoProjection proj(GeoOrigin{0.0, 0.0, 1.0});
    auto [x, z] = proj.toLocal(-0.1, 0.0);
    EXPECT_GT(z, 0.0);
    EXPECT_NEAR(x, 0.0, 0.01);
}

TEST(GeoProjectionTest, RoundTrip)
{
    // Paris, 10 m/cell
    GeoProjection proj(GeoOrigin{48.85, 2.35, 10.0});
    double lat = 48.86, lon = 2.36;
    auto [x, z]     = proj.toLocal(lat, lon);
    auto [lat2, lon2] = proj.toGeo(x, z);
    EXPECT_NEAR(lat2, lat, 1e-9);
    EXPECT_NEAR(lon2, lon, 1e-9);
}

TEST(GeoProjectionTest, CellScaleAffectsOutputInversely)
{
    // 1 m/cell → many cells for a fixed offset
    // 100 m/cell → fewer cells for the same offset
    GeoProjection proj1(GeoOrigin{0.0, 0.0,   1.0});
    GeoProjection proj2(GeoOrigin{0.0, 0.0, 100.0});
    auto [x1, z1] = proj1.toLocal(0.0, 0.01);
    auto [x2, z2] = proj2.toLocal(0.0, 0.01);
    EXPECT_NEAR(x1, x2 * 100.0, 0.01 * std::abs(x1));
}

TEST(GeoProjectionTest, KnownDistanceOneDegreeLat)
{
    // One degree of latitude ≈ 111 km at the equator.
    GeoProjection proj(GeoOrigin{0.0, 0.0, 1.0});
    auto [x, z] = proj.toLocal(-1.0, 0.0);
    EXPECT_NEAR(z, 111'195.0, 500.0);  // within 500 m
}

TEST(GeoProjectionTest, OriginAccessor)
{
    GeoOrigin orig{10.0, 20.0, 5.0};
    GeoProjection proj(orig);
    EXPECT_DOUBLE_EQ(proj.origin().lat, 10.0);
    EXPECT_DOUBLE_EQ(proj.origin().lon, 20.0);
    EXPECT_DOUBLE_EQ(proj.origin().metersPerCell, 5.0);
}
