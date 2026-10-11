#include "ascii_conversion/ascii.hpp"
#include "platform/terminal.hpp"
#include "world/world.hpp"
#include "combat/combat.hpp"
#include "game.hpp"
#include "ai/ai.hpp"
#include "ui/hud.hpp"
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <thread>
#include <set>
#include <sstream>
int checks=0;
void check(bool ok,const char* name) { ++checks; if(!ok) throw std::runtime_error(name); }
int main() { try {
    using namespace er;
    check(std::abs(length(normalized({3,4}))-1)<.0001f,"vector normalization");
    check(length(normalized({0,0}))==0,"zero normalization");
    Camera c; c.zoom=1.4f; Vec2 p{12,-6}; check(distance(p,c.unproject(c.project(p)))<.0001f,"projection inverse");
    check(noise(-4,13,42)==noise(-4,13,42),"seed deterministic");
    check(brightnessGlyph(-1)==U' '&&brightnessGlyph(2)==U'@',"ramp clamps");
    check(edgeGlyph(10,0)==U'|'&&edgeGlyph(0,10)==U'-',"edge direction");
    Pixel px; px.color={90,90,90}; auto dark=shade(px,{},1),lit=shade(px,{{{0,0},0,{255,120,20},10,1}},1);
    check(lit.r>dark.r&&int(lit.r)-dark.r>int(lit.b)-dark.b,"warm radial light");
    check(luminance(shade(px,{},.5f))<luminance(dark),"vignette");
    Framebuffer f(20,10); check(!f.diff().empty()&&f.diff().empty(),"diff only changes"); f.put(1,1,U'^'); check(!f.diff().empty(),"changed cell emitted");
    Framebuffer stable(2,1); stable.put(0,0,U'x',{100,100,100}); stable.diff();
    stable.put(0,0,U'x',{101,101,101}); stable.put(1,0,U' ',{1,2,3});
    check(stable.diff().empty(),"invisible and sub-palette color changes do not repaint");
    stable.put(0,0,U'x',{150,100,100}); check(!stable.diff().empty(),"visible lighting changes repaint");
    InputParser parser; check(parser.feed("\x1b[<0;12;").keys.empty(),"split mouse input"); auto in=parser.feed("8Mw"); check(in.click&&in.mouseX==11&&in.mouseY==7&&in.keys=="w","mouse continuation");
    check(!parser.feed("\x1b[10;").cursorReport,"partial terminal acknowledgement waits");
    auto reply=parser.feed("20Rw"); check(reply.cursorReport&&reply.keys=="w","terminal acknowledgement preserves player input");
    World a(42),b(42); check(a.trees.size()==b.trees.size(),"world repeatability"); check(!a.blocked({0,-600}),"old map boundary removed");
    a.pushGrass({0,12},3,10); a.tick(1.f/60); check(length(a.grassBend({1,12}))>0,"grass deflects");
    for(int i=0;i<600;++i) a.tick(1.f/60); check(length(a.grassBend({1,12}))<.001f,"grass spring recovers");
    check(std::abs(damageAfterArmor(100,100)-50)<.001f,"armor damage formula");
    Data data; Entity mage,mob; data.applyClass(mage,Role::Mage); mob.id=100; mob.kind=Kind::Mob; mob.pos={5,12}; mob.hp=mob.maxHp=100; mob.armor=0;
    Combat combat; std::vector<Entity*> es{&mage,&mob}; std::vector<FloatingText> texts;
    float mp=mage.mp; check(combat.cast(mage,0,mob.pos,mob.id,es,data,a,texts).empty(),"fire orb cast");
    check(mage.mp<mp&&!combat.cast(mage,0,mob.pos,mob.id,es,data,a,texts).empty(),"cost and cooldown");
    for(int i=0;i<40;++i) combat.tick(1.f/60,es,a,texts); check(mob.hp<100,"projectile swept hit");
    mob.hp=90; heal(mob,200,texts); check(mob.hp==100,"heal clamps maximum"); dealDamage(mob,1000,texts); check(!mob.alive&&mob.hp==0,"death clamps zero");
    for(int r=0;r<5;++r) for(int slot=0;slot<6;++slot) {
        Entity e,ally; data.applyClass(e,Role(r)); e.mp=1000; ally.id=2; ally.kind=Kind::Party; ally.alive=!(r==3&&slot==4); ally.hp=50; ally.pos=e.pos;
        std::vector<Entity*> targets{&e,&ally,&mob}; check(combat.cast(e,slot,{5,12},ally.id,targets,data,a,texts).empty(),"all class abilities cast");
    }
    Game game;
    Input held; held.heldMovement=true; held.movementKeys="d"; game.input(held);
    check(game.velocity.x>0,"native held movement starts immediately");
    held.movementKeys.clear(); game.input(held); check(length(game.velocity)==0,"native key release stops immediately");
    auto& victim=game.mobs.front(); victim.hp=1; dealDamage(victim,10,game.texts); game.rewards(); check(!game.loot.empty()&&game.hero.xp>0,"kill yields loot and XP");
    int xp=game.hero.xp; game.rewards(); check(game.hero.xp==xp,"kill rewarded once");
    game.hero.pos=game.loot.front().pos; game.collectLoot(); check(!game.hero.inventory.empty(),"nearby loot collected");
    game.equipSelected(); float power=game.hero.power,armor=game.hero.armor; game.equipSelected(); game.equipSelected(); check(power==game.hero.power&&armor==game.hero.armor,"equip and unequip cycles cannot stack stats");
    dealDamage(game.hero,10000,game.texts); game.respawn(); check(game.hero.alive&&game.hero.hp==game.hero.maxHp,"death and respawn");
    game.setScene("boss"); tickDragon(game,1.f/60); check(game.fight.active,"boss activates in arena");
    game.dragon.hp=game.dragon.maxHp*.6f; tickDragon(game,1.f/60); check(game.fight.phase==2,"boss phase 2");
    game.dragon.hp=game.dragon.maxHp*.3f; game.fight.ring=0; tickDragon(game,1.f/60); check(game.fight.phase==3,"boss phase 3");
    for(int i=0;i<50;++i) tickDragon(game,1.f/60); check(!game.combat.areas.empty()&&game.combat.areas.back().delay>0,"boss telegraphs before damage");
    for(auto* e:game.entities()) if(e->kind==Kind::Player||e->kind==Kind::Party) e->alive=false;
    tickDragon(game,1.f/60); check(!game.fight.active&&game.dragon.hp==game.dragon.maxHp,"wipe resets encounter");
    game.respawn(); game.setScene("boss"); dealDamage(game.dragon,100000,game.texts); int gold=game.hero.gold; tickDragon(game,1.f/60); check(game.fight.won&&game.hero.gold>gold,"boss victory rewards");
    Entity target,boss; target.hp=target.maxHp=100; target.armor=0; boss.id=500; boss.kind=Kind::Boss;
    Combat hazard; hazard.areas.push_back({target.pos,5,3,1.5f,1,0,25,boss.id,"enemy_fire"});
    std::vector<Entity*> targets{&target,&boss};
    for(int i=0;i<89;++i) hazard.tick(1.f/60,targets,a,texts); check(target.hp==100,"telegraph deals no early damage");
    for(int i=0;i<3;++i) hazard.tick(1.f/60,targets,a,texts); check(target.hp<100,"telegraph detonates after delay");
    Game replayA(123),replayB(123); replayA.setScene("boss"); replayB.setScene("boss");
    for(int i=0;i<600;++i) { replayA.tick(1.f/60); replayB.tick(1.f/60); }
    Framebuffer replayFrameA(200,60),replayFrameB(200,60); replayA.render(replayFrameA,16); replayB.render(replayFrameB,16);
    check(replayFrameA.cells==replayFrameB.cells&&replayA.hero.hp==replayB.hero.hp&&replayA.dragon.hp==replayB.dragon.hp,"fixed-step replay is deterministic");
    Framebuffer tiny(1,1); replayA.render(tiny,16); check(tiny.cells.size()==1,"minimum terminal handled");
    Game solo;
    check(solo.party.empty()&&solo.hero.level==1,"new game starts solo at level one");
    solo.screen=Screen::Title; float stoppedTime=solo.time;
    solo.tick(1); check(solo.time==stoppedTime,"title freezes simulation");
    Input key; key.keys="\r"; solo.input(key); check(solo.screen==Screen::Create,"title opens new adventure form"); solo.screen=Screen::Playing;
    key.keys="p1"; solo.input(key); solo.tick(1);
    check(solo.screen==Screen::Paused&&solo.time==stoppedTime&&solo.combat.projectiles.empty(),"pause freezes combat and ignores casts");
    key.keys="s\r"; solo.input(key); check(solo.screen==Screen::Settings,"pause opens settings");
    key.keys="\r"; solo.input(key); check(solo.hero.testing,"settings enables testing powers");
    float health=solo.hero.hp; dealDamage(solo.hero,1000000,solo.texts);
    check(solo.hero.hp==health&&solo.hero.alive,"testing player is invulnerable");
    solo.hero.mp=0; auto soloEntities=solo.entities();
    check(solo.combat.cast(solo.hero,0,solo.hero.pos+Vec2{10,0},0,soloEntities,solo.data,solo.world,solo.texts).empty(),"testing casts without mana");
    check(solo.hero.cooldown[0]<=.12f&&solo.combat.projectiles.back().power>500,"testing boosts damage and shortens cooldowns");
    key.keys=char(27); solo.input(key); check(solo.screen==Screen::Paused,"settings returns to pause");
    key.keys="p"; solo.input(key); check(solo.screen==Screen::Playing,"resume returns to game");
    solo.setTesting(false); health=solo.hero.hp; dealDamage(solo.hero,10,solo.texts);
    check(solo.hero.hp<health,"normal damage restored after testing disabled");
    int oldLevel=solo.hero.level; float oldHp=solo.hero.maxHp,oldPower=solo.hero.power;
    solo.awardXP(125); check(solo.hero.level==oldLevel+1&&solo.hero.xp==0&&solo.hero.maxHp==oldHp+8&&solo.hero.power==oldPower+2,"level threshold grants permanent stats");
    solo.data.applyClass(solo.hero,Role::Rogue); solo.data.applyClass(solo.hero,Role::Mage);
    check(solo.hero.maxHp==oldHp+8&&solo.hero.power==oldPower+2,"class switching preserves level bonuses");
    solo.hero.pos=landmarks[1].pos; oldLevel=solo.hero.level; solo.tick(1.f/60);
    check(solo.discovered[1]&&solo.hero.level==oldLevel+1,"exploration grants XP and discovers landmark");
    int afterDiscovery=solo.hero.xp; solo.tick(1.f/60); check(solo.hero.xp==afterDiscovery,"discovery rewarded only once");
    check(!solo.world.blocked(landmarks[1].pos)&&!solo.world.blocked(landmarks[2].pos),"far landmarks are accessible");
    solo.world.pushGrass(solo.hero.pos,3,10); solo.world.tick(1.f/60);
    check(length(solo.world.grassBend(solo.hero.pos+Vec2{1,0}))>0,"grass physics follows distant exploration");
    solo.setTesting(true); key.keys="m"; solo.input(key); check(solo.screen==Screen::Map,"map opens");
    key.keys="3"; solo.input(key); check(solo.screen==Screen::Playing&&distance(solo.hero.pos,landmarks[2].pos)<.01f,"testing map travel reaches distant regions");
    solo.setCompanions(true); check(solo.party.size()==3,"optional local companions enabled");
    solo.setCompanions(false); check(solo.party.empty(),"solo mode removes companions");
    auto settingsPath=std::filesystem::temp_directory_path()/("eternal-settings-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".txt");
    solo.detail=2; solo.presentationFps=120; solo.showNames=false;
    check(solo.saveSettings(settingsPath),"settings saved");
    Game configured; check(configured.loadSettings(settingsPath)&&configured.detail==2&&configured.presentationFps==120&&!configured.showNames&&configured.hero.testing,"settings restored");
    { std::ofstream bad(settingsPath); bad<<"1 1 0 0 99 1 1 1 0"; }
    check(!configured.loadSettings(settingsPath)&&configured.presentationFps==120,"invalid settings rejected without partial mutation");
    std::filesystem::remove(settingsPath);
    InputParser escapeParser; escapeParser.feed("\x1b"); std::this_thread::sleep_for(std::chrono::milliseconds(35));
    check(escapeParser.feed("").keys==std::string(1,char(27)),"standalone terminal Escape reaches pause menu");
    solo.screen=Screen::Settings; solo.menuIndex=8; Framebuffer compact(40,16); solo.render(compact,16);
    bool backVisible=false; std::string compactText; for(const auto& cell:compact.cells) compactText+=utf8(cell.glyph);
    backVisible=compactText.find("> Back")!=std::string::npos; check(backVisible,"settings selection scrolls in compact terminal");
    solo.screen=Screen::Title; solo.menuIndex=0; Framebuffer menuFrame(200,60); solo.render(menuFrame,16);
    Input click; click.click=true; click.mouseX=64; click.mouseY=20; solo.input(click);
    check(solo.screen==Screen::Create,"clicking title opens character and world creation");

    auto treeSignature=[](const World& w) { std::ostringstream out; for(const auto& t:w.trees) out<<t.pos.x<<','<<t.pos.y<<','<<t.height<<','<<t.radius<<','<<t.conifer<<';'; return out.str(); };
    World streamed(918),same(918),different(919);
    std::string home=treeSignature(streamed);
    check(home==treeSignature(same)&&home!=treeSignature(different),"terrain determined by seed");
    check(World::chunkAt({-.1f,-64.1f})==World::ChunkKey{-1,-2},"negative chunk coordinates use floor");
    for(int i=1;i<=20;++i) streamed.focus({float(i*512),float(-i*256)});
    check(streamed.loadedChunkCount()==49&&streamed.trees.size()<=49*64,"chunk cache stays bounded during exploration");
    Vec2 remote{8192,-4096}; streamed.focus(remote); same.focus({-9000,6000}); same.focus(remote);
    check(treeSignature(streamed)==treeSignature(same),"chunk generation independent of travel order");
    streamed.focus({0,12}); check(treeSignature(streamed)==home,"unloaded chunks regenerate identically");
    bool varied=false; int initialBiome=streamed.biome({1000,1000});
    for(int i=1;i<30;++i) varied|=streamed.biome({float(i*512),float(i*-320)})!=initialBiome;
    check(varied,"procedural frontier contains varied biomes");
    GenerationConfig emptyTrees; emptyTrees.treeDensity=0;
    World openWorld(42,emptyTrees); check(!openWorld.blocked({10000,-12000}),"no original finite world wall");
    check(openWorld.blocked({1000001,0}),"numeric coordinate envelope enforced");
    Game spawning; check(!spawning.mobs.empty(),"procedural enemy spawns populated");
    Entity original=spawning.mobs.front(); auto spawnKey=std::make_tuple(original.spawnChunkX,original.spawnChunkY,original.spawnSlot);
    dealDamage(spawning.mobs.front(),1000000,spawning.texts); spawning.rewards();
    spawning.hero.pos={4096,-4096}; spawning.world.focus(spawning.hero.pos); spawning.streamEnemies();
    check(!spawning.mobs.empty(),"new remote region has encounters");
    spawning.hero.pos={0,12}; spawning.streamEnemies();
    auto isOriginal=[&](const Entity& e){return std::make_tuple(e.spawnChunkX,e.spawnChunkY,e.spawnSlot)==spawnKey;};
    check(std::none_of(spawning.mobs.begin(),spawning.mobs.end(),isOriginal),"unload and revisit preserves kill cooldown");
    spawning.time+=spawning.world.config.respawnSeconds+1; spawning.streamEnemies();
    check(std::any_of(spawning.mobs.begin(),spawning.mobs.end(),isOriginal),"enemies replenish after respawn time");
    for(int i=1;i<25;++i) { spawning.hero.pos={float(i*256),float(i*-384)}; spawning.world.focus(spawning.hero.pos); spawning.streamEnemies(); }
    std::set<int> ids; for(const auto& e:spawning.mobs) ids.insert(e.id);
    check(spawning.mobs.size()<=100&&ids.size()==spawning.mobs.size(),"streaming keeps bounded enemies and unique IDs");
    Game frontier; frontier.setTesting(true); key.keys="m"; frontier.input(key); key.keys="t"; frontier.input(key);
    check(frontier.screen==Screen::Playing&&frontier.hero.pos.x>500&&!frontier.world.blocked(frontier.hero.pos)&&!frontier.world.water(frontier.hero.pos),"frontier jump finds a walkable generated destination");
    auto modRoot=std::filesystem::temp_directory_path()/("eternal-mods-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(modRoot/"a"); std::filesystem::create_directories(modRoot/"b");
    { std::ofstream out(modRoot/"a/mod.txt"); out<<"api 1\nname \"First\"\n"; }
    { std::ofstream out(modRoot/"b/mod.txt"); out<<"api 1\nname \"Second\"\n"; }
    { std::ofstream out(modRoot/"enabled.txt"); out<<"a\nb\n"; }
    { std::ofstream out(modRoot/"a/world.txt"); out<<"tree_density 0\nenemies_per_chunk 1\n"; }
    { std::ofstream out(modRoot/"b/world.txt"); out<<"tree_density 0.9\nseed_salt 123\n"; }
    auto packs=loadMods(modRoot,{"a"});
    check(packs.names.size()==2&&packs.names[1]=="Second"&&packs.generation.treeDensity==.9f&&packs.generation.enemiesPerChunk==1,"ordered mod overrides and duplicate suppression");
    check(loadMods(modRoot,{},true).names.empty(),"no-mods bypasses enabled list");
    auto base=std::filesystem::path(ER_DATA_DIR);
    std::filesystem::copy_file(base/"enemies.txt",modRoot/"b/enemies.txt");
    { std::ofstream out(modRoot/"b/enemies.txt",std::ios::app); out<<"\n\"Mod Beast\" 99 4 10 3 12 2 80 20 0 100 120 140\n"; }
    Game modded(42,base,packs);
    check(modded.data.enemies.size()==4&&modded.data.enemies.back().name=="Mod Beast"&&modded.world.seed==42u&&modded.world.terrainSeed==(42u^hash(0)^123u),"mod content and generation settings applied");
    bool failed=false; try { loadMods(modRoot,{"../outside"}); } catch(const std::exception&) { failed=true; }
    check(failed,"invalid mod identifier rejected");
    { std::ofstream out(modRoot/"b/world.txt"); out<<"enemies_per_chunk -1\n"; }
    failed=false; try { loadMods(modRoot); } catch(const std::exception&) { failed=true; }
    check(failed,"invalid generation settings rejected");
    { std::ofstream out(modRoot/"b/world.txt"); out<<"tree_density 0.5\n"; }
    { std::ofstream out(modRoot/"b/mod.txt"); out<<"api 99\n"; }
    failed=false; try { loadMods(modRoot); } catch(const std::exception&) { failed=true; }
    check(failed,"unsupported mod API rejected");
    { std::ofstream out(modRoot/"b/enemies.txt",std::ios::app); out<<"\n\"Broken\" 99 4 10 9999 12 2 80 20 0 100 120 140\n"; }
    failed=false; try { Data invalid(base,{modRoot/"b"}); } catch(const std::exception&) { failed=true; }
    check(failed,"unsafe content magnitudes rejected");
    std::filesystem::remove_all(modRoot);
    Game movementGame;
    Input diagonal; diagonal.heldMovement=true;
    for(const auto& pair:std::vector<std::string>{"wa","wd","as","sd"}) {
        diagonal.keys="wwwwdd"; diagonal.movementKeys=pair; movementGame.input(diagonal);
        check((movementGame.velocity.x>0)==(pair.find('d')!=std::string::npos)&&(movementGame.velocity.y>0)==(pair.find('s')!=std::string::npos)&&std::abs(std::abs(movementGame.velocity.x)-std::abs(movementGame.velocity.y))<.001f&&std::abs(length(movementGame.velocity)-movementGame.hero.speed)<.001f,"held diagonals ignore repeat counts and preserve speed");
    }
    diagonal.keys.clear(); diagonal.movementKeys="w"; movementGame.input(diagonal);
    check(movementGame.velocity.x==0&&movementGame.velocity.y<0,"releasing one diagonal key retains the other axis");
    diagonal.movementKeys="wasd"; movementGame.input(diagonal); check(length(movementGame.velocity)==0,"opposite held directions cancel");
    diagonal.movementKeys.clear(); movementGame.input(diagonal); check(length(movementGame.velocity)==0,"release all stops movement");
    diagonal.keys="w"; movementGame.input(diagonal); check(length(movementGame.velocity)==0,"released native key does not move from stale key-down events");
    Framebuffer avatar(200,60); movementGame.render(avatar,16);
    int brightGold=0; for(const auto& c:avatar.cells) if(c.fg==Color{178,213,224}) ++brightGold;
    check(brightGold>5,"local player has a distinct bright silhouette");
    Framebuffer dense(320,100), readable(160,50);
    movementGame.render(dense,16,&readable);
    check(movementGame.camera.width==320&&movementGame.uiWidth==160,"UI resolution is independent of world detail");
    check(readable.at(80,25).glyph==0,"interface leaves the world visible between panels");
    check(readable.at(2,46).glyph==U'1',"hotbar uses readable interface coordinates");
    movementGame.screen=Screen::Paused;
    movementGame.render(dense,16,&readable);
    Input scaledClick; scaledClick.click=true; scaledClick.mouseX=44; scaledClick.mouseY=15;
    movementGame.input(scaledClick);
    check(movementGame.screen==Screen::Playing,"scaled pause menu accepts clicks at displayed coordinates");
    Framebuffer poseA(200,100),poseB(200,100);
    Camera animationCamera; animationCamera.width=200; animationCamera.height=100;
    Entity animated=movementGame.hero; animated.pos=animationCamera.center; animated.velocity={};
    playerAvatar(poseA,animationCamera,animated,0); playerAvatar(poseB,animationCamera,animated,.6f);
    check(poseA.cells!=poseB.cells,"idle animation changes the player silhouette");
    animated.velocity={animated.speed,0}; poseA.clear(); poseB.clear();
    playerAvatar(poseA,animationCamera,animated,0); playerAvatar(poseB,animationCamera,animated,.23f);
    check(poseA.cells!=poseB.cells,"walking animation alternates feet and arms");
    auto saveRoot=std::filesystem::temp_directory_path()/("eternal-saves-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    Game adventure; adventure.saveDirectory=saveRoot; adventure.beginCreation();
    check(adventure.draftSeed.size()==12&&!adventure.draftGod,"creation supplies 12 digits with God mode opt-in");
    adventure.draftName="Quinn"; adventure.draftWorld="Northern Reach"; adventure.draftSeed="999999999999";
    adventure.draftRole=Role::Rogue;
    check(adventure.createAdventure(),"new world creates its first save");
    check(adventure.world.seed==999999999999ULL&&adventure.hero.name=="Quinn"&&adventure.hero.role==Role::Rogue,"creation preserves full seed and character choices");
    check(adventure.hero.gold==0&&adventure.hero.inventory.empty()&&adventure.hero.level==1&&!adventure.hero.testing,"normal adventure starts with nothing at level one");
    adventure.hero.inventory={0,5}; adventure.inventoryIndex=1; adventure.equipSelected(); adventure.awardXP(300);
    adventure.hero.pos={120,80}; adventure.hero.hp=17; adventure.hero.gold=31; adventure.discovered[2]=true;
    adventure.loot.push_back({{121,80},6,2}); adventure.mobs[0].hp=7;
    check(adventure.saveAdventure(),"progress and equipment saved atomically");
    Game resumed; resumed.saveDirectory=saveRoot;
    check(resumed.loadAdventure(0),"save loads into a fresh game");
    check(resumed.hero.level==adventure.hero.level&&resumed.hero.hp==17&&resumed.hero.gold==31&&resumed.hero.pos.x==120&&resumed.hero.equipment[1]==5,"save restores progression position health and clothing");
    check(resumed.mobs[0].hp==7&&resumed.loot.back().item==6&&resumed.discovered[2],"save restores enemies dropped items and exploration");
    resumed.beginCreation(); resumed.draftName="Ada"; resumed.draftWorld="God's Sandbox"; resumed.draftSeed="000000000001"; resumed.draftGod=true;
    check(resumed.createAdventure()&&resumed.activeSave==1&&resumed.hero.testing&&resumed.seedText()=="000000000001","second save preserves first and keeps leading zero seed display");
    resumed.refreshSaves(); check(resumed.saves[0].exists&&resumed.saves[1].exists,"browser lists independent saves");
    check(resumed.deleteAdventure(1)&&std::filesystem::exists(saveRoot/"slot-1.sav")&&!std::filesystem::exists(saveRoot/"slot-2.sav"),"delete removes only the selected slot");
    { std::ofstream broken(saveRoot/"slot-2.sav"); broken<<"ETERNAL 1 truncated"; }
    auto oldName=resumed.hero.name;
    check(!resumed.loadAdventure(1)&&resumed.hero.name==oldName,"truncated save rejected without replacing current game");
    resumed.beginCreation(); resumed.draftName="Test"; resumed.draftSeed="123";
    check(!resumed.createAdventure(),"creation rejects non-twelve-digit seeds");
    World highSeed(1ULL<<32),lowSeed(0);
    check(highSeed.terrainSeed!=lowSeed.terrainSeed,"high seed digits contribute to procedural generation");
    resumed.beginCreation(5); resumed.draftName="Slot Six"; resumed.draftWorld="Six"; resumed.draftSeed="123456789012";
    check(resumed.createAdventure()&&resumed.activeSave==5,"creating from an empty slot uses that exact slot");
    resumed.refreshSaves(); resumed.screen=Screen::SaveActions; resumed.selectedSave=5; resumed.menuIndex=1;
    Input confirm; confirm.keys="\r"; resumed.menuInput(confirm);
    check(resumed.screen==Screen::DeleteSave&&std::filesystem::exists(saveRoot/"slot-6.sav"),"delete action asks for confirmation without removing data");
    resumed.menuInput(confirm);
    check(resumed.screen==Screen::Saves&&std::filesystem::exists(saveRoot/"slot-6.sav"),"delete confirmation defaults to cancel");
    resumed.screen=Screen::DeleteSave; resumed.menuIndex=1; resumed.menuInput(confirm);
    check(!std::filesystem::exists(saveRoot/"slot-6.sav"),"explicit confirmation deletes selected world");
    std::filesystem::remove_all(saveRoot);
    std::cout<<checks<<" checks passed: math, renderer, input, combat, AI, progression, boss, menus and deterministic replay\n";
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; } }
