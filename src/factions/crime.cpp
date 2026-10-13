#include "game.hpp"
namespace er
{
    void Game::stealFrom(const Person &p)
    {
        auto &state = population.changes[p.id];
        if (state.gold < 0)
            state.gold = p.gold;
        if (state.gold < 5)
        {
            dialogue = "Their purse is empty.";
            return;
        }
        bool witnessed = false;
        for (const auto &witness : population.nearby)
            if (!population.away.count(witness.id) && (!population.changes.count(witness.id) || !population.changes.at(witness.id).recruited) && witness.id != p.id && distance(witness.pos, hero.pos) < 12)
            {
                witnessed = true;
                break;
            }
        int amount = std::min(10, state.gold);
        state.gold -= amount;
        hero.gold += amount;
        state.affinity -= 5;
        if (witnessed)
        {
            bounties[p.home] += 25;
            reputation[population.jobs[size_t(p.job)].faction] -= 2;
            dialogue = "A witness raises the alarm! Bounty +25 gold. Guards will pursue you.";
        }
        else
            dialogue = "You lift " + std::to_string(amount) + " gold unnoticed.";
    }
    void Game::settleBounty(const Person &p)
    {
        int &fine = bounties[p.home];
        if (!fine)
        {
            dialogue = "You have no bounty here.";
            return;
        }
        if (p.job != 2)
        {
            dialogue = "Speak to a Guard to settle your bounty.";
            return;
        }
        if (hero.gold >= fine)
        {
            hero.gold -= fine;
            dialogue = "Fine paid. You are free to go.";
        }
        else
        {
            time += 270;
            hero.xp = std::max(0, hero.xp - 25);
            dialogue = "Unable to pay, you serve a jail sentence. Six hours pass; -25 XP.";
        }
        fine = 0;
        mobs.erase(std::remove_if(mobs.begin(), mobs.end(), [](const Entity &e)
                                  { return e.name == "Pursuing Guard"; }),
                   mobs.end());
    }
    void Game::tickSociety(float dt)
    {
        societyTimer -= dt;
        if (societyTimer > 0 || world.interior)
            return;
        societyTimer = 1;
        for (auto &p : population.nearby)
        {
            auto state = population.changes.find(p.id);
            if (population.away.count(p.id) || (state != population.changes.end() && state->second.recruited))
                continue;
            if (p.job == 2 && bounties[p.home] > 0 && distance(p.pos, hero.pos) < 18 && std::none_of(mobs.begin(), mobs.end(), [](const Entity &e)
                                                                                                     { return e.alive && e.name == "Pursuing Guard"; }))
            {
                size_t before = mobs.size();
                spawnMob(p.pos, std::min(2, int(data.enemies.size()) - 1));
                if (mobs.size() > before)
                {
                    mobs.back().name = "Pursuing Guard";
                    message = "Guard: halt! Speak to a Guard to pay or serve your sentence.";
                }
            }
            for (auto &e : mobs)
                if (e.alive && distance(p.pos, e.pos) < 8 && e.name != "Pursuing Guard")
                {
                    if (p.job == 2)
                    {
                        dealDamage(e, 8, texts);
                        break;
                    }
                    else
                    {
                        p.pos = world.move(p.pos, normalized(p.pos - e.pos) * 3);
                        break;
                    }
                }
        }
    }
}
