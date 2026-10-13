#include "ascii_conversion/ascii.hpp"
#include "platform/terminal.hpp"
#include "world/world.hpp"
#include "combat/combat.hpp"
#include "game.hpp"
#include "renderer/terrain3d.hpp"
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
    Camera fp; fp.perspective=true; fp.firstPerson=true; fp.center={0,0}; fp.lookPitch=0;
    check(std::abs(fp.spriteScale({0,5})/fp.spriteScale({0,10})-2)<.01f,"perspective creature doubles in size at half distance");
    check(fp.spriteScale({0,-5})==0,"creatures behind camera have no sprite size");
    check(fp.deviceDepth({0,5})<fp.deviceDepth({0,10}),"entity depth matches near-to-far Metal ordering");
    Scene depthScene; depthScene.begin(fp);
    depthScene.put(10,10,{0,5},1,white,U'N',0);
    depthScene.put(10,10,{0,10},1,white,U'F',100);
    check(depthScene.pixels[size_t(10*fp.width+10)].shape==U'N',"near creatures win independently of world Y draw order");
    Game sprinter; sprinter.screen=Screen::Playing; Input run; run.heldMovement=true; run.movementKeys="wd";
    sprinter.input(run); float walking=length(sprinter.velocity); run.sprint=true; sprinter.input(run);
    check(std::abs(length(sprinter.velocity)-walking*1.8f)<.001f,"Shift sprint preserves normalized diagonal movement");
    run.sprint=false; sprinter.input(run); check(std::abs(length(sprinter.velocity)-walking)<.001f,"releasing Shift restores walking speed");
    run.movementKeys.clear(); run.sprint=true; sprinter.input(run); check(length(sprinter.velocity)==0,"Shift alone does not move player");
    {
        Game jumper; jumper.setTesting(true); Input jump; jump.keys=" "; jumper.input(jump);
        check(jumper.jumpSpeed>0,"Space jumps instead of casting");
        jumper.tick(.1f); float up=jumper.jumpHeight,vs=jumper.jumpSpeed;
        check(jumper.hero.cooldown[0]==0,"jump does not consume the primary ability cooldown");
        check(up>0&&jumper.camera.jumpHeight==up,"jump raises the camera with physical height");
        jumper.input(jump); check(jumper.jumpSpeed==vs,"cannot jump again in midair");
        jumper.screen=Screen::Paused; jumper.tick(.2f); check(jumper.jumpHeight==up,"pause freezes jump physics");
        jumper.screen=Screen::Playing; for(int i=0;i<60;++i) jumper.tick(1.f/60);
        check(jumper.jumpHeight==0&&jumper.jumpSpeed==0,"gravity returns the player to ground");
        check(std::find(jumper.soundEvents.begin(),jumper.soundEvents.end(),1)!=jumper.soundEvents.end(),"landing queues a sound effect");
        Game capped; capped.mobs.clear(); capped.hero.level=250; capped.spawnMob({900000,900000},0);
        check(capped.mobs.size()==1&&capped.mobs[0].level==80,"distant enemies stop at level 80 even with high-level player");
        float power=capped.mobs[0].power; capped.hero.level=300; capped.mobs.clear(); capped.spawnMob({900000,900000},0);
        check(capped.mobs[0].power==power,"enemies do not scale past level 80 with player progression");
        auto root=std::filesystem::temp_directory_path()/("eternal-cap-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        capped.saveDirectory=root; capped.activeSave=0; auto& mob=capped.mobs[0]; mob.level=100;
        mob.hp=mob.maxHp=capped.data.enemies[0].hp*(1+.2f*99); mob.power=capped.data.enemies[0].power*(1+.15f*99);
        check(capped.saveAdventure(),"legacy high-level mob fixture saves");
        Game restored; restored.saveDirectory=root; check(restored.loadAdventure(0),"legacy high-level mob save loads");
        check(restored.mobs[0].level==80&&std::abs(restored.mobs[0].power-power)<.01f,"loaded mobs clamp level and scaled power");
        std::filesystem::remove_all(root);
        World lake(42); Terrain3D lakeMesh; lakeMesh.update(lake,{0,40});
        bool foundWater=false,flatWater=true;
        for(const auto& chunk:lakeMesh.chunks) for(size_t i=0;i+2<chunk.second.vertices.size();i+=3) {
            const auto& a=chunk.second.vertices[i]; const auto& b=chunk.second.vertices[i+1]; const auto& c=chunk.second.vertices[i+2];
            if(std::abs(a.color[0]-36.f/255)<.00001f&&std::abs(a.color[2]-126.f/255)<.00001f) {
                foundWater=true; flatWater&=a.position[2]==b.position[2]&&a.position[2]==c.position[2]&&a.normal[2]==1;
            }
        }
        bool eyeAboveWater=true,swimSpot=false;
        for(int y=40;y<70;++y) for(int x=-30;x<30;++x) if(lake.water({float(x),float(y)})) {
            Camera swim; swim.perspective=swim.firstPerson=true; swim.terrain=&lake; swim.center={float(x),float(y)};
            eyeAboveWater&=swim.view().eye[2]>lake.waterLevel(swim.center); swimSpot=true;
        }
        check(swimSpot&&eyeAboveWater,"first-person eye stays above the lake surface");
        check(foundWater&&flatWater,"all lake triangles have horizontal surfaces and upward normals");
        bool highWater=false; for(int y=-3000;y<3000;y+=67) for(int x=-3000;x<3000;x+=67) {
            Vec2 p{float(x),float(y)}; if(lake.elevation(p)>20&&lake.water(p)) highWater=true;
        }
        check(!highWater,"noise stripes cannot create water on high mountain slopes");
    }
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
    {
    GenerationConfig bare; bare.treeDensity=0; World relief(42,bare),same(42,bare),different(43,bare);
    float low=1e6f,high=-1e6f; bool varies=false,cliff=false,uphill=false;
    for(int y=400;y<1800;y+=17) for(int x=400;x<1800;x+=17) {
        Vec2 p{float(x),float(y)}; auto t=relief.terrain(p);
        low=std::min(low,t.height); high=std::max(high,t.height);
        varies|=std::abs(t.height-different.elevation(p))>1;
        if(!relief.road(p)&&!placeClearing(p,relief.terrainSeed)&&length(t.slope)>1.15f) cliff|=relief.blocked(p);
        if(!uphill&&length(t.slope)>.12f&&length(t.slope)<.5f&&!relief.blocked(p)) {
            Vec2 dir=normalized(t.slope); float up=distance(p,relief.move(p,dir*.1f)),down=distance(p,relief.move(p,dir*-.1f));
            uphill=up>0&&up<down*.95f;
        }
    }
    check(high-low>35&&varies,"warped heightmap has seed-specific large-scale relief");
    check(cliff&&uphill,"terrain blocks cliffs and slows uphill movement");
    Vec2 edge{640,768}; float before=relief.elevation(edge); relief.focus(edge);
    check(std::abs(before-relief.elevation(edge))<.0001f&&std::abs(before-same.elevation(edge))<.0001f,"cached and unloaded height samples agree");
    check(std::abs(relief.elevation(edge+Vec2{.001f,0})-relief.elevation(edge-Vec2{.001f,0}))<.02f,"height is continuous across chunk boundaries");
    Camera camera; camera.terrain=&relief; camera.center=edge;
    Vec2 point=edge+Vec2{8,5}; check(distance(camera.unproject(camera.project(point)),point)<.08f,"terrain projection preserves mouse-to-world alignment");
    relief.focus({10000,10000}); check(relief.loadedChunkCount()==49&&std::abs(before-relief.elevation(edge))<.0001f,"terrain unloads and regenerates deterministically");
    }
    {
    World world3(42); world3.focus({640,768}); Camera camera3; camera3.terrain=&world3; camera3.perspective=true; camera3.center={640,768}; camera3.width=160; camera3.height=50;
    bool accurate=true;
    for(float yaw:{0.f,.6f,1.8f,3.1f}) { camera3.yaw=yaw; for(Vec2 delta:{Vec2{0,0},Vec2{3,2},Vec2{-4,1}}) {
        Vec2 p=camera3.center+delta,hit; accurate&=camera3.pick(camera3.project(p),hit)&&distance(hit,p)<.005f;
    } }
    check(accurate,"perspective ray casting inverts projection across orbit angles");
    camera3.yaw=0; auto nearScreen=camera3.project(camera3.center+Vec2{4,0}); camera3.zoom=1.5f;
    check(std::abs(camera3.project(camera3.center+Vec2{4,0}).x-80)>std::abs(nearScreen.x-80),"perspective zoom changes projected scale");
    {
        Camera eyes=camera3; eyes.center={0,12}; // Dry ground; swimming height has its own test above.
        eyes.firstPerson=true; eyes.lookPitch=15*pi/180; eyes.zoom=1;
        auto first=eyes.view();
        check(std::abs(first.eye[0]-eyes.center.x)<.001f&&std::abs(first.eye[1]-eyes.center.y)<.001f&&std::abs(first.eye[2]-eyes.ground(eyes.center)-1.75f)<.001f,"first-person eye is attached to player terrain height");
        Vec2 hit; check(eyes.pick({80,25},hit)&&dot(hit-eyes.center,Vec2{first.forward[0],first.forward[1]})>0,"first-person crosshair ray hits terrain in front");
        Vec2 projected=eyes.project(hit); check(distance(projected,{80,25})<.01f,"first-person crosshair uses the rendering projection");
        eyes.yaw=pi/2; auto turned=eyes.view(); check(turned.forward[0]>.9f&&std::abs(turned.forward[1])<.001f,"first-person mouse yaw turns camera without orbiting position");
        check(turned.eye==first.eye,"turning first-person camera leaves eye at player");
        check(eyes.project(eyes.center+Vec2{1000,0}).x<0,"first-person clips distant glyph objects beyond the terrain far plane");
    }
    auto eye=camera3.view().eye; check(eye[2]>camera3.ground(camera3.center),"orbit eye is above terrain target");
    Terrain3D mesh; mesh.update(world3,camera3.center); auto revision=mesh.revision; mesh.update(world3,camera3.center);
    check(mesh.chunks.size()==49&&mesh.revision==revision,"terrain mesh cache is bounded and reused without uploads");
    bool normals=true; for(const auto& v:mesh.chunks.begin()->second.vertices) { float n=v.normal[0]*v.normal[0]+v.normal[1]*v.normal[1]+v.normal[2]*v.normal[2]; normals&=std::abs(n-1)<.001f&&std::isfinite(v.position[2]); }
    check(normals,"mesh supplies finite positions and normalized terrain normals");
    auto before=mesh.chunks.begin()->second.vertices.front(); auto key=mesh.chunks.begin()->first;
    world3.focus({10000,10000}); mesh.update(world3,{10000,10000}); world3.focus({640,768}); mesh.update(world3,{640,768});
    check(mesh.chunks.size()==49&&mesh.chunks.at(key).vertices.front().position==before.position,"3D mesh regeneration preserves seeded heights");
    Game orbit; orbit.camera.perspective=true; orbit.camera.terrain=&orbit.world; Input drag; drag.orbitX=100; drag.orbitY=100; orbit.input(drag);
    check(orbit.camera.yaw>.5f&&orbit.camera.tilt<=65*pi/180,"orbit input rotates and bounds camera pitch");
    orbit.camera.yaw=0; Input forward3; forward3.keys="w"; orbit.input(forward3);
    check(orbit.velocity.y>0&&std::abs(orbit.velocity.x)<.001f,"W advances into the perspective view");
    orbit.velocity={};
    Input aim3; auto target=orbit.camera.project(orbit.hero.pos+Vec2{2,1}); aim3.mouseX=int(target.x); aim3.mouseY=int(target.y); aim3.click=true; orbit.input(aim3); orbit.tick(1.f/60);
    check(orbit.hero.cooldown[0]>0&&distance(orbit.aim,orbit.hero.pos)<8,"perspective mouse aim still casts through existing combat");
    }
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
    Atlas atlasA(42),atlasB(42),atlasC(43);
    check(atlasA.place({2,-1}).name==atlasB.place({2,-1}).name&&atlasA.place({2,-1}).key!=atlasC.place({2,-1}).key,"seeded sites reproduce and vary across seeds");
    check(distance(atlasA.place({0,0}).pos,{0,12})<60,"a settlement is reachable immediately after spawning");
    World roads(42); auto startSite=atlasA.place({0,0}),nextSite=atlasA.place({1,0});
    bool connected=true; for(int i=0;i<=40;++i) connected=connected&&roads.road(lerp(startSite.pos,nextSite.pos,i/40.f));
    check(connected,"roads connect adjacent sites across generation seams");
    check(atlasA.discover({0,12}).size()==1&&atlasA.discover({0,12}).empty(),"site discovery occurs once");
    check(atlasA.seen({0,12})&&!atlasA.seen({900,900}),"fog separates explored from unknown terrain");
    std::stringstream atlasSave; atlasA.save(atlasSave); Atlas atlasRestored(42);
    check(atlasRestored.load(atlasSave)&&atlasRestored.known==atlasA.known&&atlasRestored.explored==atlasA.explored,"atlas discovery persists");
    atlasA.focus({9000,-8000}); check(atlasA.nearby.size()==9,"site streaming stays bounded while exploring");
    Game mapGame; mapGame.atlas.discover(mapGame.hero.pos); Framebuffer mapFrame(160,50); mapGame.renderUI(mapFrame,16);
    bool playerMarker=false; for(int y=4;y<12;++y) for(int x=135;x<158;++x) playerMarker|=mapFrame.at(x,y).glyph==U'@';
    check(playerMarker,"minimap marks player without replacing existing HUD");
    Interior dungeonA(17,false),dungeonB(17,false),dungeonC(18,false);
    check(dungeonA.tiles==dungeonB.tiles,"interior floorplans reproduce from seed");
    std::set<std::pair<int,int>> reached; std::vector<Vec2> queue{dungeonA.entrance};
    for(size_t i=0;i<queue.size();++i) for(Vec2 d:{Vec2{1,0},{-1,0},{0,1},{0,-1}}) { Vec2 p=queue[i]+d; if(!dungeonA.blocked(p)&&reached.insert({int(p.x),int(p.y)}).second) queue.push_back(p); }
    check(reached.count({int(dungeonA.chest.x),int(dungeonA.chest.y)}),"dungeon chest reachable from entry");
    Game explorer; explorer.hero.pos=explorer.atlas.place({0,0}).pos; Vec2 outdoor=explorer.hero.pos;
    size_t outdoorCount=explorer.mobs.size(); explorer.interactPlace();
    check(explorer.world.interior&&explorer.world.interior->peaceful&&explorer.mobs.empty(),"village entrance opens a peaceful building");
    explorer.leaveInterior(); check(!explorer.world.interior&&explorer.hero.pos.x==outdoor.x&&explorer.mobs.size()==outdoorCount,"exit restores outdoor state");
    Place dangerous=explorer.atlas.place({0,0});
    for(int x=1;explorer.atlas.types[size_t(dangerous.type)].inhabited||!explorer.atlas.types[size_t(dangerous.type)].interior;++x) dangerous=explorer.atlas.place({x,0});
    explorer.hero.pos=dangerous.pos; explorer.interactPlace();
    check(explorer.world.interior&&!explorer.world.interior->peaceful&&explorer.mobs.size()==2,"hostile site spawns dungeon guardians");
    explorer.hero.pos=explorer.world.interior->chest; int chestGold=explorer.hero.gold;
    explorer.interactPlace(); check(explorer.hero.gold==chestGold,"guardians protect dungeon chest");
    for(auto& e:explorer.mobs) e.alive=false;
    explorer.interactPlace(); check(explorer.clearedPlaces.count(dangerous.id)&&explorer.hero.gold==chestGold+60,"cleared dungeon chest grants reward once");
    explorer.interactPlace(); check(explorer.hero.gold==chestGold+60,"chest cannot be farmed repeatedly");
    auto interiorRoot=std::filesystem::temp_directory_path()/("er-interior-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    explorer.saveDirectory=interiorRoot; explorer.activeSave=0; check(explorer.saveAdventure(),"interior progress saved");
    Game returning; returning.saveDirectory=interiorRoot;
    check(returning.loadAdventure(0)&&returning.world.interior&&returning.world.interior->opened,"save resumes inside the same cleared dungeon");
    returning.leaveInterior(); check(distance(returning.hero.pos,dangerous.pos)<.1f,"saved interior remembers outdoor return location");
    std::filesystem::remove_all(interiorRoot);
    Game social; auto town=social.atlas.place({0,0});
    social.atlas.focus(town.pos); social.population.tick(social.atlas,social.world,0,0,false);
    auto citizen=social.population.person(town,1),sameCitizen=social.population.person(town,1);
    check(citizen.name==sameCitizen.name&&citizen.age==sameCitizen.age,"NPC identity is deterministic");
    check(distance(social.population.scheduled(citizen,social.atlas,12,false),social.population.scheduled(citizen,social.atlas,23,false))>3,"day and night schedules have different destinations");
    check(social.population.nearby.size()<=108&&social.population.nearby.size()>=12,"nearby population is bounded and starter town inhabited");
    Game otherHistory(777); check(social.chronicle.account(0)!=otherHistory.chronicle.account(0),"different seeds generate different named history");
    social.hero.pos=social.population.nearby.front().pos; social.talkNearby();
    check(social.screen==Screen::Talk,"nearby NPC opens dialogue");
    social.talking=citizen.id; social.offerQuest(citizen);
    check(social.quests.size()==1&&social.atlas.known.count(social.quests[0].target),"NPC offers quest at real marked location");
    auto contract=social.quests[0]; auto targetSite=social.atlas.place(contract.target);
    check(!social.atlas.types[size_t(targetSite.type)].inhabited&&social.atlas.types[size_t(targetSite.type)].interior,"quest target is a reachable hostile interior");
    social.screen=Screen::Playing; social.hero.pos=targetSite.pos; social.interactPlace();
    for(auto& e:social.mobs) e.alive=false;
    social.hero.pos=social.world.interior->chest; social.interactPlace(); social.leaveInterior();
    int rewardGold=social.hero.gold; social.offerQuest(citizen);
    check(social.quests[0].rewarded&&social.hero.gold==rewardGold+90,"town dungeon return loop pays quest reward");
    social.offerQuest(citizen); check(social.hero.gold==rewardGold+90&&social.quests.size()==2,"repeat work creates new target without duplicating payout");
    social.hero.gold=5000; size_t beforeTrade=social.hero.inventory.size(); social.tradeWith(citizen,0);
    check(social.hero.inventory.size()==beforeTrade+1&&social.hero.gold<5000,"shop exchanges gold for inventory item");
    social.inventoryIndex=int(social.hero.inventory.size())-1; social.tradeWith(citizen,3);
    check(social.hero.inventory.size()==beforeTrade,"shop buys unequipped inventory item");
    social.learnLore(3); check(social.chronicle.learned.count(3)&&social.atlas.known.count(social.chronicle.events[3].place),"lore reveals a real place named in history");
    auto livingRoot=std::filesystem::temp_directory_path()/("er-living-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    social.saveDirectory=livingRoot; social.activeSave=0; check(social.saveAdventure(),"living world saves");
    Game livingRestored; livingRestored.saveDirectory=livingRoot;
    check(livingRestored.loadAdventure(0)&&livingRestored.quests.size()==2&&livingRestored.quests[0].rewarded&&livingRestored.chronicle.learned.count(3),"quest and lore progress persist");
    check(livingRestored.chronicle.account(0)==social.chronicle.account(0)&&livingRestored.population.changes[citizen.id].gold==social.population.changes[citizen.id].gold,"structured history and merchant balances persist");
    std::filesystem::remove_all(livingRoot);
    World climateWorld(918); std::set<int> climates;
    for(int y=-5000;y<=5000;y+=128) for(int x=-5000;x<=5000;x+=128) climates.insert(climateWorld.biome({float(x),float(y)}));
    check(climates.count(4)&&climates.count(5)&&climates.count(6),"procedural frontier includes desert mountain and coast climates");
    check(climates.size()==8,"height moisture and temperature generate all eight biomes");
    bool blended=false,waterFound=false; float worstWeightSum=0,worstEdge=0;
    for(int i=0;i<200;++i) {
        Vec2 p{float(i*39-3000),float(i*17-2100)};
        auto first=climateWorld.climate(climateWorld.terrain(p)),near=climateWorld.climate(climateWorld.terrain(p+Vec2{.01f,0}));
        float sum=0; int contributors=0;
        for(int b=0;b<8;++b) { sum+=first.weights[size_t(b)]; contributors+=first.weights[size_t(b)]>.1f; worstEdge=std::max(worstEdge,std::abs(first.weights[size_t(b)]-near.weights[size_t(b)])); }
        worstWeightSum=std::max(worstWeightSum,std::abs(sum-1)); blended|=contributors>1;
        waterFound|=climateWorld.water(p);
    }
    check(blended&&worstWeightSum<.0001f&&worstEdge<.03f,"biome weights normalize and blend continuously at edges");
    check(waterFound,"frontier contains height basins or river channels");
    World plantWorld(42); plantWorld.focus({-3000,-3000}); size_t forestTrees=plantWorld.trees.size();
    auto remembered=plantWorld.trees; plantWorld.focus({10000,10000}); plantWorld.focus({-3000,-3000});
    bool matching=remembered.size()==plantWorld.trees.size();
    for(size_t i=0;i<remembered.size()&&matching;++i) matching=remembered[i].species==plantWorld.trees[i].species&&distance(remembered[i].pos,plantWorld.trees[i].pos)<.001f;
    check(matching&&forestTrees>0,"biome vegetation survives chunk unload and regeneration identically");

    check(worldHour(0)==8&&worldHour(45*16)==0,"world clock advances and wraps deterministically");
    check(weatherAt(42,360)==weatherAt(42,360),"weather is determined by seed and time");
    Environment wildlife; wildlife.tick(climateWorld,{300,300},{},0,0);
    check(!wildlife.animals.empty()&&wildlife.animals.size()<=18,"wildlife simulation stays bounded");
    Framebuffer daylight(120,40),nightlight(120,40); for(auto& c:daylight.cells) c={U'.',white,background}; nightlight.cells=daylight.cells;
    Camera climateCamera; wildlife.atmosphere(daylight,climateCamera,42,0); wildlife.atmosphere(nightlight,climateCamera,42,45*15);
    check(daylight.cells!=nightlight.cells,"day and night have visibly different atmosphere");
    {
    Game crafter; crafter.materials={3,3,2}; crafter.hero.gold=100;
    crafter.craft(0); check(crafter.hero.inventory.back()==6&&crafter.materials[0]==0&&crafter.hero.gold==90,"smithing consumes resources and creates a usable weapon");
    size_t craftedCount=crafter.hero.inventory.size(); crafter.craft(0);
    check(crafter.hero.inventory.size()==craftedCount&&crafter.hero.gold==90,"crafting rejects insufficient materials without charging");
    crafter.hero.hp=1; crafter.craft(1); check(crafter.hero.hp==51&&crafter.materials[1]==0,"alchemy consumes herbs to restore health");
    auto priorMaterials=crafter.materials; crafter.gather(); auto gatheredMaterials=crafter.materials; crafter.gather();
    check(crafter.materials==gatheredMaterials&&priorMaterials!=gatheredMaterials,"gathering limited to once per chunk per day");
    crafter.mastery[size_t(crafter.hero.role)]=20; crafter.screen=Screen::Progress; crafter.menuIndex=1;
    float untalentedPower=crafter.hero.power; Input learn; learn.keys="\r"; crafter.progressInput(learn);
    check(crafter.hero.power==untalentedPower+2&&crafter.perkPoints()==0,"mastery earns spendable class talent points");
    auto trainedRole=crafter.hero.role; crafter.applyPlayerClass(Role::Warrior); crafter.applyPlayerClass(trainedRole);
    check(crafter.hero.power==untalentedPower+2,"class switching retains that class's talents without stacking");
    crafter.hero.inventory.assign(20,1); check(!crafter.canCarry(0),"carrying capacity prevents overloading with more items");
    Game criminal; criminal.population.tick(criminal.atlas,criminal.world,0,0,false);
    auto victim=criminal.population.person(criminal.atlas.place({0,0}),1);
    criminal.hero.pos=criminal.population.scheduled(victim,criminal.atlas,8,false);
    criminal.stealFrom(victim); check(criminal.bounties[victim.home]==25,"witnessed theft creates a local bounty");
    criminal.tickSociety(1);
    check(std::any_of(criminal.mobs.begin(),criminal.mobs.end(),[](const Entity& e){return e.name=="Pursuing Guard";}),"nearby guard pursues a wanted player");
    auto guard=criminal.population.person(criminal.atlas.place({0,0}),2); int beforeFine=criminal.hero.gold;
    criminal.settleBounty(guard); check(criminal.bounties[victim.home]==0&&criminal.hero.gold==beforeFine-25,"fine payment clears local bounty");
    criminal.bounties[victim.home]=25; criminal.hero.gold=0; float beforeJail=criminal.time; criminal.settleBounty(guard);
    check(criminal.time==beforeJail+270&&criminal.bounties[victim.home]==0,"unpayable fine advances time through a jail sentence");
    auto progressionRoot=std::filesystem::temp_directory_path()/("er-progress-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    crafter.saveDirectory=progressionRoot; crafter.activeSave=0; crafter.bounties[{0,0}]=25;
    check(crafter.saveAdventure(),"progression state saved");
    Game trained; trained.saveDirectory=progressionRoot;
    check(trained.loadAdventure(0)&&trained.talents==crafter.talents&&trained.materials==crafter.materials&&trained.harvested==crafter.harvested&&trained.bounties==crafter.bounties,"talents materials harvest and crime state persist");
    Game legacyFixture; legacyFixture.saveDirectory=progressionRoot; legacyFixture.activeSave=1; legacyFixture.saveAdventure();
    std::ifstream legacyIn(progressionRoot/"slot-2.sav"); std::stringstream legacyBuffer; legacyBuffer<<legacyIn.rdbuf(); std::string latestSave=legacyBuffer.str();
    const char* sections[]={"\nATLAS", "\nINTERIORS", "\nLIVING", "\nPROGRESSION", "\nQUEST_DETAILS"};
    for(int version=1;version<=5;++version) {
        std::string old=latestSave; old[8]=char('0'+version); old.resize(old.find(sections[version-1]));
        { std::ofstream file(progressionRoot/"slot-3.sav"); file<<old<<'\n'; }
        Game legacyReader; legacyReader.saveDirectory=progressionRoot; check(legacyReader.loadAdventure(2),"previous save format remains readable");
    }
    std::filesystem::remove_all(progressionRoot);
    }
    {
    Game courier; auto giver=courier.population.person(courier.atlas.place({0,0}),1);
    courier.quests.push_back({giver.id,{2,2},0,true,true,"previous job"});
    courier.offerQuest(giver);
    check(courier.quests.back().kind==QuestKind::Delivery,"second contract is a real delivery");
    auto recipient=courier.quests.back().contact;
    courier.deliverQuests(giver.id); check(!courier.quests.back().complete,"wrong recipient cannot complete delivery");
    courier.deliverQuests(recipient); check(courier.quests.back().complete,"named recipient completes delivery");
    int before=courier.hero.gold; courier.offerQuest(giver);
    check(courier.hero.gold==before+90&&courier.quests.back().rewarded,"delivery pays only after return");
    courier.offerQuest(giver);
    check(courier.hero.gold==before+90&&courier.quests.back().kind==QuestKind::Escort,"next job is escort without duplicate delivery payout");
    auto& escort=courier.quests.back(); courier.hero.pos=escort.escortPos+Vec2{6,0}; courier.mobs.clear();
    auto start=escort.escortPos; courier.tickQuests(.1f);
    check(distance(start,escort.escortPos)>0,"escort walks toward nearby player");
    courier.hero.pos=escort.escortPos+Vec2{6,0}; Entity threat; threat.alive=true; threat.pos=escort.escortPos; courier.mobs.push_back(threat);
    start=escort.escortPos; courier.tickQuests(.1f); check(distance(start,escort.escortPos)==0,"escort waits for nearby threats to be cleared"); courier.mobs.clear();
    courier.hero.pos=courier.atlas.place({0,0}).pos; courier.interactPlace(); check(!courier.world.interior,"escort blocks interior transitions");
    courier.hero.pos=escort.escortPos+Vec2{40,0}; start=escort.escortPos; courier.tickQuests(.1f);
    check(distance(start,escort.escortPos)==0,"escort waits when player runs too far ahead");
    courier.screen=Screen::Map; courier.mapSelected=PlaceId{0,0}; courier.atlas.visited.insert({0,0}); Input enter; enter.keys="\r"; courier.menuInput(enter);
    check(courier.screen==Screen::Map&&distance(start,escort.escortPos)==0,"escort prevents fast travel");
    auto questRoot=std::filesystem::temp_directory_path()/("er-quests-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    courier.saveDirectory=questRoot; courier.activeSave=0; check(courier.saveAdventure(),"active escort saves");
    Game resume; resume.saveDirectory=questRoot;
    check(resume.loadAdventure(0)&&resume.escortActive()&&distance(resume.quests.back().escortPos,escort.escortPos)<.01f&&resume.population.away.count(escort.contact),"escort restores position identity and hidden home resident");
    escort.escortPos=courier.atlas.place(escort.target).pos; courier.hero.pos=escort.escortPos; courier.tickQuests(.1f);
    check(escort.complete&&!courier.escortActive(),"escort completes only when both reach destination");
    before=courier.hero.gold; courier.offerQuest(giver); check(courier.hero.gold==before+90&&escort.rewarded,"escort returns for reward");
    std::filesystem::remove_all(questRoot);
    }
    Game travelRules; travelRules.atlas.known.insert({2,2}); travelRules.mapSelected=PlaceId{2,2}; travelRules.screen=Screen::Map;
    Input travelKey; travelKey.keys="\r"; travelRules.menuInput(travelKey);
    check(travelRules.screen==Screen::Map,"rumors mark places without unlocking unvisited fast travel");
    travelRules.atlas.discover(travelRules.atlas.place({2,2}).pos); travelRules.menuInput(travelKey);
    check(travelRules.screen==Screen::Playing,"visiting a place unlocks fast travel");
    std::cout<<checks<<" checks passed: math, renderer, input, combat, AI, progression, boss, menus and deterministic replay\n";
} catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; } }
