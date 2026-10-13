#include "game.hpp"
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
namespace er
{
    namespace
    {
        std::filesystem::path slotPath(const std::filesystem::path &root, int slot) { return root / ("slot-" + std::to_string(slot + 1) + ".sav"); }
        void writeEntity(std::ostream &out, const Entity &e)
        {
            out << e.id << ' ' << std::quoted(e.name) << ' ' << int(e.kind) << ' ' << int(e.role) << ' '
                << e.pos.x << ' ' << e.pos.y << ' ' << e.home.x << ' ' << e.home.y << ' '
                << e.hp << ' ' << e.maxHp << ' ' << e.mp << ' ' << e.maxMp << ' ' << e.armor << ' ' << e.power << ' ' << e.speed << ' '
                << e.level << ' ' << e.xp << ' ' << e.gold << ' ' << e.testing << ' ' << e.alive << ' ' << e.rewarded << ' ' << e.summoned << ' '
                << e.archetype << ' ' << e.spawnChunkX << ' ' << e.spawnChunkY << ' ' << e.spawnSlot << ' ' << e.deadTime << ' '
                << int(e.color.r) << ' ' << int(e.color.g) << ' ' << int(e.color.b) << ' ' << e.inventory.size() << ' ';
            for (int id : e.inventory)
                out << id << ' ';
            for (int id : e.equipment)
                out << id << ' ';
            for (float cooldown : e.cooldown)
                out << cooldown << ' ';
            out << '\n';
        }
        bool readEntity(std::istream &in, Entity &e, const Data &data)
        {
            int kind, role, r, g, b, count;
            if (!(in >> e.id >> std::quoted(e.name) >> kind >> role >> e.pos.x >> e.pos.y >> e.home.x >> e.home.y >> e.hp >> e.maxHp >> e.mp >> e.maxMp >> e.armor >> e.power >> e.speed >> e.level >> e.xp >> e.gold >> e.testing >> e.alive >> e.rewarded >> e.summoned >> e.archetype >> e.spawnChunkX >> e.spawnChunkY >> e.spawnSlot >> e.deadTime >> r >> g >> b >> count))
                return false;
            if (kind < 0 || kind > 3 || role < 0 || role > 4 || count < 0 || count > 24 || e.name.size() > 80 || e.level < 1 || e.level > 100000 || e.xp < 0 || e.gold < 0 || e.archetype < 0 || e.archetype >= int(data.enemies.size()))
                return false;
            for (float v : {e.pos.x, e.pos.y, e.home.x, e.home.y, e.hp, e.maxHp, e.mp, e.maxMp, e.armor, e.power, e.speed, e.deadTime})
                if (!std::isfinite(v) || std::abs(v) > 10000000)
                    return false;
            if (std::abs(e.pos.x) > 999000 || std::abs(e.pos.y) > 999000 || e.maxHp <= 0 || e.maxMp < 0 || e.speed < 0)
                return false;
            e.kind = Kind(kind);
            e.role = Role(role);
            e.color = {uint8_t(r), uint8_t(g), uint8_t(b)};
            e.inventory.resize(size_t(count));
            for (int &id : e.inventory)
                if (!(in >> id) || id < 0 || id >= int(data.items.size()))
                    return false;
            for (size_t slot = 0; slot < e.equipment.size(); ++slot)
            {
                int &id = e.equipment[slot];
                if (!(in >> id) || id < -1 || id >= int(data.items.size()))
                    return false;
                if (id >= 0 && (data.items[size_t(id)].slot != int(slot) || std::find(e.inventory.begin(), e.inventory.end(), id) == e.inventory.end()))
                    return false;
            }
            for (float &cooldown : e.cooldown)
                if (!(in >> cooldown) || !std::isfinite(cooldown) || cooldown < 0)
                    return false;
            if (e.kind == Kind::Mob && e.level > 80)
            {
                float healthScale = (1 + .2f * 79) / (1 + .2f * (e.level - 1));
                e.hp *= healthScale;
                e.maxHp *= healthScale;
                e.power *= (1 + .15f * 79) / (1 + .15f * (e.level - 1));
                e.level = 80;
            }
            return true;
        }
    }
    std::string Game::seedText() const
    {
        std::ostringstream out;
        out << std::setw(12) << std::setfill('0') << world.seed;
        return out.str();
    }
    void Game::refreshSaves()
    {
        for (int i = 0; i < 8; ++i)
        {
            std::error_code error;
            saves[size_t(i)] = {};
            auto path = slotPath(saveDirectory, i);
            if (!std::filesystem::exists(path, error))
                continue;
            saves[size_t(i)].exists = true;
            std::ifstream in(path);
            std::string magic, name, player;
            int version;
            uint64_t seed;
            if (in >> magic >> version >> std::quoted(name) >> seed >> std::quoted(player); in && magic == "ETERNAL" && (version >= 1 && version <= 7))
                saves[size_t(i)].label = name + " / " + player;
            else
                saves[size_t(i)].label = "Unreadable save (can be deleted)";
        }
    }
    void Game::beginCreation(int slot)
    {
        creationSlot = slot;
        refreshSaves();
        draftName = "";
        draftWorld = "New world";
        draftRole = Role::Warrior;
        draftGod = false;
        std::random_device random;
        uint64_t seed = (uint64_t(random()) << 32) | random();
        std::ostringstream out;
        out << std::setw(12) << std::setfill('0') << seed % 1000000000000ULL;
        draftSeed = out.str();
        screen = Screen::Create;
        menuIndex = 0;
        editingField = false;
        menuNotice.clear();
    }
    bool Game::createAdventure()
    {
        if (draftSeed.size() != 12 || draftSeed.find_first_not_of("0123456789") != std::string::npos)
        {
            menuNotice = "Seed must contain exactly 12 digits.";
            return false;
        }
        if (draftName.empty() || draftName.find_first_not_of(' ') == std::string::npos || draftWorld.find_first_not_of(' ') == std::string::npos)
        {
            menuNotice = "Enter a world name and player name.";
            return false;
        }
        refreshSaves();
        int slot = creationSlot;
        if (slot < 0)
        {
            slot = 0;
            while (slot < 8 && saves[size_t(slot)].exists)
                ++slot;
        }
        else if (slot >= 8 || saves[size_t(slot)].exists)
        {
            menuNotice = "That slot is no longer empty.";
            return false;
        }
        if (slot == 8)
        {
            menuNotice = "All 8 slots are full. Delete a save first.";
            return false;
        }
        Game fresh(std::stoull(draftSeed), sourceData, sourceMods);
        fresh.saveDirectory = saveDirectory;
        fresh.activeSave = slot;
        fresh.worldName = draftWorld;
        fresh.hero.name = draftName;
        fresh.applyPlayerClass(draftRole);
        fresh.hero.gold = 0;
        fresh.hero.inventory.clear();
        fresh.hero.equipment.fill(-1);
        fresh.setTesting(draftGod);
        fresh.discovered.fill(false);
        fresh.discovered[0] = true;
        fresh.hero.pos = {0, 12};
        fresh.camera.center = fresh.hero.pos;
        fresh.detail = detail;
        fresh.presentationFps = presentationFps;
        fresh.camera.zoom = camera.zoom;
        fresh.showHelp = showHelp;
        fresh.showNames = showNames;
        if (!fresh.saveAdventure())
        {
            menuNotice = fresh.menuNotice;
            return false;
        }
        *this = std::move(fresh);
        screen = Screen::Playing;
        message = "Your story begins. I: inventory | E: gather loot | Esc: save and pause";
        return true;
    }
    bool Game::saveAdventure()
    {
        if (activeSave < 0)
            return true;
        try
        {
            std::filesystem::create_directories(saveDirectory);
            auto path = slotPath(saveDirectory, activeSave), temp = path;
            temp += ".tmp";
            std::ofstream out(temp, std::ios::trunc);
            out << std::setprecision(9);
            out << "ETERNAL 7 " << std::quoted(worldName) << ' ' << world.seed << ' ' << std::quoted(hero.name) << '\n';
            out << activeMods.size() << ' ';
            for (const auto &mod : activeMods)
                out << std::quoted(mod) << ' ';
            out << '\n';
            out << data.items.size() << ' ';
            for (const auto &item : data.items)
                out << std::quoted(item.name) << ' ';
            out << '\n';
            const auto &gen = world.config;
            out << gen.treeDensity << ' ' << gen.lakeThreshold << ' ' << gen.biomeScale << ' ' << gen.respawnSeconds << ' ' << gen.levelDistance << ' ' << gen.enemiesPerChunk << ' ' << gen.seedSalt << '\n';
            out << time << ' ' << companions << ' ' << nextMobId << '\n';
            writeEntity(out, hero);
            writeEntity(out, dragon);
            out << party.size() << '\n';
            for (const auto &e : party)
                writeEntity(out, e);
            out << mobs.size() << '\n';
            for (const auto &e : mobs)
                writeEntity(out, e);
            for (bool found : discovered)
                out << found << ' ';
            out << '\n';
            out << fight.active << ' ' << fight.won << ' ' << fight.elapsed << ' ' << fight.ring << ' ' << fight.breath << ' ' << fight.wing << ' ' << fight.adds << ' ' << fight.phase << '\n';
            out << loot.size() << '\n';
            for (const auto &item : loot)
                out << item.pos.x << ' ' << item.pos.y << ' ' << item.item << ' ' << item.gold << '\n';
            out << respawnAfter.size() << '\n';
            for (const auto &entry : respawnAfter)
                out << std::get<0>(entry.first) << ' ' << std::get<1>(entry.first) << ' ' << std::get<2>(entry.first) << ' ' << entry.second << '\n';
            atlas.save(out);
            out << "INTERIORS 1 " << bool(interiorSite) << '\n';
            if (interiorSite)
            {
                out << interiorSite->first << ' ' << interiorSite->second << ' ' << outsidePosition.x << ' ' << outsidePosition.y << ' ' << world.interior->opened << ' ' << world.interior->trapSpent << '\n';
                out << outsideMobs.size() << '\n';
                for (const auto &e : outsideMobs)
                    writeEntity(out, e);
                out << outsideLoot.size() << '\n';
                for (const auto &item : outsideLoot)
                    out << item.pos.x << ' ' << item.pos.y << ' ' << item.item << ' ' << item.gold << '\n';
            }
            out << clearedPlaces.size() << '\n';
            for (auto id : clearedPlaces)
                out << id.first << ' ' << id.second << '\n';
            saveLiving(out);
            out << "COMBAT_BRAINS 1 " << combatBrains.size() << '\n';
            for (const auto &brain : combatBrains)
                brain.save(out);
            out.close();
            if (!out)
                throw std::runtime_error("Cannot write save file");
            std::filesystem::rename(temp, path);
            saveTimer = 0;
            menuNotice = "Adventure saved.";
            return true;
        }
        catch (const std::exception &e)
        {
            menuNotice = std::string("Save failed: ") + e.what();
            return false;
        }
    }
    bool Game::loadAdventure(int slot)
    {
        if (slot < 0 || slot >= 8)
            return false;
        try
        {
            std::ifstream in(slotPath(saveDirectory, slot));
            std::string magic, title, name;
            int version;
            uint64_t seed;
            auto require = [&](bool condition)
            { if(!condition) throw std::runtime_error("Invalid or incompatible save"); };
            require(bool(in >> magic >> version >> std::quoted(title) >> seed >> std::quoted(name)) && magic == "ETERNAL" && (version >= 1 && version <= 7) && seed < 1000000000000ULL && title.size() <= 24);
            size_t count;
            require(bool(in >> count) && count == activeMods.size());
            for (const auto &mod : activeMods)
            {
                std::string stored;
                require(bool(in >> std::quoted(stored)) && stored == mod);
            }
            require(bool(in >> count) && count == data.items.size());
            for (const auto &item : data.items)
            {
                std::string stored;
                require(bool(in >> std::quoted(stored)) && stored == item.name);
            }
            ModSet savedMods = sourceMods;
            auto &gen = savedMods.generation;
            require(bool(in >> gen.treeDensity >> gen.lakeThreshold >> gen.biomeScale >> gen.respawnSeconds >> gen.levelDistance >> gen.enemiesPerChunk >> gen.seedSalt));
            for (float value : {gen.treeDensity, gen.lakeThreshold, gen.biomeScale, gen.respawnSeconds, gen.levelDistance})
                require(std::isfinite(value));
            require(gen.treeDensity >= 0 && gen.treeDensity <= 1 && gen.biomeScale >= 32 && gen.respawnSeconds >= 1 && gen.levelDistance >= 1 && gen.enemiesPerChunk >= 0 && gen.enemiesPerChunk <= 16);
            Game fresh(seed, sourceData, savedMods);
            fresh.worldName = title;
            fresh.activeSave = slot;
            fresh.saveDirectory = saveDirectory;
            require(bool(in >> fresh.time >> fresh.companions >> fresh.nextMobId) && std::isfinite(fresh.time) && fresh.time >= 0 && fresh.nextMobId >= 1000);
            require(readEntity(in, fresh.hero, data) && fresh.hero.kind == Kind::Player);
            require(readEntity(in, fresh.dragon, data) && fresh.dragon.kind == Kind::Boss);
            auto entities = [&](std::vector<Entity> &es, size_t max)
            { require(bool(in>>count)&&count<=max); es.resize(count); for(auto& e:es) require(readEntity(in,e,data)); };
            entities(fresh.party, 3);
            entities(fresh.mobs, 100);
            for (auto &found : fresh.discovered)
                require(bool(in >> found));
            auto &f = fresh.fight;
            require(bool(in >> f.active >> f.won >> f.elapsed >> f.ring >> f.breath >> f.wing >> f.adds >> f.phase) && f.phase >= 1 && f.phase <= 3);
            for (float value : {f.elapsed, f.ring, f.breath, f.wing, f.adds})
                require(std::isfinite(value));
            require(bool(in >> count) && count <= 200);
            fresh.loot.resize(count);
            for (auto &item : fresh.loot)
                require(bool(in >> item.pos.x >> item.pos.y >> item.item >> item.gold) && std::isfinite(item.pos.x) && std::isfinite(item.pos.y) && item.item >= 0 && item.item < int(data.items.size()) && item.gold >= 0);
            require(bool(in >> count) && count <= 100000);
            for (size_t i = 0; i < count; ++i)
            {
                int x, y, id;
                float when;
                require(bool(in >> x >> y >> id >> when) && std::isfinite(when));
                fresh.respawnAfter[{x, y, id}] = when;
            }
            if (version >= 2)
                require(fresh.atlas.load(in));
            if (version >= 3)
            {
                std::string tag;
                int format;
                bool inside;
                require(bool(in >> tag >> format >> inside) && tag == "INTERIORS" && format == 1);
                if (inside)
                {
                    int x, y;
                    bool opened, trap;
                    require(bool(in >> x >> y >> fresh.outsidePosition.x >> fresh.outsidePosition.y >> opened >> trap) && std::abs(int64_t(x)) < 4000 && std::abs(int64_t(y)) < 4000);
                    require(std::isfinite(fresh.outsidePosition.x) && std::isfinite(fresh.outsidePosition.y) && std::abs(fresh.outsidePosition.x) < 999000 && std::abs(fresh.outsidePosition.y) < 999000);
                    fresh.interiorSite = PlaceId{x, y};
                    auto site = fresh.atlas.place(*fresh.interiorSite);
                    fresh.world.interior.emplace(site.key, fresh.atlas.types[size_t(site.type)].inhabited);
                    fresh.world.interior->opened = opened;
                    fresh.world.interior->trapSpent = trap;
                    require(!fresh.world.interior->blocked(fresh.hero.pos));
                    entities(fresh.outsideMobs, 100);
                    require(bool(in >> count) && count <= 200);
                    fresh.outsideLoot.resize(count);
                    for (auto &item : fresh.outsideLoot)
                        require(bool(in >> item.pos.x >> item.pos.y >> item.item >> item.gold) && std::isfinite(item.pos.x) && std::isfinite(item.pos.y) && item.item >= 0 && item.item < int(data.items.size()) && item.gold >= 0);
                }
                require(bool(in >> count) && count <= 1000000);
                for (size_t i = 0; i < count; ++i)
                {
                    int x, y;
                    require(bool(in >> x >> y) && std::abs(int64_t(x)) < 4000 && std::abs(int64_t(y)) < 4000);
                    fresh.clearedPlaces.insert({x, y});
                }
            }
            if (version >= 4)
                require(fresh.loadLiving(in, version));
            if (version >= 7)
            {
                std::string tag;
                int format;
                require(bool(in >> tag >> format >> count) && tag == "COMBAT_BRAINS" && format == 1 && (count == 0 || count == data.enemies.size()));
                fresh.combatBrains.resize(count);
                for (auto &brain : fresh.combatBrains)
                    require(brain.load(in));
            }
            fresh.atlas.discover(fresh.interiorSite ? fresh.outsidePosition : fresh.hero.pos);
            fresh.camera.center = fresh.hero.pos;
            fresh.camera.zoom = camera.zoom;
            fresh.world.focus(fresh.hero.pos);
            fresh.enemyCenter = World::chunkAt(fresh.hero.pos);
            fresh.spawnTimer = 1;
            fresh.aim = fresh.hero.pos + Vec2{5, 0};
            fresh.detail = detail;
            fresh.presentationFps = presentationFps;
            fresh.showHelp = showHelp;
            fresh.showNames = showNames;
            *this = std::move(fresh);
            message = "Adventure restored. I: inventory | Esc: pause";
            return true;
        }
        catch (const std::exception &e)
        {
            menuNotice = std::string("Load failed: ") + e.what();
            return false;
        }
    }
    bool Game::deleteAdventure(int slot)
    {
        if (slot < 0 || slot >= 8)
            return false;
        std::error_code error;
        bool removed = std::filesystem::remove(slotPath(saveDirectory, slot), error);
        if (error || !removed)
        {
            menuNotice = "Could not delete save.";
            return false;
        }
        if (activeSave == slot)
            activeSave = -1;
        refreshSaves();
        menuNotice = "Save deleted.";
        return true;
    }
    int Game::menuCount() const
    {
        switch (screen)
        {
        case Screen::Title:
            return 4;
        case Screen::Create:
            return 7;
        case Screen::Saves:
            return 9;
        case Screen::SaveActions:
            return 3;
        case Screen::DeleteSave:
            return 2;
        case Screen::Paused:
            return 6;
        default:
            return 9;
        }
    }
}
