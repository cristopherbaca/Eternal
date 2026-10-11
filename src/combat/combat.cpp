#include "combat/combat.hpp"
namespace er {
bool hostile(const Entity& a,const Entity& b) { auto enemy=[](const Entity& e){return e.kind==Kind::Mob||e.kind==Kind::Boss;}; return enemy(a)!=enemy(b); }
float damageAfterArmor(float p,float armor) { return std::max(0.f,p)*100/(100+std::max(0.f,armor)); }
float dealDamage(Entity& e,float p,std::vector<FloatingText>& texts) {
    if(!e.alive) return 0;
    float dealt=std::min(e.hp,damageAfterArmor(p,e.armor)*(e.shield>0?.5f:1.f)); e.hp-=dealt; e.hurtTime=5;
    if(dealt>=1&&texts.size()<200) texts.push_back({e.pos,std::to_string(int(std::round(dealt))),e.kind==Kind::Player?Color{242,91,80}:Color{240,224,205}});
    if(e.hp<=0) { e.hp=0; e.alive=false; e.brain=Brain::Dead; e.deadTime=0; } return dealt;
}
float heal(Entity& e,float amount,std::vector<FloatingText>& texts) {
    if(!e.alive) return 0;
    float restored=clamp(amount,0.f,e.maxHp-e.hp); e.hp+=restored;
    if(restored>=1&&texts.size()<200) texts.push_back({e.pos,"+"+std::to_string(int(std::round(restored))),{114,241,129}}); return restored;
}
void Combat::burst(Vec2 pos,Color color,int count,float radius) {
    for(int i=0;i<count&&particles.size()<2200;++i) { float a=float(i)*2*pi/std::max(count,1),n=noise(int(serial++),i); float life=.35f+n*.7f; particles.push_back({pos,{std::cos(a)*radius,std::sin(a)*radius},.5f+n*2,life,life,color,U"o+.*"[i%4]}); }
}
std::string Combat::cast(Entity& c,int slot,Vec2 aim,int target,const std::vector<Entity*>& es,const Data& data,World& world,std::vector<FloatingText>& texts) {
    if(slot<0||slot>=6||!c.alive) return "Cannot cast while fallen";
    const auto& a=data.abilities[size_t(c.role)][size_t(slot)];
    if(c.cooldown[size_t(slot)]>0) return a.name+" is cooling down";
    if(c.mp<a.cost) return "Not enough MP";
    Entity* selected=nullptr; for(auto* e:es) if(e->id==target) selected=e;
    Vec2 dir=normalized(aim-c.pos); if(length(dir)<.1f) dir=c.facing;
    aim=c.pos+dir*std::min(distance(c.pos,aim),a.range); c.facing=dir;
    const auto& effect=a.effect;
    if(effect=="resurrect"&&(!selected||selected->alive||hostile(c,*selected)||distance(c.pos,selected->pos)>a.range)) return "Target a fallen ally with Tab";
    if(effect=="heal"&&selected&&!hostile(c,*selected)&&(!selected->alive||distance(c.pos,selected->pos)>a.range)) return "Ally is fallen or out of range";
    c.mp-=a.cost; c.cooldown[size_t(slot)]=a.cooldown;
    float power=a.power*(.85f+c.power/200);
    if(effect=="heal") { Entity* e=selected&&!hostile(c,*selected)?selected:&c; heal(*e,a.power,texts); burst(e->pos,{103,232,139},16,2); }
    else if(effect=="resurrect") { selected->alive=true; selected->hp=selected->maxHp*.5f; selected->mp=selected->maxMp*.3f; selected->brain=Brain::Idle; burst(selected->pos,{155,255,183},40,5); }
    else if(effect=="mana") { c.mp=std::min(c.maxMp,c.mp+a.power); burst(c.pos,{112,160,255},22,3); }
    else if(effect=="shield") { c.shield=a.duration; burst(c.pos,{148,190,244},20,3); }
    else if(effect=="heal_group"||effect=="group_buff") {
        for(auto* e:es) if(!hostile(c,*e)&&distance(c.pos,e->pos)<a.radius) { heal(*e,a.power,texts); if(effect=="group_buff") e->shield=a.duration; } burst(c.pos,{129,231,146},40,a.radius);
    } else if(effect=="taunt") { for(auto* e:es) if(hostile(c,*e)&&distance(c.pos,e->pos)<a.radius) { e->target=c.id; e->taunt=a.duration; } burst(c.pos,{248,174,92},32,a.radius); }
    else if(effect=="dash"||effect=="charge") {
        for(int i=0;i<20;++i) c.pos=world.move(c.pos,dir*(a.range/20));
        c.shield=.4f; burst(c.pos,{155,161,216},18,3);
        if(effect=="charge") for(auto* e:es) if(hostile(c,*e)&&distance(c.pos,e->pos)<a.radius) dealDamage(*e,power,texts);
    } else if(effect=="melee"||effect=="bash"||effect=="backstab"||effect=="poison"||effect=="frost") {
        for(auto* e:es) if(hostile(c,*e)&&distance(c.pos,e->pos)<a.radius&&(effect=="frost"||dot(normalized(e->pos-c.pos),dir)>-.2f)) {
            float hit=power; if(effect=="backstab") hit*=dot(e->facing,normalized(c.pos-e->pos))<0?1.7f:.7f;
            dealDamage(*e,hit,texts); if(effect=="bash"||effect=="frost") e->slow=a.duration; if(effect=="poison") e->poison=a.duration;
        }
        burst(c.pos,effect=="frost"?Color{153,217,253}:Color{237,183,141},36,a.radius);
    } else if(effect=="fire"||effect=="heal_area"||effect=="trap"||effect=="volley") {
        areas.push_back({aim,a.radius,a.duration,effect=="trap"?.5f:0.f,.6f,0,power,c.id,effect});
    } else {
        int count=effect=="multishot"?5:effect=="knives"?8:1;
        for(int i=0;i<count;++i) { float angle=std::atan2(dir.y,dir.x)+(effect=="knives"?float(i)*2*pi/count:(i-(count-1)*.5f)*.16f);
            Color col=(effect=="fire_orb")?Color{255,172,63}:effect=="holy"?Color{242,234,167}:effect=="bolt"?Color{135,166,255}:Color{202,216,184};
            projectiles.push_back({c.pos,{std::cos(angle)*a.speed,std::sin(angle)*a.speed},c.id,power,a.radius,std::min(a.duration,a.range/std::max(1.f,a.speed)),effect,col});
        }
    } return {};
}
void Combat::tick(float dt,const std::vector<Entity*>& es,World& world,std::vector<FloatingText>& texts) {
    auto ownerOf=[&](int id)->Entity* { for(auto* e:es) if(e->id==id) return e; return nullptr; };
    for(auto* e:es) {
        for(auto& cd:e->cooldown) cd=std::max(0.f,cd-dt);
        e->slow=std::max(0.f,e->slow-dt); e->shield=std::max(0.f,e->shield-dt); e->taunt=std::max(0.f,e->taunt-dt); e->hurtTime=std::max(0.f,e->hurtTime-dt);
        if(!e->alive) { e->deadTime+=dt; continue; }
        e->mp=std::min(e->maxMp,e->mp+dt*5); if(e->hurtTime<=0) e->hp=std::min(e->maxHp,e->hp+dt*2);
        if(e->poison>0) { e->poison-=dt; e->dotTimer-=dt; if(e->dotTimer<=0) { dealDamage(*e,12,texts); e->dotTimer=1; } }
    }
    for(auto& p:projectiles) {
        Vec2 old=p.pos; p.pos+=p.velocity*dt; p.life-=dt; Entity* owner=ownerOf(p.owner); bool hit=world.blocked(p.pos);
        if(owner) for(auto* e:es) if(e->alive&&hostile(*owner,*e)) {
            Vec2 step=p.pos-old; float t=clamp(dot(e->pos-old,step)/std::max(.0001f,dot(step,step)),0.f,1.f);
            if(distance(e->pos,old+step*t)<(e->kind==Kind::Boss?3.f:1.2f)) {
                dealDamage(*e,p.power,texts); hit=true;
                if(p.effect=="fire_orb") for(auto* other:es) if(other!=e&&hostile(*owner,*other)&&distance(other->pos,p.pos)<p.radius) dealDamage(*other,p.power*.6f,texts);
                break;
            }
        }
        if(hit) { p.life=0; burst(p.pos,p.color,30,p.radius*2); world.pushGrass(p.pos,5,15); }
        else if(p.effect=="fire_orb"||p.effect=="bolt") { particles.push_back({p.pos,p.velocity*-.07f,1.4f,.35f,.35f,p.color,U'.'}); world.pushGrass(p.pos,2,dt*30); }
    }
    projectiles.erase(std::remove_if(projectiles.begin(),projectiles.end(),[](const auto& p){return p.life<=0;}),projectiles.end());
    for(auto& a:areas) {
        if(a.delay>0) { a.delay-=dt; continue; }
        a.life-=dt; a.timer-=dt; Entity* owner=ownerOf(a.owner); if(!owner) continue;
        if(a.timer<=0) {
            a.timer=a.interval;
            for(auto* e:es) if(e->alive&&distance(e->pos,a.pos)<a.radius) {
                if(a.effect=="heal_area"&&!hostile(*owner,*e)) heal(*e,a.power,texts);
                else if(hostile(*owner,*e)) {
                    if(a.effect=="cone"&&dot(normalized(e->pos-a.pos),a.direction)<.65f) continue;
                    dealDamage(*e,a.power,texts); if(a.effect=="trap") { e->slow=4; a.life=0; }
                }
            }
            burst(a.pos,a.effect=="heal_area"?Color{121,243,139}:Color{255,153,52},16,a.radius);
        } world.pushGrass(a.pos,a.radius,dt*14);
    }
    areas.erase(std::remove_if(areas.begin(),areas.end(),[](const auto& a){return a.life<=0;}),areas.end());
    for(auto& p:particles) { p.pos+=p.velocity*dt; p.velocity=p.velocity*(1-dt*1.5f); p.height+=dt*.5f; p.life-=dt; }
    particles.erase(std::remove_if(particles.begin(),particles.end(),[](const auto& p){return p.life<=0;}),particles.end());
    if(particles.size()>2200) particles.resize(2200);
}
void Combat::render(Scene& s,float time) const {
    for(const auto& a:areas) {
        Color col=a.delay>0?Color{232,59,57}:a.effect=="heal_area"?Color{113,221,132}:Color{251,146,49};
        if(a.delay<=0&&s.lights.size()<32) s.lights.push_back({a.pos,.5f,col,a.radius*2,1.3f});
        int points=int(a.radius*30);
        for(int i=0;i<points;++i) { float angle=i*2*pi/points; Vec2 d{std::cos(angle),std::sin(angle)};
            if(a.effect=="cone"&&dot(d,a.direction)<.65f) continue;
            s.point(a.pos+d*a.radius,.15f,col,a.delay>0?U'x':U'o',{0,0,1},.8f);
            if(a.delay<=0&&i%3==0) s.point(a.pos+d*(a.radius*noise(i,int(time*8))),.3f,col,U".:+*"[i%4],{0,0,1},1);
        }
    }
    for(const auto& p:particles) s.point(p.pos,p.height,p.color*(p.life/p.maxLife),p.glyph,{0,0,1},.7f);
    for(const auto& p:projectiles) {
        bool fire=p.effect=="fire_orb"; if(s.lights.size()<32) s.lights.push_back({p.pos,2,p.color,fire?10.f:4.f,fire?2.2f:1.f});
        s.point(p.pos,1.5f,p.color,fire?U'O':U'-',{0,0,1},1);
        if(fire) { s.point(p.pos+Vec2{-.5f,0},1.5f,{255,222,141},U'(',{0,0,1},1); s.point(p.pos+Vec2{.5f,0},1.5f,{255,222,141},U')',{0,0,1},1); }
        else for(int i=1;i<4;++i) s.point(p.pos-normalized(p.velocity)*(i*.5f),1.5f,p.color*(1-i*.18f),U'-');
    }
}
}
