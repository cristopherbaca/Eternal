#include "worldgen/atlas.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
namespace er
{
    Vec2 placeAnchor(int x, int y, uint32_t seed)
    {
        if (x == 0 && y == 0)
            return {24, 20};
        return {x * 256.f + 96 + noise(x, y, seed + 1701) * 64, y * 256.f + 96 + noise(x, y, seed + 1702) * 64};
    }
    bool placeClearing(Vec2 p, uint32_t seed)
    {
        auto id = Atlas::tile(p);
        return distance(p, placeAnchor(id.first, id.second, seed)) < 23;
    }
    static float segmentDistance(Vec2 p, Vec2 a, Vec2 b)
    {
        Vec2 d = b - a;
        return distance(p, a + d * clamp(dot(p - a, d) / std::max(1.f, dot(d, d)), 0.f, 1.f));
    }
    bool connectingRoad(Vec2 p, uint32_t seed)
    {
        auto id = Atlas::tile(p);
        Vec2 a = placeAnchor(id.first, id.second, seed);
        // Four straight approaches join at every settlement/landmark, including across tile seams.
        for (auto d : {PlaceId{1, 0}, {-1, 0}, {0, 1}, {0, -1}})
            if (segmentDistance(p, a, placeAnchor(id.first + d.first, id.second + d.second, seed)) < 2)
                return true;
        return false;
    }
    Atlas::Atlas(uint32_t s, const std::filesystem::path &data) : seed(s)
    {
        auto file = [&](const char *name)
        { auto p=data/name; return std::filesystem::exists(p)?p:std::filesystem::path(ER_DATA_DIR)/name; };
        std::ifstream in(file("places.txt"));
        std::string row;
        while (std::getline(in, row))
        {
            if (row.empty() || row[0] == '#')
                continue;
            std::istringstream line(row);
            PlaceType t;
            if (!(line >> std::quoted(t.name) >> t.glyph >> t.inhabited >> t.interior >> std::quoted(t.purpose)))
                throw std::runtime_error("Invalid places.txt row");
            types.push_back(t);
        }
        std::ifstream names(file("place_names.txt"));
        while (std::getline(names, row))
            if (!row.empty() && row[0] != '#')
                prefixes.push_back(row);
        if (types.size() < 3 || prefixes.empty())
            throw std::runtime_error("World place tables are empty");
        focus({0, 12});
    }
    Place Atlas::place(PlaceId id) const
    {
        uint32_t key = hash(uint32_t(id.first) * 73856093u ^ uint32_t(id.second) * 19349663u ^ seed ^ 3911u);
        int kind = int(key % types.size());
        if (id == PlaceId{0, 0})
            kind = 0;
        if (id == PlaceId{1, 1})
            kind = 2;
        return {id, placeAnchor(id.first, id.second, seed), prefixes[(key >> 8) % prefixes.size()] + " " + types[size_t(kind)].name, kind, key};
    }
    void Atlas::focus(Vec2 p)
    {
        auto id = tile(p);
        if (id == center)
            return;
        center = id;
        nearby.clear();
        for (int y = id.second - 1; y <= id.second + 1; ++y)
            for (int x = id.first - 1; x <= id.first + 1; ++x)
                nearby.push_back(place({x, y}));
    }
    std::vector<Place> Atlas::discover(Vec2 p)
    {
        focus(p);
        auto f = fogTile(p);
        for (int y = -1; y <= 1; ++y)
            for (int x = -1; x <= 1; ++x)
                explored.insert({f.first + x, f.second + y});
        std::vector<Place> found;
        for (const auto &site : nearby)
            if (distance(p, site.pos) < 60 && visited.insert(site.id).second)
            {
                known.insert(site.id);
                found.push_back(site);
            }
        return found;
    }
    void Atlas::render(Scene &s, float time) const
    {
        for (const auto &site : nearby)
        {
            Vec2 projected = s.camera.project(site.pos, 5);
            if (projected.x < -60 || projected.y < -60 || projected.x > s.camera.width + 60 || projected.y > s.camera.height + 60)
                continue;
            const auto &type = types[size_t(site.type)];
            if (!type.inhabited)
            {
                // Different landmarks share scene primitives, not the house silhouette.
                if (type.glyph == '+' || type.glyph == '*')
                {
                    for (float dx : {-3.f, 0.f, 3.f})
                        for (float z = 0; z < 5; z += .5f)
                            s.point(site.pos + Vec2{dx, 0}, z, {150, 179, 183}, z > 4 ? U'*' : U'|');
                }
                else if (type.glyph == '$')
                {
                    for (float dx = -2; dx <= 2; dx += .5f)
                        for (float z = 0; z < 2; z += .5f)
                            s.point(site.pos + Vec2{dx, 0}, z, {225, 181, 101}, U'=');
                }
                else if (type.glyph == 'W')
                {
                    for (float dx = -7; dx <= 7; dx += .5f)
                        s.point(site.pos + Vec2{dx, 0}, std::abs(dx) * .4f, {139, 112, 90}, U'\\');
                    for (float z = 0; z < 10; z += .5f)
                        s.point(site.pos, z, {170, 158, 125}, U'|');
                }
                else if (type.glyph == 'A')
                {
                    for (float dx = -3; dx <= 3; dx += .5f)
                        for (float z = 0; z < 13; z += .5f)
                            if (std::abs(dx) > 1 || int(z) % 4 != 2)
                                s.point(site.pos + Vec2{dx, 0}, z, {145, 151, 168}, U'#');
                }
                else
                {
                    int height = type.glyph == 'B' ? 8 : 5;
                    for (float dx = -5; dx <= 5; dx += .5f)
                        for (float z = 0; z < height; z += .5f)
                        {
                            if (std::abs(dx) < 2 && z < height - 2)
                                continue;
                            if (type.glyph == 'R' && int(dx + z + 20) % 4 == 0)
                                continue;
                            s.point(site.pos + Vec2{dx, 0}, z, type.glyph == '!' ? Color{188, 137, 100} : Color{122, 138, 154}, type.glyph == '!' ? U'/' : U'#');
                        }
                }
                s.point(site.pos, 1, {239, 196, 123}, U'>', {}, 1);
                s.point(site.pos, 9, {231, 177, 118}, type.glyph, {}, 1);
                s.lights.push_back({site.pos + Vec2{2, 1}, 2, {231, 160, 93}, 10, .8f});
                continue;
            }
            int houses = type.inhabited ? (site.type == 2 ? 5 : 3) : 1;
            for (int house = 0; house < houses; ++house)
            {
                const Vec2 offsets[] = {{0, 0}, {-9, -8}, {9, -8}, {-16, 4}, {16, 4}};
                Vec2 p = site.pos + offsets[house];
                if (!type.inhabited)
                    p = site.pos;
                for (float x = -3; x <= 3; x += .5f)
                    for (float z = 0; z < 6; z += .5f)
                    {
                        if (std::abs(x) < 1 && z < 3)
                            continue;
                        s.point(p + Vec2{x, 0}, z, type.inhabited ? Color{151, 121, 91} : Color{126, 139, 152}, z > 4 ? U'/' : U'#');
                    }
                s.point(p, 1, {242, 198, 112}, U'>', {}, 1);
                for (int i = 0; i < 5; ++i)
                    s.point(p + Vec2{float(std::sin(time + i) * .7), 0}, 7 + i * .7f, {132, 145, 155}, U'~', {}, .2f);
                s.point(p + Vec2{3, 0}, 6, {240, 157, 80}, type.glyph, {}, 1);
            }
            s.lights.push_back({site.pos + Vec2{2, 1}, 2, {244, 173, 88}, 12, 1});
        }
    }
    void Atlas::labels(Framebuffer &f, const Camera &c, Vec2 player) const
    {
        for (const auto &site : nearby)
            if (distance(site.pos, player) < 65)
            {
                f.drawDepth = c.deviceDepth(site.pos, 9);
                Vec2 p = c.project(site.pos, 9);
                f.text(int(p.x) - int(site.name.size() / 2), int(p.y), site.name, {226, 199, 131});
                if (distance(site.pos, player) < (types[size_t(site.type)].inhabited ? 22 : 7))
                    f.text(int(p.x) - 8, int(p.y) + 1, "F: enter / inspect", {174, 205, 219});
                f.drawDepth = 0;
            }
    }
    void Atlas::save(std::ostream &out) const
    {
        out << "ATLAS 2\n"
            << known.size() << '\n';
        for (auto id : known)
            out << id.first << ' ' << id.second << '\n';
        out << explored.size() << '\n';
        for (auto id : explored)
            out << id.first << ' ' << id.second << '\n';
        out << visited.size() << '\n';
        for (auto id : visited)
            out << id.first << ' ' << id.second << '\n';
    }
    bool Atlas::load(std::istream &in)
    {
        std::string magic;
        int version;
        if (!(in >> magic >> version) || magic != "ATLAS" || (version != 1 && version != 2))
            return false;
        auto read = [&](std::set<PlaceId> &values, int bound)
        { size_t count; if(!(in>>count)||count>1000000) return false; for(size_t i=0;i<count;++i) { int x,y; if(!(in>>x>>y)||std::abs(int64_t(x))>bound||std::abs(int64_t(y))>bound) return false; values.insert({x,y}); } return true; };
        if (!read(known, 4000) || !read(explored, 32000))
            return false;
        if (version == 1)
            visited = known;
        else if (!read(visited, 4000))
            return false;
        for (auto id : visited)
            if (!known.count(id))
                return false;
        return true;
    }
}
