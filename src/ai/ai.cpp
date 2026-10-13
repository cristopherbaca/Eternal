#include "ai/ai.hpp"
#include "game.hpp"
namespace er {
void tickMobs(Game& g,float dt) {
    if(dt<=0) return;
    if(g.combatBrains.size()!=g.data.enemies.size()) {
        g.combatBrains.clear();
        for(size_t i=0;i<g.data.enemies.size();++i)
            g.combatBrains.emplace_back(hash(uint32_t(g.world.seed)^uint32_t(g.world.seed>>32)^uint32_t(i+1)));
    }
    auto es=g.entities();
    for(auto& mob:g.mobs) {
        if(mob.archetype<0||size_t(mob.archetype)>=g.combatBrains.size()) continue;
        auto& brain=g.combatBrains[size_t(mob.archetype)]; auto& memory=mob.experience;
        auto finish=[&](float progress,float terminal) {
            if(memory.action>=0&&memory.learning) {
                float hurt=std::max(0.f,memory.hp-mob.hp)/std::max(1.f,mob.maxHp);
                brain.learn(memory.observation,memory.action,memory.reward-3*hurt+progress+terminal);
            }
            memory={};
        };
        if(!mob.alive) { finish(0,-.8f); continue; }
        if(distance(mob.pos,g.hero.pos)>55) { memory={}; mob.velocity={}; continue; }
        const auto& def=g.data.enemies[size_t(mob.archetype)]; Entity* target=nullptr; float best=def.aggro;
        if(mob.taunt>0) for(auto* e:es) if(e->id==mob.target&&e->alive&&hostile(mob,*e)) target=e;
        if(!target) for(auto* e:es) if(e->alive&&hostile(mob,*e)) { float d=distance(mob.pos,e->pos); if(d<best) { target=e; best=d; } }
        mob.velocity={}; mob.attackTimer=std::max(0.f,mob.attackTimer-dt);
        if(!target) {
            finish(0,0); mob.brain=Brain::Idle;
            if(distance(mob.pos,mob.home)>1) mob.velocity=normalized(mob.home-mob.pos)*mob.speed*.5f;
        } else {
            mob.target=target->id; mob.facing=normalized(target->pos-mob.pos); float d=distance(mob.pos,target->pos);
            memory.timer-=dt;
            // Stop stale actions immediately when the target or attack range changes.
            bool invalid=memory.target!=target->id||(memory.action==1&&d>def.range)||(memory.action==4&&mob.taunt>0);
            if(memory.action<0||memory.timer<=0||invalid) {
                float progress=memory.target==target->id ? clamp((memory.distance-d)/std::max(1.f,mob.speed)*.25f,-.15f,.15f):0;
                finish(progress,0);
                float danger=0;
                for(const auto& area:g.combat.areas) {
                    bool hostileArea=false;
                    for(auto* owner:es) if(owner->id==area.owner) { hostileArea=hostile(mob,*owner); break; }
                    if(hostileArea&&distance(mob.pos,area.pos)<area.radius+1) danger=1;
                }
                float health=clamp(mob.hp/std::max(1.f,mob.maxHp),0.f,1.f);
                memory.observation={health,clamp(target->hp/std::max(1.f,target->maxHp),0.f,1.f),
                    clamp(d/std::max(1.f,def.aggro),0.f,1.f),clamp(mob.attackTimer/1.3f,0.f,1.f),
                    d<=def.range?1.f:0.f,danger,clamp(length(target->velocity)/12.f,0.f,1.f)};
                // Modest starting preferences preserve useful behavior before experience.
                CombatNetwork::Scores prior{.2f,.45f,.08f,.08f,health<.3f?.18f:-.15f};
                if(danger) { prior[2]+=.15f; prior[3]+=.15f; prior[4]+=.15f; }
                std::array<bool,5> allowed{d>def.range*.8f,d<=def.range&&mob.attackTimer<=0,true,true,mob.taunt<=0};
                memory.action=brain.choose(memory.observation,prior,allowed);
                memory.target=target->id; memory.timer=.3f; memory.hp=mob.hp; memory.distance=d;
                memory.learning=!target->testing; // God-mode practice must not teach that hits never work.
            }
            int action=memory.action;
            mob.brain=d>def.range?Brain::Chase:Brain::Attack;
            if(action==0) mob.velocity=mob.facing*mob.speed;
            else if(action==2||action==3) {
                Vec2 side{-mob.facing.y,mob.facing.x};
                // Circle while closing to effective range, never teleport or bypass terrain.
                mob.velocity=normalized(side*(action==2?1.f:-1.f)+mob.facing*(d>def.range?1.f:0.f))*mob.speed*.8f;
            } else if(action==4) mob.velocity=mob.facing*(-mob.speed*.7f);
            else if(action==1&&d<=def.range&&mob.attackTimer<=0) {
                float dealt=dealDamage(*target,mob.power,g.texts);
                memory.reward+=.8f*clamp(dealt/std::max(1.f,mob.power),0.f,1.f);
                if(!target->alive) memory.reward+=.3f;
                mob.attackTimer=1.3f; g.combat.burst(target->pos,mob.color,5,1);
            }
        }
        mob.pos=g.world.move(mob.pos,mob.velocity*dt*(mob.slow>0?.2f:1.f));
        g.world.pushGrass(mob.pos,2,dt*15);
    }
}
}
