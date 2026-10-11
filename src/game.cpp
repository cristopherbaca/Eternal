#include "game.hpp"
#include "ui/hud.hpp"
#include "ai/ai.hpp"
#include "assets/dragon.hpp"
#include <cstdio>
namespace er {
Game::Game(uint32_t seed,const std::filesystem::path& dataPath):data(dataPath),world(seed) {
    data.applyClass(hero,Role::Mage);
    Entity e; e.kind=Kind::Party; e.id=2; e.name="Stasia_apk"; e.role=Role::Cleric; e.pos={-4,14}; e.hp=e.maxHp=244; e.mp=e.maxMp=556; e.level=5; e.color={148,186,137}; party.push_back(e);
    e.id=3; e.name="Jade"; e.role=Role::Warrior; e.pos={5,16}; e.hp=e.maxHp=786; e.mp=e.maxMp=53; e.level=1; e.color={151,151,171}; party.push_back(e);
    e.id=4; e.name="Elowen"; e.role=Role::Mage; e.pos={-6,9}; e.hp=e.maxHp=183; e.mp=e.maxMp=350; e.level=7; e.color={132,137,196}; party.push_back(e);
    dragon.id=500; dragon.kind=Kind::Boss; dragon.name="Dragon"; dragon.pos=dragon.home={30,-30}; dragon.hp=dragon.maxHp=data.boss.hp; dragon.armor=data.boss.armor; dragon.power=data.boss.power; dragon.speed=data.boss.speed;
    mobs.reserve(128);
    for(int i=0;i<22;++i) {
        Vec2 p{noise(i,9,seed)*120-60,noise(i,13,seed)*65-25};
        if(world.blocked(p)||world.water(p+Vec2{0,18})||distance(p,hero.pos)<9) continue;
        spawnMob(p,i%3);
    }
    spawnMob({12,10},0); spawnMob({-13,17},0); spawnMob({18,20},2);
}
void Game::spawnMob(Vec2 pos,int archetype) {
    if(mobs.size()>=100) return;
    const auto& def=data.enemies.at(size_t(archetype)); Entity e; e.id=100+int(mobs.size()); e.name=def.name; e.kind=Kind::Mob; e.archetype=archetype;
    e.pos=e.home=pos; e.maxHp=e.hp=def.hp; e.armor=def.armor; e.power=def.power; e.speed=def.speed; e.color=def.color; e.role=archetype==1?Role::Mage:Role::Warrior; mobs.push_back(e);
}
void Game::collectLoot() {
    for(auto it=loot.begin();it!=loot.end();) {
        if(distance(hero.pos,it->pos)<4&&hero.inventory.size()<24) { hero.inventory.push_back(it->item); hero.gold+=it->gold; message="Collected "+data.items[size_t(it->item)].name+" +"+std::to_string(it->gold)+"g"; it=loot.erase(it); }
        else ++it;
    }
}
void Game::equipSelected() {
    if(hero.inventory.empty()) return;
    inventoryIndex=clamp(inventoryIndex,0,int(hero.inventory.size())-1); int id=hero.inventory[size_t(inventoryIndex)]; const auto& item=data.items[size_t(id)];
    int old=hero.equipment[size_t(item.slot)]; if(old>=0) { hero.armor-=data.items[size_t(old)].armor; hero.power-=data.items[size_t(old)].power; }
    hero.equipment[size_t(item.slot)]=id; hero.armor+=item.armor; hero.power+=item.power; message="Equipped "+item.name;
}
void Game::respawn() {
    if(hero.alive) return;
    hero.alive=true; hero.hp=hero.maxHp; hero.mp=hero.maxMp; hero.pos={0,12}; hero.poison=hero.slow=hero.hurtTime=0; hero.cooldown.fill(0); hero.gold=std::max(0,hero.gold-25); velocity={};
    combat.areas.clear(); combat.projectiles.clear();
    for(size_t i=0;i<party.size();++i) { auto& e=party[i]; e.alive=true; e.hp=e.maxHp; e.mp=e.maxMp; e.pos=hero.pos+Vec2{float(i)*4-4,3}; }
    message="Returned to Ashenmere (-25g).";
}
void Game::rewards() {
    for(auto& e:mobs) {
        if(!e.alive&&!e.rewarded) { const auto& def=data.enemies[size_t(e.archetype)]; e.rewarded=true; hero.xp+=def.xp; for(auto& guest:guests) if(distance(guest.pos,e.pos)<35) guest.xp+=def.xp; loot.push_back({e.pos,def.item,def.gold}); message=def.name+" defeated. +"+std::to_string(def.xp)+" XP. E: collect loot"; }
        if(!e.alive&&e.deadTime>45&&!e.summoned) { e.alive=true; e.hp=e.maxHp; e.pos=e.home; e.rewarded=false; e.poison=0; e.brain=Brain::Idle; }
    }
    while(hero.xp>=100+hero.level*25) { hero.xp-=100+hero.level*25; ++hero.level; hero.maxHp+=8; hero.hp=hero.maxHp; hero.power+=2; texts.push_back({hero.pos,"LEVEL UP",{255,223,115},3,3}); }
    for(auto& guest:guests) while(guest.xp>=100+guest.level*25) { guest.xp-=100+guest.level*25; ++guest.level; guest.maxHp+=8; guest.hp=guest.maxHp; guest.power+=2; }
    if(loot.size()>200) loot.erase(loot.begin(),loot.begin()+int(loot.size()-200));
}
std::vector<Entity*> Game::entities() { std::vector<Entity*> es; if(!serverMode) es.push_back(&hero); for(auto& e:guests) es.push_back(&e); for(auto& e:party) es.push_back(&e); for(auto& e:mobs) es.push_back(&e); if(dragon.id==500) es.push_back(&dragon); return es; }
void Game::setScene(const std::string& name) {
    if(name=="water") hero.pos={0,34}; else if(name=="forest") hero.pos={-33,3}; else if(name=="ruins") hero.pos={0,-2}; else if(name=="boss") hero.pos={30,-17};
    camera.center=hero.pos; aim=hero.pos+Vec2{7,-3};
    if(name=="boss") { aim=dragon.pos; hero.target=dragon.id; }
    for(size_t i=0;i<party.size();++i) party[i].pos=hero.pos+Vec2{float(i)*5-5,3};
}
void Game::input(const Input& in) {
    Vec2 direction{};
    for(char k:in.keys) {
        if(k=='q') quit=true;
        if(k=='b'&&hero.alive) { setScene("boss"); message="The Dragon's hollow. Dodge red telegraphs."; }
        if(k=='i') { inventoryMenu=!inventoryMenu; continue; }
        if(k=='r') respawn();
        if(k=='e') collectLoot();
        if(inventoryMenu) { if(k=='j'||k=='v') ++inventoryIndex; if(k=='k'||k=='^') --inventoryIndex; inventoryIndex=clamp(inventoryIndex,0,std::max(0,int(hero.inventory.size())-1)); if(k=='\r'||k=='\n') equipSelected(); continue; }
        if(k=='c') classMenu=!classMenu;
        if(k=='h') helpMenu=!helpMenu;
        if(k>='1'&&k<='6') {
            if(classMenu&&k<='5') {
                if(hero.hurtTime>0||!hero.alive||fight.active) { message="Change class while alive and out of combat"; continue; }
                data.applyClass(hero,Role(k-'1'));
                for(int item:hero.equipment) if(item>=0) { hero.armor+=data.items[size_t(item)].armor; hero.power+=data.items[size_t(item)].power; }
                classMenu=false; message=std::string("Class: ")+roleName(hero.role);
            }
            else if(!classMenu) casts.push_back(k-'1');
        }
        if(k==' ') casts.push_back(0);
        if(k=='\t') {
            auto es=entities(); std::vector<int> ids;
            for(auto* e:es) if(e->id!=hero.id&&(e->alive||e->kind==Kind::Party)&&distance(e->pos,hero.pos)<35) ids.push_back(e->id);
            auto current=std::find(ids.begin(),ids.end(),hero.target);
            if(!ids.empty()) hero.target=(current==ids.end()||++current==ids.end())?ids.front():*current;
            for(auto* e:es) if(e->id==hero.target) aim=e->pos;
        }
        if(k=='^') aim.y-=2; if(k=='v') aim.y+=2; if(k=='<') aim.x-=2; if(k=='>') aim.x+=2;
        if(k=='w') direction.y-=1; if(k=='s') direction.y+=1;
        if(k=='a') direction.x-=1; if(k=='d') direction.x+=1;
        if(k=='='||k=='+') camera.zoom=clamp(camera.zoom+.1f,.55f,2.f); if(k=='-') camera.zoom=clamp(camera.zoom-.1f,.55f,2.f);
    }
    if(length(direction)>0) { velocity=normalized(direction)*hero.speed; movementTime=.16f; }
    if(in.scroll) camera.zoom=clamp(camera.zoom+in.scroll*.1f,.55f,2.f);
    if(in.mouseX>=0) aim=camera.unproject({float(in.mouseX),float(in.mouseY)});
    if(in.click) casts.push_back(0);
}
void Game::tick(float dt) {
    time+=dt; movementTime-=dt; if(movementTime<=0) velocity={};
    if(hero.alive) hero.pos=world.move(hero.pos,velocity*dt*(world.water(hero.pos)?.55f:1.f)*(hero.slow>0?.35f:1.f)); hero.velocity=velocity;
    tickDragon(*this,dt);
    auto es=entities(); combat.tick(dt,es,world,texts);
    for(int slot:casts) { auto error=combat.cast(hero,slot,aim,hero.target,es,data,world,texts); if(!error.empty()) message=error; }
    casts.clear();
    tickParty(*this,dt); tickMobs(*this,dt); rewards();
    world.tick(dt); world.pushGrass(hero.pos,2.8f,dt*30);
    for(const auto& e:party) world.pushGrass(e.pos,2.5f,dt*25);
    for(auto& t:texts) t.life-=dt;
    texts.erase(std::remove_if(texts.begin(),texts.end(),[](const auto& t){return t.life<=0;}),texts.end());
    camera.center=lerp(camera.center,hero.pos,1-std::exp(-dt*6));
}
void Game::render(Framebuffer& f,float ms) {
    if(f.width<40||f.height<16) { f.clear(); f.text(0,0,"Resize to at least 40x16"); f.text(0,2,"Q: quit"); return; }
    camera.width=f.width; camera.height=f.height; scene.begin(camera); world.render(scene,time);
    if(hero.alive) drawCharacter(scene,hero.pos,hero.color,time,length(velocity)>0,int(hero.role));
    for(const auto& e:party) if(e.alive) drawCharacter(scene,e.pos,e.color,time,length(e.velocity)>0,int(e.role));
    for(const auto& e:mobs) if(e.alive) {
        if(e.archetype==0) drawSkeleton(scene,e.pos,e.color,time); else drawCharacter(scene,e.pos,e.color,time,length(e.velocity)>0,int(e.role));
    }
    for(const auto& item:loot) scene.point(item.pos,.3f,{192,171,109},U'*',{0,0,1},.2f);
    for(const auto& e:guests) if(e.alive) drawCharacter(scene,e.pos,e.color,time,length(e.velocity)>0,int(e.role));
    if(dragon.alive) drawDragon(scene,dragon.pos,time);
    combat.render(scene,time);
    world.reflections(scene,time); scene.convert(f);
    nameLabel(f,camera,hero); for(const auto& e:party) nameLabel(f,camera,e,e.id==hero.target);
    for(const auto& e:mobs) nameLabel(f,camera,e,e.id==hero.target);
    for(const auto& item:loot) { Vec2 p=camera.project(item.pos); if(p.y>2&&p.y<f.height-5) f.text(int(p.x)-4,int(p.y)-1,data.items[size_t(item.item)].name,{151,139,111}); }
    for(const auto& e:guests) nameLabel(f,camera,e,e.id==hero.target);
    nameLabel(f,camera,dragon,dragon.id==hero.target);
    floatingLabels(f,camera,texts); auto panels=party; for(const auto& e:guests) if(e.group==hero.group) panels.insert(panels.begin(),e); hud(f,hero,panels,fight.active?&dragon:nullptr,ms);
    Vec2 cursor=camera.project(aim); f.put(int(cursor.x),int(cursor.y),U'+',white);
    for(int y=f.height-4;y<f.height;++y) for(int x=0;x<f.width;++x) f.put(x,y,U' ');
    int slotWidth=std::max(1,(f.width-4)/6);
    for(int i=0;i<6;++i) {
        const auto& a=data.abilities[size_t(hero.role)][size_t(i)]; std::string label=std::to_string(i+1)+" "+a.name;
        if(hero.cooldown[size_t(i)]>0) label="["+std::to_string(i+1)+"] "+std::to_string(int(std::ceil(hero.cooldown[size_t(i)])))+"s";
        f.text(2+i*slotWidth,f.height-4,label.substr(0,size_t(std::max(1,slotWidth-1))),hero.cooldown[size_t(i)]>0?muted:Color{215,202,161});
        if(f.width>90) f.text(2+i*slotWidth,f.height-3,std::to_string(int(a.cost))+" MP",muted);
    }
    f.text(2,f.height-2,message.substr(0,size_t(std::max(0,f.width-4))),muted);
    f.text(2,f.height-1,"WASD move  Mouse/arrows aim  1-6 cast  Tab target  C class  H help  Q quit",{82,104,117});
    if(!chatLog.empty()) f.text(2,f.height-5,chatLog.back().substr(0,size_t(std::max(0,f.width-4))),{163,191,200});
    if(chatEditing) f.text(2,f.height-2,"> "+chatDraft+"_",white);
    if(classMenu||helpMenu) {
        int w=std::min(66,f.width-4),x=(f.width-w)/2,y=std::max(2,f.height/2-7); box(f,x,y,w,14);
        f.text(x+2,y+1,classMenu?"CHOOSE YOUR CLASS":"ASHENMERE / controls",{222,201,148});
        if(classMenu) for(int i=0;i<5;++i) f.text(x+3,y+3+i*2,std::to_string(i+1)+"  "+roleName(Role(i)));
        else { const char* lines[]={"WASD: move / hold to repeat","Mouse or arrows: aim / click or Space: primary","1-6: ability / Tab: cycle nearby targets","C: class / +/- or wheel: zoom","I: inventory / E: collect loot / R: respawn","B: travel to Dragon arena / H: close help","Q or Ctrl-C: restore terminal and quit"}; for(int i=0;i<7;++i) f.text(x+2,y+3+i,lines[i]); }
    }
    if(inventoryMenu) {
        int w=std::min(64,f.width-2),h=std::min(f.height-2,28),x=(f.width-w)/2,y=1; box(f,x,y,w,h);
        f.text(x+2,y+1,"INVENTORY / "+std::to_string(hero.gold)+"g",{218,199,145});
        f.text(x+2,y+2,"J/K select - Enter equip - I close",muted);
        f.text(x+2,y+3,"Power "+std::to_string(int(hero.power))+"  Armor "+std::to_string(int(hero.armor))+"  XP "+std::to_string(hero.xp)+"/"+std::to_string(100+hero.level*25));
        if(hero.inventory.empty()) f.text(x+2,y+5,"Defeat enemies; press E near their loot.",muted);
        int start=std::max(0,inventoryIndex-(h-9));
        for(int i=start;i<int(hero.inventory.size())&&i-start<h-7;++i) {
            const auto& item=data.items[size_t(hero.inventory[size_t(i)])]; bool equipped=hero.equipment[size_t(item.slot)]==item.id;
            f.text(x+2,y+5+i-start,(i==inventoryIndex?"> ":"  ")+item.name+(equipped?" [equipped]":"")+"  +"+std::to_string(int(item.power))+" power / +"+std::to_string(int(item.armor))+" armor",i==inventoryIndex?white:muted);
        }
    }
    if(!hero.alive) { int x=std::max(0,f.width/2-19),y=f.height/2; box(f,x,y,38,5); f.text(x+3,y+1,"You have fallen",{238,110,99}); f.text(x+3,y+3,"R: return to Ashenmere (-25g)"); }
}
}
