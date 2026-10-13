#include "worldgen/environment.hpp"
#include "npc/population.hpp"
namespace er
{
    void Environment::tick(const World &world, Vec2 hero, const std::vector<Entity> &threats, float time, float dt)
    {
        auto tile = World::chunkAt(hero);
        if (tile != center)
        {
            center = tile;
            animals.clear();
            for (int y = tile.second - 1; y <= tile.second + 1; ++y)
                for (int x = tile.first - 1; x <= tile.first + 1; ++x)
                    for (int n = 0; n < 2; ++n)
                    {
                        uint32_t key = hash(uint32_t(x) * 773u ^ uint32_t(y) * 991u ^ world.terrainSeed ^ uint32_t(n + 913));
                        Vec2 pos{x * 64.f + 8 + float(key % 48), y * 64.f + 8 + float((key >> 8) % 48)};
                        if (world.water(pos) || world.blocked(pos))
                            continue;
                        animals.push_back({pos, pos, key, n == 1});
                    }
        }
        for (auto &animal : animals)
        {
            Vec2 target = animal.home + Vec2{std::sin(time * .25f + animal.key % 100) * 7, std::cos(time * .19f + animal.key % 100) * 5};
            float speed = animal.bird ? 5.f : 1.7f;
            if (!animal.bird)
            {
                Vec2 danger = hero;
                float nearest = distance(hero, animal.pos);
                for (const auto &e : threats)
                    if (e.alive && e.name == "Wolf" && distance(e.pos, animal.pos) < nearest)
                    {
                        danger = e.pos;
                        nearest = distance(e.pos, animal.pos);
                    }
                if (nearest < 12)
                {
                    target = animal.pos + normalized(animal.pos - danger) * 15;
                    speed = 6;
                }
            }
            Vec2 d = target - animal.pos;
            if (length(d) > .1f)
                animal.pos = world.move(animal.pos, normalized(d) * std::min(length(d), speed * dt));
        }
    }
    void Environment::render(Scene &scene, float time) const
    {
        for (const auto &a : animals)
        {
            if (a.bird)
            {
                scene.point(a.pos, 5 + std::sin(time + a.key % 10), {158, 182, 194}, int(time * 5) % 2 ? U'v' : U'^');
                continue;
            }
            scene.point(a.pos, 1.2f, {177, 150, 113}, U'd');
            scene.point(a.pos + Vec2{.8f, 0}, 2, {196, 176, 142}, U'Y');
            scene.point(a.pos + Vec2{-.7f, 0}, .4f, {157, 136, 107}, U'/');
        }
    }
    void Environment::atmosphere(Framebuffer &f, const Camera &camera, uint32_t seed, float time) const
    {
        float hour = worldHour(time);
        int weather = weatherAt(seed, time);
        float daylight = hour < 6 || hour >= 21 ? .66f : hour < 8 || hour >= 19 ? .83f
                                                                                : 1.f;
        for (int y = 0; y < f.height; ++y)
            for (int x = 0; x < f.width; ++x)
            {
                auto &cell = f.cells[size_t(y * f.width + x)];
                float visibility = weather == 3 ? clamp(1 - distance(camera.unproject({float(x), float(y)}), camera.center) / 130, .45f, 1.f) : 1;
                cell.fg = cell.fg * (daylight * visibility);
                if ((weather == 1 || weather == 2 || weather == 4) && hash(uint32_t(x) * 713u ^ uint32_t(y + int(time * 18)) * 917u ^ seed) % 347 == 0)
                {
                    cell.glyph = weather == 2 ? U'*' : U'\'';
                    cell.fg = weather == 2 ? Color{154, 171, 192} : Color{84, 121, 158};
                }
            }
    }
}
