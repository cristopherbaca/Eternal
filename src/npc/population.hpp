#pragma once
#include "worldgen/atlas.hpp"
#include "world/world.hpp"
#include <tuple>
namespace er {
using NpcId=std::tuple<int,int,int>;
struct Person {
    NpcId id; PlaceId home; uint32_t key=0; int age=0,job=0,trait=0,gold=0;
    std::string name,culture; Vec2 pos;
    int friendIndex=0,rivalIndex=0,familyIndex=0;
};
struct Job { std::string name,faction; char glyph='n'; };
struct Trait { std::string name,greeting,likes,dislikes; };
struct PersonState { int gold=-1,affinity=0; bool recruited=false; };
class Population {
public:
    std::vector<Job> jobs; std::vector<Trait> traits;
    std::vector<Person> nearby;
    std::map<NpcId,PersonState> changes;
    explicit Population(const std::filesystem::path& data={});
    Person person(const Place& home,int index) const;
    Vec2 scheduled(const Person& person,const Atlas& atlas,float hour,bool shelter) const;
    void tick(const Atlas& atlas,const World& world,float time,float dt,bool shelter);
    void render(Scene& scene,float time) const;
    void labels(Framebuffer& frame,const Camera& camera,Vec2 hero,Vec2 aim) const;
    void save(std::ostream& out) const;
    bool load(std::istream& in);
private:
    struct Name { std::string culture,given,family; }; std::vector<Name> names;
    PlaceId center{2147483647,2147483647};
};
inline float worldHour(float time) { return std::fmod(8+time/45,24.f); }
inline int weatherAt(uint32_t seed,float time) { return int(hash(seed+uint32_t(time/180))%5); }
inline const char* weatherName(int weather) { const char* names[]={"Clear","Rain","Snow","Fog","Storm"}; return names[clamp(weather,0,4)]; }
}
