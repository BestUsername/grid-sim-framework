#include "libmap/osm_format.hpp"
#include "libmap/geo_projection.hpp"
#include "libmap/map_layer.hpp"
#include "libmap/map_tile.hpp"
#include "libmap/map_world.hpp"

#include <tinyxml2.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace grid::libmap {
namespace {

// ── Internal data structures ──────────────────────────────────────────────────

struct OsmNode {
    double lat = 0.0;
    double lon = 0.0;
};

struct OsmWay {
    std::vector<int64_t>                         refs;
    std::unordered_map<std::string, std::string> tags;
};

// ── Surface classification helpers ────────────────────────────────────────────

/// Higher value = wins when surfaces overlap.
static int surfacePriority(SurfaceType s)
{
    switch (s) {
    case SurfaceType::Building: return 7;
    case SurfaceType::Road:     return 6;
    case SurfaceType::Water:    return 5;
    case SurfaceType::Forest:   return 4;
    case SurfaceType::Swamp:    return 3;
    case SurfaceType::Sand:     return 2;
    case SurfaceType::Mountain: return 1;
    default:                    return 0;
    }
}

/// Map OSM tags to the most specific matching `SurfaceType`, or `Unknown` if
/// no recognised tag is present.  Checks in priority order: building, highway/
/// railway, natural, waterway, landuse.
static SurfaceType surfaceFromTags(const std::unordered_map<std::string, std::string>& tags)
{
    if (tags.count("building"))
        return SurfaceType::Building;

    if (tags.count("highway") || tags.count("railway"))
        return SurfaceType::Road;

    {
        auto it = tags.find("natural");
        if (it != tags.end()) {
            const auto& v = it->second;
            if (v == "water"   || v == "bay")               return SurfaceType::Water;
            if (v == "wood"    || v == "tree_row")           return SurfaceType::Forest;
            if (v == "sand"    || v == "beach")              return SurfaceType::Sand;
            if (v == "wetland" || v == "marsh")              return SurfaceType::Swamp;
            if (v == "peak"    || v == "cliff" || v == "rock") return SurfaceType::Mountain;
        }
    }

    if (tags.count("waterway"))
        return SurfaceType::Water;

    {
        auto it = tags.find("landuse");
        if (it != tags.end()) {
            const auto& v = it->second;
            if (v == "forest" || v == "orchard" || v == "vineyard") return SurfaceType::Forest;
            if (v == "basin"  || v == "reservoir")                   return SurfaceType::Water;
        }
    }

    return SurfaceType::Unknown;
}

/// True when a closed way should be rasterised as a filled area polygon.
static bool isArea(const std::unordered_map<std::string, std::string>& tags, bool closed)
{
    if (!closed) return false;

    if (tags.count("building") || tags.count("landuse")) return true;

    {
        auto it = tags.find("area");
        if (it != tags.end() && it->second == "yes") return true;
    }
    {
        auto it = tags.find("natural");
        if (it != tags.end()) {
            const auto& v = it->second;
            if (v == "water"   || v == "wood"    ||
                v == "sand"    || v == "beach"   ||
                v == "wetland" || v == "marsh")
                return true;
        }
    }
    {
        auto it = tags.find("waterway");
        if (it != tags.end()) {
            const auto& v = it->second;
            if (v == "riverbank" || v == "dock" || v == "basin") return true;
        }
    }
    return false;
}

/// How many cells wide to draw a road/linear feature.
static int lineWidth(const std::unordered_map<std::string, std::string>& tags)
{
    auto it = tags.find("highway");
    if (it != tags.end()) {
        const auto& v = it->second;
        if (v == "motorway" || v == "trunk"    || v == "primary")   return 3;
        if (v == "secondary"|| v == "tertiary")                      return 2;
    }
    return 1;
}

// ── Rasterisation primitives ──────────────────────────────────────────────────

/// Set a cell only if the new surface has higher priority than the existing one.
static void trySet(MapLayer<TerrainTile>& layer, int x, int z, SurfaceType surf)
{
    if (x < 0 || z < 0) return;
    auto ux = static_cast<size_t>(x);
    auto uz = static_cast<size_t>(z);
    if (!layer.inBounds(ux, uz)) return;
    auto& tile = layer.at(ux, uz);
    if (surfacePriority(surf) > surfacePriority(tile.surface))
        tile.surface = surf;
}

/// Bresenham line with optional half-width perpendicular spread.
static void rasterLine(MapLayer<TerrainTile>& layer,
                       int x0, int z0, int x1, int z1,
                       SurfaceType surf, int width)
{
    int dx  = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dz  = std::abs(z1 - z0), sz = z0 < z1 ? 1 : -1;
    int err = (dx > dz ? dx : -dz) / 2;
    int hw  = width / 2;

    for (;;) {
        if (dx >= dz)
            for (int d = -hw; d <= hw; ++d) trySet(layer, x0, z0 + d, surf);
        else
            for (int d = -hw; d <= hw; ++d) trySet(layer, x0 + d, z0, surf);

        if (x0 == x1 && z0 == z1) break;
        int e2 = err;
        if (e2 > -dx) { err -= dz; x0 += sx; }
        if (e2 <  dz) { err += dx; z0 += sz; }
    }
}

/// Scanline polygon fill followed by an outline pass for the edges.
static void rasterPoly(MapLayer<TerrainTile>& layer,
                       const std::vector<std::pair<int, int>>& pts,
                       SurfaceType surf)
{
    if (pts.size() < 3) return;

    int minZ = std::numeric_limits<int>::max();
    int maxZ = std::numeric_limits<int>::min();
    for (auto& p : pts) {
        minZ = std::min(minZ, p.second);
        maxZ = std::max(maxZ, p.second);
    }
    minZ = std::max(minZ, 0);
    maxZ = std::min(maxZ, static_cast<int>(layer.height()) - 1);

    for (int z = minZ; z <= maxZ; ++z) {
        std::vector<int> xs;
        size_t n = pts.size();
        for (size_t i = 0, j = n - 1; i < n; j = i++) {
            int y0 = pts[j].second, y1 = pts[i].second;
            if ((y0 <= z && y1 > z) || (y1 <= z && y0 > z)) {
                int x0 = pts[j].first, x1 = pts[i].first;
                xs.push_back(x0 + (z - y0) * (x1 - x0) / (y1 - y0));
            }
        }
        std::sort(xs.begin(), xs.end());
        for (size_t i = 0; i + 1 < xs.size(); i += 2) {
            int xStart = std::max(0, xs[i]);
            int xEnd   = std::min(static_cast<int>(layer.width()) - 1, xs[i + 1]);
            for (int x = xStart; x <= xEnd; ++x)
                trySet(layer, x, z, surf);
        }
    }

    // Outline edges (ensures thin polygons are visible).
    for (size_t i = 0, j = pts.size() - 1; i < pts.size(); j = i++)
        rasterLine(layer, pts[j].first, pts[j].second, pts[i].first, pts[i].second, surf, 1);
}

// ── Tag parsing ───────────────────────────────────────────────────────────────

/// Collect all `<tag k="..." v="..."/>` children of @p parent into a map.
static std::unordered_map<std::string, std::string>
collectTags(const tinyxml2::XMLElement* parent)
{
    std::unordered_map<std::string, std::string> tags;
    for (auto* el = parent->FirstChildElement("tag"); el;
         el       = el->NextSiblingElement("tag")) {
        const char* k = el->Attribute("k");
        const char* v = el->Attribute("v");
        if (k && v) tags[k] = v;
    }
    return tags;
}

// ── Core parser ───────────────────────────────────────────────────────────────

/// Parse an already-loaded tinyxml2 document and rasterise it into a new
/// `MapWorld` with cells of @p cellSize metres.  Returns `std::nullopt` on
/// structural errors (no `<osm>` root, no `<node>` elements).
static std::optional<MapWorld> buildFromDoc(const tinyxml2::XMLDocument& doc,
                                            double cellSize)
{
    const auto* root = doc.FirstChildElement("osm");
    if (!root) {
        std::cerr << "OsmFormat: missing <osm> root element\n";
        return std::nullopt;
    }

    // ── 1. Parse all <node> elements ─────────────────────────────────────────
    std::unordered_map<int64_t, OsmNode> nodes;
    double minLat =  90.0, maxLat = -90.0;
    double minLon = 180.0, maxLon = -180.0;

    for (auto* el = root->FirstChildElement("node"); el;
         el       = el->NextSiblingElement("node")) {
        int64_t id  = 0;
        double  lat = 0.0, lon = 0.0;
        el->QueryInt64Attribute ("id",  &id);
        el->QueryDoubleAttribute("lat", &lat);
        el->QueryDoubleAttribute("lon", &lon);
        nodes[id] = {lat, lon};
        minLat = std::min(minLat, lat);  maxLat = std::max(maxLat, lat);
        minLon = std::min(minLon, lon);  maxLon = std::max(maxLon, lon);
    }

    if (nodes.empty()) {
        std::cerr << "OsmFormat: no <node> elements found\n";
        return std::nullopt;
    }

    // ── 2. Parse all <way> elements ───────────────────────────────────────────
    std::vector<OsmWay> ways;
    for (auto* el = root->FirstChildElement("way"); el;
         el       = el->NextSiblingElement("way")) {
        OsmWay way;
        for (auto* nd = el->FirstChildElement("nd"); nd;
             nd       = nd->NextSiblingElement("nd")) {
            int64_t ref = 0;
            nd->QueryInt64Attribute("ref", &ref);
            way.refs.push_back(ref);
        }
        way.tags = collectTags(el);
        if (!way.refs.empty() && surfaceFromTags(way.tags) != SurfaceType::Unknown)
            ways.push_back(std::move(way));
    }

    // ── 3. Compute grid dimensions from the bounding box ─────────────────────
    static constexpr double kEarth  = 6'371'000.0;
    static constexpr double kDegRad = M_PI / 180.0;
    const double midLat   = (minLat + maxLat) / 2.0;
    const double latScale = kEarth * kDegRad;
    const double lonScale = kEarth * kDegRad * std::cos(midLat * kDegRad);
    const double widthM   = (maxLon - minLon) * lonScale;
    const double heightM  = (maxLat - minLat) * latScale;
    const size_t W = std::max<size_t>(1, static_cast<size_t>(std::ceil(widthM  / cellSize)));
    const size_t H = std::max<size_t>(1, static_cast<size_t>(std::ceil(heightM / cellSize)));

    // ── 4. Build MapWorld ─────────────────────────────────────────────────────
    // Origin at the NW corner (maxLat, minLon) so that:
    //   x increases → east, z increases → south (both ≥ 0 for all nodes).
    MapWorld world(W, H, cellSize);
    world.setGeoOrigin(GeoOrigin{maxLat, minLon, cellSize});

    // ── 5. Rasterise each way ─────────────────────────────────────────────────
    GeoProjection proj(world.geoOrigin());
    for (const auto& way : ways) {
        const SurfaceType surf = surfaceFromTags(way.tags);
        if (surf == SurfaceType::Unknown) continue;

        // Project referenced node lat/lons to grid integer coordinates.
        std::vector<std::pair<int, int>> pts;
        pts.reserve(way.refs.size());
        for (int64_t ref : way.refs) {
            auto nit = nodes.find(ref);
            if (nit == nodes.end()) continue;
            auto [fx, fz] = proj.toLocal(nit->second.lat, nit->second.lon);
            pts.push_back({static_cast<int>(std::floor(fx)),
                           static_cast<int>(std::floor(fz))});
        }
        if (pts.size() < 2) continue;

        const bool closed = (way.refs.front() == way.refs.back()) && pts.size() >= 3;
        if (isArea(way.tags, closed)) {
            auto polyPts = pts;
            if (!polyPts.empty() && polyPts.front() == polyPts.back())
                polyPts.pop_back();
            rasterPoly(world.terrain(), polyPts, surf);
        } else {
            const int w = lineWidth(way.tags);
            for (size_t i = 1; i < pts.size(); ++i)
                rasterLine(world.terrain(),
                           pts[i - 1].first, pts[i - 1].second,
                           pts[i].first,     pts[i].second,
                           surf, w);
        }
    }

    return world;
}

} // anonymous namespace

// ── OsmFormat public API ──────────────────────────────────────────────────────

OsmFormat::OsmFormat(Options opts) : m_opts(opts) {}

std::optional<MapWorld> OsmFormat::readFromFile(const std::string& path) const
{
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(path.c_str()) != tinyxml2::XML_SUCCESS) {
        std::cerr << "OsmFormat: cannot load '" << path << "': "
                  << doc.ErrorStr() << '\n';
        return std::nullopt;
    }
    return buildFromDoc(doc, m_opts.cellSize);
}

std::optional<MapWorld> OsmFormat::readFromString(const std::string& xml) const
{
    tinyxml2::XMLDocument doc;
    if (doc.Parse(xml.c_str()) != tinyxml2::XML_SUCCESS) {
        std::cerr << "OsmFormat: XML parse error: " << doc.ErrorStr() << '\n';
        return std::nullopt;
    }
    return buildFromDoc(doc, m_opts.cellSize);
}

} // namespace grid::libmap
