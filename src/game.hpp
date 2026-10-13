#pragma once
#include "world/world.hpp"
#include "platform/terminal.hpp"
#include "entities/entity.hpp"
#include "combat/combat.hpp"
#include <tuple>
#include <optional>
#include "worldgen/atlas.hpp"
#include "npc/population.hpp"
#include "history/chronicle.hpp"
#include "quests/quest.hpp"
#include "worldgen/environment.hpp"
#include <deque>
namespace er
{
    enum class Screen
    {
        Title,
        Playing,
        Paused,
        Settings,
        Map,
        Create,
        Saves,
        SaveActions,
        DeleteSave,
        Talk,
        Trade,
        Journal,
        Lore,
        Progress
    };
    struct BossFight
    {
        bool active = false, won = false;
        float elapsed = 0, ring = 3, breath = 6, wing = 11, adds = 16;
        int phase = 1;
    };
    class Game
    {
    public:
        std::filesystem::path sourceData, saveDirectory = "saves";
        ModSet sourceMods;
        std::vector<CombatNetwork> combatBrains;
        std::string worldName = "Untitled world", draftWorld = "New world", draftName = "", draftSeed;
        Role draftRole = Role::Warrior;
        bool draftGod = false, editingField = false;
        int activeSave = -1, selectedSave = 0, creationSlot = -1;
        float saveTimer = 0;
        std::string menuNotice;
        struct SaveInfo
        {
            bool exists = false;
            std::string label = "Empty slot";
        };
        std::array<SaveInfo, 8> saves{};
        void refreshSaves();
        void beginCreation(int slot = -1);
        bool createAdventure();
        bool saveAdventure();
        bool loadAdventure(int slot);
        bool deleteAdventure(int slot);
        int menuCount() const;
        std::string seedText() const;
        bool frontEnd() const { return screen == Screen::Title || screen == Screen::Create || screen == Screen::Saves || screen == Screen::SaveActions || screen == Screen::DeleteSave || (screen == Screen::Settings && returnScreen == Screen::Title); }
        std::vector<std::string> activeMods;
        std::optional<PlaceId> interiorSite;
        std::set<PlaceId> clearedPlaces;
        Vec2 outsidePosition{};
        std::vector<Entity> outsideMobs;
        std::vector<Loot> outsideLoot;
        float transition = 0;
        void interactPlace();
        void leaveInterior();
        void tickInterior(float dt);
        std::array<int, 5> mastery{};
        std::array<int, 15> talents{};
        std::array<int, 3> materials{};
        std::array<int, 4> craftPractice{};
        std::map<PlaceId, int> harvested, bounties;
        float societyTimer = 0;
        void applyPlayerClass(Role role);
        int perkPoints() const;
        int carryWeight() const;
        bool canCarry(int item) const;
        void gather();
        void craft(int index);
        void progressInput(const Input &input);
        void renderProgress(Framebuffer &frame) const;
        void stealFrom(const Person &p);
        void settleBounty(const Person &p);
        void tickSociety(float dt);
        Environment environment;
        std::deque<std::string> eventLog;
        std::string lastEvent;
        Atlas atlas;
        Population population;
        Chronicle chronicle;
        std::vector<Quest> quests;
        std::map<std::string, int> reputation;
        NpcId talking{};
        std::string dialogue;
        int loreIndex = 0;
        void saveLiving(std::ostream &out) const;
        bool loadLiving(std::istream &in, int saveVersion);
        void talkNearby();
        Person speaker() const;
        void learnLore(int index);
        void offerQuest(const Person &person);
        bool escortActive() const;
        bool questReady(const Quest &q) const;
        void tickQuests(float dt);
        void deliverQuests(NpcId recipient);
        void renderEscorts(Scene &scene) const;
        void socialInput(const Input &input);
        void renderSocial(Framebuffer &frame) const;
        int buyPrice(const Person &person, const ItemDef &item) const;
        void tradeWith(const Person &person, int action);
        bool socialScreen() const { return screen == Screen::Talk || screen == Screen::Trade || screen == Screen::Journal || screen == Screen::Lore; }
        Vec2 mapCenter{};
        float mapStep = 12;
        std::optional<PlaceId> mapSelected;
        void minimap(Framebuffer &frame) const;
        void drawMap(Framebuffer &frame, int x, int y, int width, int height, Vec2 center, float step, bool full) const;
        Data data;
        World world;
        Combat combat;
        Camera camera;
        Entity hero;
        std::vector<Entity> party, mobs;
        Entity dragon;
        std::vector<FloatingText> texts;
        Vec2 velocity{}, aim{5, 12};
        float time = 0;
        bool quit = false;
        float jumpHeight = 0, jumpSpeed = 0, stepTimer = 0;
        std::vector<int> soundEvents;
        bool classMenu = false, helpMenu = false;
        std::string message = "Welcome to Ashenmere. C: class  H: help";
        bool inventoryMenu = false;
        int inventoryIndex = 0;
        std::vector<Loot> loot;
        BossFight fight;
        Screen screen = Screen::Playing, returnScreen = Screen::Playing;
        int menuIndex = 0, presentationFps = 60, detail = 1;
        bool companions = false, showHelp = true, showNames = true;
        std::array<bool, 5> discovered{};
        void setTesting(bool enabled);
        void menuInput(const Input &input);
        void renderMenu(Framebuffer &frame);
        void renderUI(Framebuffer &frame, float ms);
        int uiWidth = 0, uiHeight = 0;
        void setCompanions(bool enabled);
        void awardXP(int amount);
        bool loadSettings(const std::filesystem::path &path = ".eternalrpg-settings.txt");
        bool saveSettings(const std::filesystem::path &path = ".eternalrpg-settings.txt") const;
        explicit Game(uint64_t seed = 42, const std::filesystem::path &dataPath = {}, const ModSet &mods = {});
        void setScene(const std::string &name);
        void input(const Input &in);
        void tick(float dt);
        void render(Framebuffer &f, float ms, Framebuffer *interface = nullptr);
        std::vector<Entity *> entities();
        void streamEnemies();
        void spawnMob(Vec2 pos, int archetype);
        void collectLoot();
        void equipSelected();
        void respawn();
        void rewards();
        void resetBoss();

    private:
        int nextMobId = 1000;
        float spawnTimer = 0;
        using SpawnKey = std::tuple<int, int, int>;
        std::map<SpawnKey, float> respawnAfter;
        World::ChunkKey enemyCenter{2147483647, 2147483647};
        float movementTime = 0;
        Scene scene;
        std::vector<int> casts;
    };
}
