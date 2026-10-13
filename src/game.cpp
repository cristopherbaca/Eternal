#include "game.hpp"
#include "ui/hud.hpp"
#include "ai/ai.hpp"
#include "assets/dragon.hpp"
#include <algorithm>
#include <cmath>
namespace er {
Game::Game(uint64_t seed,const std::filesystem::path& dataPath,const ModSet& mods):sourceData(dataPath),sourceMods(mods),activeMods(mods.names),atlas(uint32_t(seed)^hash(uint32_t(seed>>32))^mods.generation.seedSalt,dataPath),population(dataPath),chronicle(atlas,population,dataPath),data(dataPath,mods.directories),world(seed,mods.generation) {
    data.applyClass(hero,Role::Mage);
    dragon.id=500; dragon.kind=Kind::Boss; dragon.name="Dragon"; dragon.pos=dragon.home={30,-30}; dragon.hp=dragon.maxHp=data.boss.hp; dragon.armor=data.boss.armor; dragon.power=data.boss.power; dragon.speed=data.boss.speed;
    discovered[0]=true; atlas.discover(hero.pos);
    mobs.reserve(128);
    streamEnemies();
}
void Game::spawnMob(Vec2 pos,int archetype) {
    if(mobs.size()>=100) return;
    const auto& def=data.enemies.at(size_t(archetype)); Entity e; e.id=nextMobId++; e.name=def.name; e.kind=Kind::Mob; e.archetype=archetype;
    e.pos=e.home=pos; e.maxHp=e.hp=def.hp; e.armor=def.armor; e.power=def.power; e.speed=def.speed; e.color=def.color; e.role=archetype==1?Role::Mage:Role::Warrior; e.level=1+std::min(79,int(distance(pos,{0,12})/world.config.levelDistance)); e.hp=e.maxHp*=1+.2f*(e.level-1); e.power*=1+.15f*(e.level-1); if(archetype==0&&hash(uint32_t(e.id)^world.terrainSeed)%7==0) e.name="Wolf"; mobs.push_back(e);
}
void Game::streamEnemies() {
    auto center=World::chunkAt(hero.pos); enemyCenter=center;
    for(auto it=respawnAfter.begin();it!=respawnAfter.end();) {
        if(it->second<=time) it=respawnAfter.erase(it); else ++it;
    }
    // Evict only far-away encounters. IDs are never derived from vector indices.
    mobs.erase(std::remove_if(mobs.begin(),mobs.end(),[&](const Entity& e) {
        return distance(e.home,hero.pos)>180&&distance(e.pos,hero.pos)>150;
    }),mobs.end());
    loot.erase(std::remove_if(loot.begin(),loot.end(),[&](const Loot& item){return distance(item.pos,hero.pos)>200;}),loot.end());
    for(int y=center.second-1;y<=center.second+1;++y) for(int x=center.first-1;x<=center.first+1;++x) {
        for(int slot=0;slot<world.config.enemiesPerChunk;++slot) {
            SpawnKey key{x,y,slot}; if(respawnAfter.count(key)) continue;
            bool exists=false;
            for(const auto& e:mobs) if(e.spawnSlot==slot&&e.spawnChunkX==x&&e.spawnChunkY==y) { exists=true; break; }
            if(exists) continue;
            // Coordinate hashing means visiting chunks in a different order gives the same spawns.
            Vec2 p{float(x*64)+4+noise(x,y,world.terrainSeed+uint32_t(slot*71+83))*56,
                   float(y*64)+4+noise(x,y,world.terrainSeed+uint32_t(slot*97+211))*56};
            if(world.blocked(p)||world.water(p)||distance(p,hero.pos)<12) continue;
            int archetype=std::min(int(data.enemies.size())-1,int(noise(x,y,world.terrainSeed+uint32_t(slot+811))*data.enemies.size()));
            size_t before=mobs.size(); spawnMob(p,archetype);
            if(mobs.size()>before) { auto& e=mobs.back(); e.spawnChunkX=x; e.spawnChunkY=y; e.spawnSlot=slot; }
        }
    }
    if(hero.target!=dragon.id&&std::none_of(mobs.begin(),mobs.end(),[&](const Entity& e){return e.id==hero.target;})&&
       std::none_of(party.begin(),party.end(),[&](const Entity& e){return e.id==hero.target;})) hero.target=0;
}
void Game::collectLoot() {
    for(auto it=loot.begin();it!=loot.end();) {
        if(distance(hero.pos,it->pos)<4&&canCarry(it->item)) { hero.inventory.push_back(it->item); hero.gold+=it->gold; message="Collected "+data.items[size_t(it->item)].name+" +"+std::to_string(it->gold)+"g"; it=loot.erase(it); }
        else ++it;
    }
}
void Game::equipSelected() {
    if(hero.inventory.empty()) return;
    inventoryIndex=clamp(inventoryIndex,0,int(hero.inventory.size())-1); int id=hero.inventory[size_t(inventoryIndex)]; const auto& item=data.items[size_t(id)];
    int old=hero.equipment[size_t(item.slot)]; if(old==id) { hero.armor-=item.armor; hero.power-=item.power; hero.equipment[size_t(item.slot)]=-1; message="Unequipped "+item.name; return; } if(old>=0) { hero.armor-=data.items[size_t(old)].armor; hero.power-=data.items[size_t(old)].power; }
    hero.equipment[size_t(item.slot)]=id; hero.armor+=item.armor; hero.power+=item.power; message="Equipped "+item.name;
}
void Game::respawn() {
    if(hero.alive) return;
    if(world.interior) leaveInterior();
    hero.alive=true; hero.hp=hero.maxHp; hero.mp=hero.maxMp; hero.pos={0,12}; hero.poison=hero.slow=hero.hurtTime=0; hero.cooldown.fill(0); hero.gold=std::max(0,hero.gold-25); velocity={}; jumpHeight=jumpSpeed=0; camera.jumpHeight=0;
    combat.areas.clear(); combat.projectiles.clear();
    for(size_t i=0;i<party.size();++i) { auto& e=party[i]; e.alive=true; e.hp=e.maxHp; e.mp=e.maxMp; e.pos=hero.pos+Vec2{float(i)*4-4,3}; }
    message="Returned to Ashenmere (-25g).";
}
void Game::rewards() {
    for(auto& e:mobs) {
        if(!e.alive&&!e.rewarded) { const auto& def=data.enemies[size_t(e.archetype)]; e.rewarded=true; awardXP(def.xp*e.level); if(e.spawnSlot>=0) respawnAfter[{e.spawnChunkX,e.spawnChunkY,e.spawnSlot}]=time+world.config.respawnSeconds; loot.push_back({e.pos,def.item,def.gold}); if(e.id%3==0&&data.items.size()>7) loot.push_back({e.pos+Vec2{1,0},5+(e.id/3)%3,0}); message=def.name+" defeated. +"+std::to_string(def.xp*e.level)+" XP. E: collect loot"; }
        if(!world.interior&&!e.alive&&e.deadTime>world.config.respawnSeconds&&!e.summoned) { e.alive=true; e.hp=e.maxHp; e.pos=e.home; e.rewarded=false; e.poison=0; e.brain=Brain::Idle; }
    }
    if(loot.size()>200) loot.erase(loot.begin(),loot.begin()+int(loot.size()-200));
}
std::vector<Entity*> Game::entities() { std::vector<Entity*> es; es.push_back(&hero); for(auto& e:party) es.push_back(&e); for(auto& e:mobs) es.push_back(&e); if(dragon.id==500&&!world.interior) es.push_back(&dragon); return es; }
void Game::setScene(const std::string& name) {
    if(name=="grass") hero.pos={0,12}; else if(name=="water") hero.pos={0,34}; else if(name=="forest") hero.pos={-33,3}; else if(name=="ruins") hero.pos={0,-2}; else if(name=="boss") hero.pos={30,-17};
    world.focus(hero.pos);
    camera.center=hero.pos; aim=hero.pos+Vec2{7,-3};
    if(name=="boss") { aim=dragon.pos; hero.target=dragon.id; }
    for(size_t i=0;i<party.size();++i) party[i].pos=hero.pos+Vec2{float(i)*5-5,3};
}
void Game::input(const Input& in) {
    if(screen!=Screen::Playing) { menuInput(in); return; }
    if(in.keys.find(char(27))!=std::string::npos||in.keys.find('p')!=std::string::npos) {
        screen=Screen::Paused; menuIndex=0; velocity={}; casts.clear(); return;
    }
    if(in.keys.find('m')!=std::string::npos&&!world.interior) { screen=Screen::Map; mapCenter=hero.pos; returnScreen=Screen::Playing; velocity={}; casts.clear(); return; }
    if(!inventoryMenu&&!classMenu&&!helpMenu) {
        if(in.keys.find('u')!=std::string::npos) { screen=Screen::Progress; menuIndex=0; velocity={}; casts.clear(); dialogue.clear(); return; }
        if(in.keys.find('y')!=std::string::npos) { talkNearby(); return; }
        if(in.keys.find('j')!=std::string::npos||in.keys.find('l')!=std::string::npos) { screen=in.keys.find('l')!=std::string::npos?Screen::Lore:Screen::Journal; menuIndex=0; velocity={}; casts.clear(); dialogue.clear(); return; }
    }
    if(camera.perspective) {
        if(in.keys.find('[')!=std::string::npos) camera.fov=std::max(35*pi/180,camera.fov-3*pi/180);
        if(in.keys.find(']')!=std::string::npos) camera.fov=std::min(80*pi/180,camera.fov+3*pi/180);
        camera.yaw=std::remainder(camera.yaw+in.orbitX*(camera.firstPerson?.004f:.006f),2*pi);
        if(camera.firstPerson) camera.lookPitch=clamp(camera.lookPitch+in.orbitY*.003f,-80*pi/180,80*pi/180);
        else camera.tilt=clamp(camera.tilt+in.orbitY*.004f,45*pi/180,65*pi/180); }
    if(in.heldMovement) { velocity={}; movementTime=0; }
    Vec2 direction{};
    for(char k:in.keys+in.movementKeys) {
        if(k=='q') quit=true;
        if(k=='b'&&hero.alive&&!world.interior&&!escortActive()) { setScene("boss"); message="The Dragon's hollow. Dodge red telegraphs."; }
        if(k=='i') { inventoryMenu=!inventoryMenu; continue; }
        if(k=='r') respawn();
        if(k=='e') collectLoot();
        if(k=='g') gather();
        if(k=='f') { interactPlace(); continue; }
        if(inventoryMenu) { if(k=='j'||k=='v') ++inventoryIndex; if(k=='k'||k=='^') --inventoryIndex; inventoryIndex=clamp(inventoryIndex,0,std::max(0,int(hero.inventory.size())-1)); if(k=='\r'||k=='\n') equipSelected(); continue; }
        if(k=='c') classMenu=!classMenu;
        if(k=='h') helpMenu=!helpMenu;
        if(k>='1'&&k<='6') {
            if(classMenu&&k<='5') {
                if(!hero.testing&&(hero.hurtTime>0||!hero.alive||fight.active)) { message="Change class while alive and out of combat"; continue; }
                applyPlayerClass(Role(k-'1'));
                for(int item:hero.equipment) if(item>=0) { hero.armor+=data.items[size_t(item)].armor; hero.power+=data.items[size_t(item)].power; }
                classMenu=false; message=std::string("Class: ")+roleName(hero.role);
            }
            else if(!classMenu) casts.push_back(k-'1');
        }
        if(k==' '&&hero.alive&&jumpHeight<=0&&jumpSpeed<=0) { jumpSpeed=7.f; soundEvents.push_back(0); }
        if(k=='\t') {
            auto es=entities(); std::vector<int> ids;
            for(auto* e:es) if(e->id!=hero.id&&(e->alive||e->kind==Kind::Party)&&distance(e->pos,hero.pos)<35) ids.push_back(e->id);
            auto current=std::find(ids.begin(),ids.end(),hero.target);
            if(!ids.empty()) hero.target=(current==ids.end()||++current==ids.end())?ids.front():*current;
            for(auto* e:es) if(e->id==hero.target) aim=e->pos;
        }
        if(k=='^') aim.y-=2;
        if(k=='v') aim.y+=2;
        if(k=='<') aim.x-=2;
        if(k=='>') aim.x+=2;

        if(k=='='||k=='+') camera.zoom=clamp(camera.zoom+.1f,.55f,2.f);
        if(k=='-') camera.zoom=clamp(camera.zoom-.1f,.55f,2.f);
    }
    const auto& movement=in.heldMovement?in.movementKeys:in.keys;
    if(!inventoryMenu&&!classMenu&&!helpMenu) {
        direction.x=float(movement.find('d')!=std::string::npos)-float(movement.find('a')!=std::string::npos);
        direction.y=float(movement.find('s')!=std::string::npos)-float(movement.find('w')!=std::string::npos);
    }
    if(camera.perspective) direction={direction.x*std::cos(camera.yaw)-direction.y*std::sin(camera.yaw),-direction.x*std::sin(camera.yaw)-direction.y*std::cos(camera.yaw)};
    if(length(direction)>0) { velocity=normalized(direction)*hero.speed*(in.sprint?1.8f:1.f); movementTime=.16f; }
    if(in.scroll) camera.zoom=clamp(camera.zoom+in.scroll*.1f,.55f,2.f);
    if(in.mouseX>=0) { Vec2 hit; Vec2 pixel=camera.perspective&&camera.firstPerson?Vec2{camera.width*.5f,camera.height*.5f}:Vec2{float(in.mouseX),float(in.mouseY)};
        if(camera.pick(pixel,hit)) aim=hit;
        else if(camera.perspective&&camera.firstPerson) { auto view=camera.view(); aim=hero.pos+normalized(Vec2{view.forward[0],view.forward[1]})*30; } }
    if(in.click) casts.push_back(0);
}
void Game::tick(float dt) {
    if(message!=lastEvent) { eventLog.push_front(message); if(eventLog.size()>5) eventLog.pop_back(); lastEvent=message; }
    if(screen!=Screen::Playing) return;
    saveTimer+=dt;
    if(activeSave>=0&&saveTimer>=30) { saveTimer=0; if(!saveAdventure()) message=menuNotice; }
    transition=std::max(0.f,transition-dt);
    world.focus(hero.pos);
    if(!world.interior) {
    for(const auto& p:atlas.discover(hero.pos)) { awardXP(35); message="Discovered "+p.name+": "+atlas.types[size_t(p.type)].purpose; }
    spawnTimer-=dt;
    if(spawnTimer<=0||World::chunkAt(hero.pos)!=enemyCenter) { streamEnemies(); spawnTimer=1; }
    for(size_t i=0;i<landmarks.size();++i) if(!discovered[i]&&distance(hero.pos,landmarks[i].pos)<16) {
        discovered[i]=true; awardXP(150); message=std::string("Discovered ")+landmarks[i].name+" +150 XP";
    }
    }
    time+=dt;
    if(!world.interior) { population.tick(atlas,world,time,dt,weatherAt(world.terrainSeed,time)!=0); environment.tick(world,hero.pos,mobs,time,dt); }
    movementTime-=dt; if(movementTime<=0) velocity={};
    Vec2 previousPosition=hero.pos;
    if(hero.alive) {
        hero.pos=world.move(hero.pos,velocity*dt*(world.water(hero.pos)?.55f:1.f)*(hero.slow>0?.35f:1.f));
    }
    if(jumpHeight>0||jumpSpeed>0) {
        jumpHeight-=world.elevation(hero.pos)-world.elevation(previousPosition);
        jumpHeight+=jumpSpeed*dt-9.f*dt*dt; jumpSpeed-=18.f*dt;
        if(jumpHeight<=0) { jumpHeight=jumpSpeed=0; soundEvents.push_back(1); }
    }
    camera.jumpHeight=jumpHeight;
    stepTimer-=dt;
    if(jumpHeight==0&&distance(previousPosition,hero.pos)>.001f&&stepTimer<=0) {
        soundEvents.push_back(2); stepTimer=length(velocity)>hero.speed*1.1f?.23f:.38f;
    }
    if(soundEvents.size()>32) soundEvents.erase(soundEvents.begin(),soundEvents.end()-32);
    hero.velocity=velocity;
    if(!world.interior) tickDragon(*this,dt);
    tickInterior(dt); tickSociety(dt); tickQuests(dt);
    auto es=entities(); combat.tick(dt,es,world,texts);
    for(int slot:casts) { auto error=combat.cast(hero,slot,aim,hero.target,es,data,world,texts); if(!error.empty()) message=error; else { mastery[size_t(hero.role)]=std::min(100000,mastery[size_t(hero.role)]+1); soundEvents.push_back(3); } }
    casts.clear();
    if(companions) tickParty(*this,dt); tickMobs(*this,dt); rewards();
    world.tick(dt); world.pushGrass(hero.pos,2.8f,dt*30);
    for(const auto& e:party) world.pushGrass(e.pos,2.5f,dt*25);
    for(auto& t:texts) t.life-=dt;
    texts.erase(std::remove_if(texts.begin(),texts.end(),[](const auto& t){return t.life<=0;}),texts.end());
    camera.center=camera.perspective&&camera.firstPerson?hero.pos:lerp(camera.center,hero.pos,1-std::exp(-dt*6));
}
void Game::render(Framebuffer& f,float ms,Framebuffer* interface) {
    if(camera.perspective&&camera.firstPerson) camera.center=hero.pos;
    camera.terrain=&world; // Terrain projection also keeps objects, labels and mouse aim grounded.
    if(f.width<40||f.height<16) { f.clear(); f.text(0,0,"Resize to at least 40x16"); f.text(0,2,"Q: quit"); return; }
    camera.width=f.width; camera.height=f.height;
    if(frontEnd()) {
        f.clear();
        for(int x=0;x<f.width;++x) {
            int ridge=int(f.height*.78+std::sin(x*.07)*3+std::sin(x*.19)*2);
            f.put(x,ridge,U'^',{40,64,77});
            if(x%7==0) f.put(x,int(noise(x,0,42)*f.height*.65f),U'.',{47,67,86});
            for(int y=ridge+1;y<f.height;++y) if((x+y)%4==0) f.put(x,y,U'/',{21,34,46});
        }
        if(interface) { for(auto& cell:interface->cells) cell={U'\0',white,background}; renderUI(*interface,ms); }
        else renderUI(f,ms);
        return;
    }
    scene.begin(camera); if(!camera.perspective) world.render(scene,time); if(!world.interior) { atlas.focus(hero.pos); atlas.render(scene,time); population.render(scene,time); environment.render(scene,time); renderEscorts(scene); }
    for(const auto& e:party) if(e.alive) drawCharacter(scene,e.pos,e.color,time,length(e.velocity)>0,int(e.role));
    for(const auto& e:mobs) if(e.alive) {
        if(e.name=="Wolf"&&camera.perspective) drawWolf(scene,e.pos,time);
        else if(e.name=="Wolf") { scene.point(e.pos,1,{172,168,166},U'w'); scene.point(e.pos+Vec2{1,0},1.5f,{220,179,128},U'>'); }
        else if(e.archetype==0) drawSkeleton(scene,e.pos,mix(e.color,{237,128,104},.55f),time); else drawCharacter(scene,e.pos,mix(e.color,{237,128,104},.55f),time,length(e.velocity)>0,int(e.role));
    }
    for(const auto& item:loot) scene.point(item.pos,.3f,{192,171,109},U'*',{0,0,1},.2f);
    if(dragon.alive&&!world.interior) drawDragon(scene,dragon.pos,time);
    combat.render(scene,time);
    if(!camera.perspective) world.reflections(scene,time); scene.convert(f);
    if(!world.interior&&!camera.perspective) environment.atmosphere(f,camera,world.terrainSeed,time);
    if(hero.alive&&!(camera.perspective&&camera.firstPerson)) playerAvatar(f,camera,hero,time);
    if(showNames) { if(!(camera.perspective&&camera.firstPerson)) nameLabel(f,camera,hero); for(const auto& e:party) nameLabel(f,camera,e,e.id==hero.target);
    for(const auto& e:mobs) nameLabel(f,camera,e,e.id==hero.target);
    for(const auto& item:loot) { Vec2 p=camera.project(item.pos); f.drawDepth=camera.deviceDepth(item.pos,.3f); if(p.y>2&&p.y<f.height-5) f.text(int(p.x)-4,int(p.y)-1,data.items[size_t(item.item)].name,{151,139,111}); f.drawDepth=0; }
    if(!world.interior) nameLabel(f,camera,dragon,dragon.id==hero.target);
    }
    if(!world.interior) { atlas.labels(f,camera,hero.pos); population.labels(f,camera,hero.pos,aim); }
    if(transition>0) for(auto& cell:f.cells) cell.fg=cell.fg*(1-transition/.25f);
    floatingLabels(f,camera,texts);
    Vec2 cursor=camera.perspective&&camera.firstPerson?Vec2{f.width*.5f,f.height*.5f}:camera.project(aim); f.put(int(cursor.x),int(cursor.y),U'+',white);
    if(interface) {
        // NUL cells are transparent in the native interface layer.
        for(auto& cell:interface->cells) cell={U'\0',white,background};
        if(screen!=Screen::Playing) for(auto& cell:f.cells) cell.fg=cell.fg*.24f;
        renderUI(*interface,ms);
    } else renderUI(f,ms);
}
void Game::renderUI(Framebuffer& f,float ms) {
    uiWidth=f.width; uiHeight=f.height;
    if(frontEnd()) { renderMenu(f); return; }
    hud(f,hero,party,fight.active?&dragon:nullptr,ms);
    minimap(f);
    if(f.width>=100&&f.height>=40&&!party.size()) {
        int y=f.height-9;
        for(size_t i=0;i<std::min(size_t(3),eventLog.size());++i) f.text(2,y+int(i),eventLog[i].substr(0,70),{124,158,163});
    }
    for(int y=f.height-5;y<f.height;++y) for(int x=0;x<f.width;++x) f.put(x,y,U' ');
    int slotWidth=std::max(1,(f.width-4)/6);
    for(int i=0;i<6;++i) {
        const auto& a=data.abilities[size_t(hero.role)][size_t(i)]; std::string label=std::to_string(i+1)+" "+a.name;
        if(hero.cooldown[size_t(i)]>0) label="["+std::to_string(i+1)+"] "+std::to_string(int(std::ceil(hero.cooldown[size_t(i)])))+"s";
        f.text(2+i*slotWidth,f.height-4,label.substr(0,size_t(std::max(1,slotWidth-1))),hero.cooldown[size_t(i)]>0?muted:Color{215,202,161});
        if(f.width>90) f.text(2+i*slotWidth,f.height-3,std::to_string(int(a.cost))+" MP",muted);
    }
    f.text(2,f.height-2,message.substr(0,size_t(std::max(0,f.width-4))),muted);
    if(showHelp) f.text(2,f.height-1,"WASD move  F enter  Y talk  J quests  L lore  U skills  G gather  M map  Esc pause",{165,188,202});
    f.text(2,f.height-5,(interiorSite?atlas.place(*interiorSite).name:std::string(world.region(hero.pos)))+" | Lv "+std::to_string(hero.level)+" XP "+std::to_string(hero.xp)+"/"+std::to_string(100+hero.level*25)+(hero.testing?" | TEST POWER ON":""),{196,190,144});
    if(classMenu||helpMenu) {
        int w=std::min(66,f.width-4),x=(f.width-w)/2,y=std::max(2,f.height/2-7); box(f,x,y,w,14);
        f.text(x+2,y+1,classMenu?"CHOOSE YOUR CLASS":"ASHENMERE / controls",{222,201,148});
        if(classMenu) for(int i=0;i<5;++i) f.text(x+3,y+3+i*2,std::to_string(i+1)+"  "+roleName(Role(i)));
        else { const char* lines[]={"WASD: move | Shift: sprint | Space: jump","Mouse or arrows: aim / click or 1: primary","1-6: ability / Tab: cycle nearby targets","C: class / +/- or wheel: zoom","I: inventory / E: collect loot / R: respawn","B: travel to Dragon arena / H: close help","Q or Ctrl-C: restore terminal and quit"}; for(int i=0;i<7;++i) f.text(x+2,y+3+i,lines[i]); }
    }
    if(inventoryMenu) {
        int w=std::min(64,f.width-2),h=std::min(f.height-2,28),x=(f.width-w)/2,y=1; box(f,x,y,w,h);
        f.text(x+2,y+1,"INVENTORY / "+std::to_string(hero.gold)+"g",{218,199,145});
        f.text(x+2,y+2,"J/K select - Enter equip/unequip - I close",muted);
        f.text(x+2,y+3,"Power "+std::to_string(int(hero.power))+"  Armor "+std::to_string(int(hero.armor))+"  XP "+std::to_string(hero.xp)+"/"+std::to_string(100+hero.level*25));
        const char* slots[]={"Weapon", "Clothing", "Charm"};
        for(int slot=0;slot<3;++slot) {
            int id=hero.equipment[size_t(slot)];
            f.text(x+2,y+5+slot,std::string(slots[slot])+": "+(id<0?"None":data.items[size_t(id)].name),{155,200,212});
        }
        f.text(x+2,y+8,"Weight "+std::to_string(carryWeight())+"/60 | U: mastery/crafting",muted);
        if(hero.inventory.empty()) f.text(x+2,y+9,"Defeat enemies; press E near their loot.",muted);
        int start=std::max(0,inventoryIndex-(h-13));
        for(int i=start;i<int(hero.inventory.size())&&i-start<h-11;++i) {
            const auto& item=data.items[size_t(hero.inventory[size_t(i)])]; bool equipped=hero.equipment[size_t(item.slot)]==item.id;
            f.text(x+2,y+9+i-start,(i==inventoryIndex?"> ":"  ")+item.name+(equipped?" [equipped]":"")+"  +"+std::to_string(int(item.power))+" power / +"+std::to_string(int(item.armor))+" armor",i==inventoryIndex?white:muted);
        }
    }
    if(screen==Screen::Progress) { renderProgress(f); return; }
    if(socialScreen()) { renderSocial(f); return; }
    if(screen!=Screen::Playing) { renderMenu(f); return; }
    if(!hero.alive) { int x=std::max(0,f.width/2-19),y=f.height/2; box(f,x,y,38,5); f.text(x+3,y+1,"You have fallen",{238,110,99}); f.text(x+3,y+3,"R: return to Ashenmere (-25g)"); }
}
}
