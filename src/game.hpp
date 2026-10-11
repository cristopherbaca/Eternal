#pragma once
#include "world/world.hpp"
#include "platform/terminal.hpp"
#include "entities/entity.hpp"
#include "combat/combat.hpp"
#include <tuple>
namespace er {
enum class Screen { Title, Playing, Paused, Settings, Map, Create, Saves, SaveActions, DeleteSave };
struct BossFight { bool active=false,won=false; float elapsed=0,ring=3,breath=6,wing=11,adds=16; int phase=1; };
class Game {
public:
    std::filesystem::path sourceData,saveDirectory="saves";
    ModSet sourceMods;
    std::string worldName="Untitled world",draftWorld="New world",draftName="",draftSeed;
    Role draftRole=Role::Warrior;
    bool draftGod=false,editingField=false;
    int activeSave=-1,selectedSave=0,creationSlot=-1;
    float saveTimer=0;
    std::string menuNotice;
    struct SaveInfo { bool exists=false; std::string label="Empty slot"; };
    std::array<SaveInfo,8> saves{};
    void refreshSaves();
    void beginCreation(int slot=-1);
    bool createAdventure();
    bool saveAdventure();
    bool loadAdventure(int slot);
    bool deleteAdventure(int slot);
    int menuCount() const;
    std::string seedText() const;
    bool frontEnd() const { return screen==Screen::Title||screen==Screen::Create||screen==Screen::Saves||screen==Screen::SaveActions||screen==Screen::DeleteSave||(screen==Screen::Settings&&returnScreen==Screen::Title); }
    std::vector<std::string> activeMods;
    Data data; World world; Combat combat; Camera camera; Entity hero; std::vector<Entity> party,mobs; Entity dragon;
    std::vector<FloatingText> texts; Vec2 velocity{},aim{5,12}; float time=0; bool quit=false;
    bool classMenu=false,helpMenu=false; std::string message="Welcome to Ashenmere. C: class  H: help";
    bool inventoryMenu=false; int inventoryIndex=0; std::vector<Loot> loot;
    BossFight fight;
    Screen screen=Screen::Playing,returnScreen=Screen::Playing;
    int menuIndex=0,presentationFps=60,detail=1;
    bool companions=false,showHelp=true,showNames=true;
    std::array<bool,5> discovered{};
    void setTesting(bool enabled);
    void menuInput(const Input& input);
    void renderMenu(Framebuffer& frame);
    void renderUI(Framebuffer& frame,float ms);
    int uiWidth=0,uiHeight=0;
    void setCompanions(bool enabled);
    void awardXP(int amount);
    bool loadSettings(const std::filesystem::path& path=".eternalrpg-settings.txt");
    bool saveSettings(const std::filesystem::path& path=".eternalrpg-settings.txt") const;
    explicit Game(uint64_t seed=42,const std::filesystem::path& dataPath={},const ModSet& mods={});
    void setScene(const std::string& name);
    void input(const Input& in); void tick(float dt); void render(Framebuffer& f,float ms,Framebuffer* interface=nullptr);
    std::vector<Entity*> entities();
    void streamEnemies();
    void spawnMob(Vec2 pos,int archetype);
    void collectLoot(); void equipSelected(); void respawn(); void rewards();
    void resetBoss();
private:
    int nextMobId=1000;
    float spawnTimer=0;
    using SpawnKey=std::tuple<int,int,int>;
    std::map<SpawnKey,float> respawnAfter;
    World::ChunkKey enemyCenter{2147483647,2147483647};
    float movementTime=0; Scene scene; std::vector<int> casts;
};
}
