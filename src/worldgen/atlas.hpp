#pragma once
#include "renderer/scene.hpp"
#include <filesystem>
#include <set>
#include <map>
#include <istream>
namespace er {
using PlaceId=std::pair<int,int>;
struct PlaceType { std::string name,purpose; char glyph='?'; bool inhabited=false,interior=false; };
struct Place { PlaceId id; Vec2 pos; std::string name; int type=0; uint32_t key=0; };
// Stateless geometry is shared by the terrain, atlas and streamed population.
Vec2 placeAnchor(int x,int y,uint32_t seed);
bool placeClearing(Vec2 pos,uint32_t seed);
bool connectingRoad(Vec2 pos,uint32_t seed);
class Atlas {
public:
    uint32_t seed;
    std::vector<PlaceType> types;
    std::vector<std::string> prefixes;
    std::vector<Place> nearby;
    std::set<PlaceId> known,explored;
    explicit Atlas(uint32_t seed,const std::filesystem::path& data={});
    Place place(PlaceId id) const;
    static PlaceId tile(Vec2 p) { return {int(std::floor(p.x/256)),int(std::floor(p.y/256))}; }
    static PlaceId fogTile(Vec2 p) { return {int(std::floor(p.x/32)),int(std::floor(p.y/32))}; }
    void focus(Vec2 p);
    std::vector<Place> discover(Vec2 p);
    bool seen(Vec2 p) const { return explored.count(fogTile(p)); }
    void render(Scene& scene,float time) const;
    void labels(Framebuffer& frame,const Camera& camera,Vec2 player) const;
    void save(std::ostream& out) const;
    bool load(std::istream& in);
private: PlaceId center{2147483647,2147483647};
};
}
