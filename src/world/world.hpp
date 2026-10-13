#pragma once
#include "renderer/scene.hpp"
#include <array>
#include <map>
#include <optional>
#include "interiors/interior.hpp"
#include "mods/mods.hpp"
namespace er
{
    struct Landmark
    {
        const char *name;
        Vec2 pos;
    };
    inline constexpr std::array<Landmark, 5> landmarks{{{"Ashenmere Ruins", {0, 12}}, {"Frostwood", {-120, -70}}, {"Ember Hills", {130, -40}}, {"Reedlands", {-100, 110}}, {"Dragon Hollow", {30, -30}}}};
    struct Tree
    {
        Vec2 pos;
        float height = 8, radius = 3;
        bool conifer = false;
        int species = 0;
    };
    class World
    {
    public:
        std::optional<Interior> interior;
        uint64_t seed;
        uint32_t terrainSeed;
        std::vector<Tree> trees;
        using ChunkKey = std::pair<int, int>;
        static constexpr int chunkSize = 64, chunkRadius = 3;
        GenerationConfig config;
        explicit World(uint64_t seed = 42, GenerationConfig config = {});
        static ChunkKey chunkAt(Vec2 p) { return {int(std::floor(p.x / chunkSize)), int(std::floor(p.y / chunkSize))}; }
        size_t loadedChunkCount() const { return chunks.size(); }
        struct Terrain
        {
            float height = 0;
            Vec2 slope{};
            float moisture = .5f, temperature = .5f, river = 1;
        };
        struct Climate
        {
            std::array<float, 8> weights{};
            int dominant = 0;
        };
        Climate climate(const Terrain &sample) const;
        static const char *biomeName(int id);
        Terrain terrain(Vec2 p) const;
        float elevation(Vec2 p) const { return interior ? 0 : terrain(p).height; }
        float visualHeight(Vec2 p) const { return elevation(p) * .12f; }
        float rawHeight(Vec2 p) const;
        int biome(Vec2 p) const;
        float terrainNoise(Vec2 p, uint32_t salt, float scale) const;
        bool road(Vec2 p) const;
        bool water(Vec2 p) const;
        float waterLevel(Vec2 p) const;
        float shore(float x) const { return 37 + 3 * std::sin(x * .09f) + 1.5f * std::sin(x * .23f); }
        bool blocked(Vec2 p) const;
        Vec2 move(Vec2 p, Vec2 delta) const;
        void tick(float dt);
        void focus(Vec2 pos);
        const char *region(Vec2 pos) const;
        void pushGrass(Vec2 pos, float radius, float strength);
        Vec2 grassBend(Vec2 pos) const;
        void render(Scene &scene, float time) const;
        void reflections(Scene &scene, float time) const;

    private:
        Vec2 grassCenter{};
        ChunkKey centerChunk{2147483647, 2147483647};
        std::map<ChunkKey, std::vector<Tree>> chunks;
        static constexpr int terrainStep = 4, terrainSide = chunkSize / terrainStep + 1;
        struct HeightSample
        {
            float height = 0, moisture = .5f, temperature = .5f, river = 1;
        };
        HeightSample rawSample(Vec2 p) const;
        using HeightGrid = std::array<HeightSample, terrainSide * terrainSide>;
        std::map<ChunkKey, HeightGrid> heights;
        void buildHeight(ChunkKey key);
        std::vector<HeightGrid> heightLookup = std::vector<HeightGrid>(49);
        mutable std::vector<Vec2> surface;
        bool treeAt(int x, int y, Tree &tree) const;
        void streamChunks(Vec2 pos);
        void rebuildShadows();
        struct Blade
        {
            Vec2 bend{}, velocity{};
        };
        std::vector<Blade> blades = std::vector<Blade>(181 * 141);
        std::vector<float> shadows = std::vector<float>(181 * 141, 1.f);
    };
    void drawCharacter(Scene &s, Vec2 p, Color color, float time, bool moving = false, int style = 1);
    void drawWolf(Scene &s, Vec2 p, float time);
    void drawSkeleton(Scene &s, Vec2 p, Color color, float time);
}
