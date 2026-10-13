#include "ai/ai.hpp"
#include "game.hpp"
namespace er
{
    void tickParty(Game &g, float dt)
    {
        auto es = g.entities();
        for (size_t i = 0; i < g.party.size(); ++i)
        {
            auto &e = g.party[i];
            if (!e.alive)
                continue;
            Entity *target = nullptr;
            float best = 22;
            for (auto *foe : es)
                if (foe->alive && hostile(e, *foe) && (distance(foe->pos, g.hero.pos) < 15 || foe->brain == Brain::Attack || g.hero.target == foe->id))
                {
                    float d = distance(e.pos, foe->pos);
                    if (d < best)
                    {
                        best = d;
                        target = foe;
                    }
                }
            if (g.fight.active && g.dragon.alive)
                target = &g.dragon;
            Vec2 goal = g.hero.pos + Vec2{float(i) * 5 - 5, 3};
            e.velocity = {};
            bool dodge = false;
            for (const auto &area : g.combat.areas)
                if (area.owner >= 100 && distance(e.pos, area.pos) < area.radius + 1)
                {
                    Vec2 escape = normalized(e.pos - area.pos);
                    if (length(escape) < .1f)
                        escape = {1, 0};
                    if (area.effect == "cone" && dot(escape, area.direction) < .5f)
                        continue;
                    goal = e.pos + escape * 8;
                    dodge = true;
                    break;
                }
            if (e.role == Role::Cleric)
            {
                Entity *patient = &g.hero;
                float ratio = 2;
                int wounded = 0;
                for (auto *ally : es)
                    if (!hostile(e, *ally) && distance(e.pos, ally->pos) < 22)
                    {
                        if (!ally->alive)
                            g.combat.cast(e, 4, ally->pos, ally->id, es, g.data, g.world, g.texts);
                        else
                        {
                            if (ally->hp / ally->maxHp < .75f)
                                ++wounded;
                            if (ally->hp / ally->maxHp < ratio)
                            {
                                patient = ally;
                                ratio = ally->hp / ally->maxHp;
                            }
                        }
                    }
                if (ratio < .8f)
                    g.combat.cast(e, 0, patient->pos, patient->id, es, g.data, g.world, g.texts);
                if (wounded >= 2)
                    g.combat.cast(e, 1, patient->pos, patient->id, es, g.data, g.world, g.texts);
                if (target && ratio > .85f)
                    g.combat.cast(e, 2, target->pos, target->id, es, g.data, g.world, g.texts);
            }
            else if (target)
            {
                float d = distance(e.pos, target->pos);
                if (e.role == Role::Warrior)
                {
                    if (!dodge)
                        goal = target->pos + normalized(e.pos - target->pos) * 1.8f;
                    if (d < 3)
                    {
                        g.combat.cast(e, 0, target->pos, target->id, es, g.data, g.world, g.texts);
                        g.combat.cast(e, 2, target->pos, target->id, es, g.data, g.world, g.texts);
                    }
                    if (d < 11)
                        g.combat.cast(e, 1, target->pos, target->id, es, g.data, g.world, g.texts);
                    if (e.hp < e.maxHp * .5f)
                        g.combat.cast(e, 4, e.pos, e.id, es, g.data, g.world, g.texts);
                }
                else
                {
                    if (!dodge)
                        goal = target->pos + normalized(e.pos - target->pos) * 12;
                    if (d < 24)
                    {
                        g.combat.cast(e, 0, target->pos, target->id, es, g.data, g.world, g.texts);
                        g.combat.cast(e, 1, target->pos, target->id, es, g.data, g.world, g.texts);
                    }
                    if (e.mp < 35)
                        g.combat.cast(e, 5, e.pos, e.id, es, g.data, g.world, g.texts);
                }
            }
            else if (!dodge)
                goal += Vec2{std::sin(g.time * .3f + float(i)) * 1.4f, std::cos(g.time * .22f + float(i)) * 1.2f};
            if (distance(e.pos, goal) > .8f)
            {
                e.velocity = normalized(goal - e.pos) * e.speed * (dodge ? 1.3f : .75f);
                e.pos = g.world.move(e.pos, e.velocity * dt);
            }
            e.facing = target ? normalized(target->pos - e.pos) : normalized(e.velocity);
        }
    }
}
