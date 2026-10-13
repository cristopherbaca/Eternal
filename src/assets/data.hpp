#pragma once
#include "entities/entity.hpp"
#include <filesystem>
namespace er
{
    struct ClassDef
    {
        float hp = 0, mp = 0, armor = 0, power = 0, speed = 0;
        Color color{};
    };
    struct Ability
    {
        std::string name, effect;
        float cost = 0, cooldown = 0, power = 0, radius = 0, speed = 0, range = 0, duration = 0;
    };
    struct EnemyDef
    {
        std::string name;
        float hp = 0, armor = 0, power = 0, speed = 0, aggro = 0, range = 0;
        int xp = 0, gold = 0, item = 0;
        Color color{};
    };
    struct ItemDef
    {
        int id = 0;
        std::string name;
        int slot = 0;
        float armor = 0, power = 0;
        int value = 0;
    };
    struct BossDef
    {
        float hp = 7200, armor = 20, power = 38, speed = 2.4f, enrage = 150;
        int gold = 1200;
    };
    struct BossPhase
    {
        int phase = 1;
        float threshold = 1, ringInterval = 5.2f;
        int rings = 1;
        float damage = 45, breathInterval = 9;
        int adds = 0;
    };
    class Data
    {
    public:
        std::array<ClassDef, 5> classes{};
        std::array<std::array<Ability, 6>, 5> abilities{};
        std::vector<EnemyDef> enemies;
        std::vector<ItemDef> items;
        BossDef boss;
        std::vector<BossPhase> phases;
        explicit Data(const std::filesystem::path &path = {}, const std::vector<std::filesystem::path> &overlays = {});
        void applyClass(Entity &e, Role role) const;
    };
    Role parseRole(const std::string &name);
}
