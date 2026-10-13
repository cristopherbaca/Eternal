#include "assets/data.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <set>
namespace er
{
    Role parseRole(const std::string &name)
    {
        for (int i = 0; i < 5; ++i)
            if (name == roleName(Role(i)))
                return Role(i);
        throw std::runtime_error("Unknown class: " + name);
    }
    Data::Data(const std::filesystem::path &requested, const std::vector<std::filesystem::path> &overlays)
    {
        auto root = requested.empty() ? (std::filesystem::exists("data/classes.txt") ? std::filesystem::path("data") : std::filesystem::path(ER_DATA_DIR)) : requested;
        auto read = [&](const char *file, auto consume)
        {
            auto path = root / file;
            for (const auto &layer : overlays)
                if (std::filesystem::exists(layer / file))
                    path = layer / file;
            std::ifstream f(path);
            if (!f)
                throw std::runtime_error("Cannot read game data: " + (root / file).string());
            std::string line;
            int row = 0;
            while (std::getline(f, line))
            {
                ++row;
                auto first = line.find_first_not_of(" \t\r");
                if (first == std::string::npos || line[first] == '#')
                    continue;
                std::istringstream in(line);
                consume(in);
                if (in.fail())
                    throw std::runtime_error(std::string(file) + ":" + std::to_string(row) + ": malformed row");
            }
        };
        read("classes.txt", [&](std::istream &in)
             { std::string role; in>>role; auto& c=classes[size_t(parseRole(role))]; int r=0,g=0,b=0; in>>c.hp>>c.mp>>c.armor>>c.power>>c.speed>>r>>g>>b; c.color=rgb(float(r),float(g),float(b)); if(c.hp<=0||c.mp<0||c.speed<=0) throw std::runtime_error("Invalid class stats"); });
        read("abilities.txt", [&](std::istream &in)
             { std::string role; int slot=0; in>>role>>slot; if(slot<1||slot>6) throw std::runtime_error("Ability slot outside 1..6"); auto& a=abilities[size_t(parseRole(role))][size_t(slot-1)]; in>>std::quoted(a.name)>>a.effect>>a.cost>>a.cooldown>>a.power>>a.radius>>a.speed>>a.range>>a.duration; if(a.cost<0||a.cooldown<0||a.range<0) throw std::runtime_error("Invalid ability stats"); });
        for (int i = 0; i < 5; ++i)
        {
            if (classes[size_t(i)].hp <= 0)
                throw std::runtime_error("Missing class");
            for (const auto &a : abilities[size_t(i)])
                if (a.name.empty())
                    throw std::runtime_error("Missing ability");
        }
        read("enemies.txt", [&](std::istream &in)
             { EnemyDef e; int r=0,g=0,b=0; in>>std::quoted(e.name)>>e.hp>>e.armor>>e.power>>e.speed>>e.aggro>>e.range>>e.xp>>e.gold>>e.item>>r>>g>>b; e.color=rgb(float(r),float(g),float(b)); if(e.hp<=0||e.speed<0||e.range<=0) throw std::runtime_error("Invalid enemy"); enemies.push_back(e); });
        read("items.txt", [&](std::istream &in)
             { ItemDef item; in>>item.id>>std::quoted(item.name)>>item.slot>>item.armor>>item.power>>item.value; if(item.id!=int(items.size())||item.slot<0||item.slot>2) throw std::runtime_error("Invalid item id/slot"); items.push_back(item); });
        if (enemies.size() < 3 || items.size() < 5)
            throw std::runtime_error("Content requires at least three enemy archetypes and five base items");
        for (const auto &e : enemies)
            if (e.item < 0 || e.item >= int(items.size()))
                throw std::runtime_error("Unknown loot item");
        read("boss.txt", [&](std::istream &in)
             { in>>boss.hp>>boss.armor>>boss.power>>boss.speed>>boss.enrage>>boss.gold; if(boss.hp<=0||boss.enrage<=0) throw std::runtime_error("Invalid boss"); });
        read("boss_phases.txt", [&](std::istream &in)
             { BossPhase p; in>>p.phase>>p.threshold>>p.ringInterval>>p.rings>>p.damage>>p.breathInterval>>p.adds; if(p.phase!=int(phases.size())+1||p.rings<1||p.ringInterval<=0) throw std::runtime_error("Invalid boss phase"); phases.push_back(p); });
        if (phases.size() != 3)
            throw std::runtime_error("Expected three boss phases");
        auto range = [](float value, float low, float high)
        { return std::isfinite(value) && value >= low && value <= high; };
        auto nameValid = [](const std::string &name)
        { return !name.empty() && name.size() <= 60 && std::all_of(name.begin(), name.end(), [](unsigned char c)
                                                                   { return c >= 32 && c < 127; }); };
        const std::set<std::string> effects = {"melee", "taunt", "bash", "charge", "shield", "group_buff", "fire_orb", "fire", "frost", "dash", "bolt", "mana", "arrow", "multishot", "trap", "pierce", "volley", "heal", "heal_area", "holy", "resurrect", "heal_group", "backstab", "poison", "knives"};
        for (const auto &c : classes)
            if (!range(c.hp, 1, 1000000) || !range(c.mp, 0, 1000000) || !range(c.power, 0, 10000) || !range(c.armor, 0, 10000) || !range(c.speed, .1f, 40))
                throw std::runtime_error("Class stats outside supported bounds");
        for (const auto &slots : abilities)
            for (const auto &a : slots)
            {
                if (!nameValid(a.name) || !effects.count(a.effect) || !range(a.cost, 0, 1000000) || !range(a.cooldown, 0, 600) || !range(a.power, 0, 100000) || !range(a.radius, 0, 50) || !range(a.speed, 0, 200) || !range(a.range, 0, 100) || !range(a.duration, 0, 600))
                    throw std::runtime_error("Invalid mod ability: " + a.name);
            }
        if (enemies.size() > 128 || items.size() > 1024)
            throw std::runtime_error("Content pack exceeds supported table size");
        for (const auto &e : enemies)
            if (!nameValid(e.name) || !range(e.hp, 1, 1000000) || !range(e.armor, 0, 10000) || !range(e.power, 0, 10000) || !range(e.speed, 0, 40) || !range(e.aggro, 0, 80) || !range(e.range, .1f, 50) || e.xp < 0 || e.xp > 100000 || e.gold < 0 || e.gold > 100000)
                throw std::runtime_error("Enemy stats outside supported bounds: " + e.name);
        for (const auto &item : items)
            if (!nameValid(item.name) || !range(item.armor, 0, 10000) || !range(item.power, 0, 10000) || item.value < 0)
                throw std::runtime_error("Invalid mod item: " + item.name);
        if (!range(boss.hp, 1, 10000000) || !range(boss.armor, 0, 10000) || !range(boss.power, 0, 10000) || !range(boss.speed, 0, 40) || !range(boss.enrage, 1, 3600) || boss.gold < 0 || boss.gold > 1000000)
            throw std::runtime_error("Boss stats outside supported bounds");
        for (const auto &p : phases)
            if (!range(p.threshold, 0, 1) || !range(p.ringInterval, .2f, 600) || !range(p.damage, 0, 100000) || !range(p.breathInterval, .2f, 600) || p.rings > 16 || p.adds < 0 || p.adds > 16)
                throw std::runtime_error("Boss phase outside supported bounds");
    }
    void Data::applyClass(Entity &e, Role role) const
    {
        const auto &c = classes[size_t(role)];
        e.role = role;
        e.hp = e.maxHp = c.hp + 8 * std::max(0, e.level - 1);
        e.mp = e.maxMp = c.mp;
        e.armor = c.armor;
        e.power = c.power + 2 * std::max(0, e.level - 1);
        e.speed = c.speed;
        e.color = c.color;
        e.cooldown.fill(0);
    }
}
