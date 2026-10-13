#include "game.hpp"
namespace er
{
    bool Game::questReady(const Quest &q) const
    {
        return q.complete || (q.kind == QuestKind::Dungeon && clearedPlaces.count(q.target));
    }
    bool Game::escortActive() const
    {
        return std::any_of(quests.begin(), quests.end(), [](const Quest &q)
                           { return q.kind == QuestKind::Escort && !q.complete; });
    }
    void Game::deliverQuests(NpcId recipient)
    {
        for (auto &q : quests)
            if (q.kind == QuestKind::Delivery && !q.complete && q.contact == recipient)
            {
                q.complete = true;
                dialogue = "Letter delivered. Return to the giver for your reward.";
                message = dialogue;
            }
    }
    void Game::tickQuests(float dt)
    {
        population.away.clear();
        for (auto &q : quests)
            if (q.kind == QuestKind::Escort && !q.complete)
            {
                population.away.insert(q.contact);
                if (world.interior || !hero.alive)
                    continue;
                float gap = distance(q.escortPos, hero.pos);
                bool danger = std::any_of(mobs.begin(), mobs.end(), [&](const Entity &e)
                                          { return e.alive && distance(e.pos, q.escortPos) < 10; });
                if (gap > 3 && gap < 35 && !danger)
                {
                    // Small bounded steps preserve collision even after a long frame.
                    float step = std::min({dt * hero.speed * .95f, gap - 3, 1.f});
                    q.escortPos = world.move(q.escortPos, normalized(hero.pos - q.escortPos) * step);
                }
                if (distance(q.escortPos, atlas.place(q.target).pos) < 18 && distance(hero.pos, q.escortPos) < 10)
                {
                    q.complete = true;
                    population.away.erase(q.contact);
                    message = "Traveler arrived safely. Return to the giver for your reward.";
                }
            }
    }
    void Game::renderEscorts(Scene &scene) const
    {
        for (const auto &q : quests)
            if (q.kind == QuestKind::Escort && !q.complete)
            {
                Color color{255, 222, 139};
                scene.point(q.escortPos, 3, color, U'o', {}, .5f);
                scene.point(q.escortPos, 2, color, U'e', {}, .5f);
                scene.point(q.escortPos, 1, color, int(time * 3) % 2 ? U'/' : U'\\', {}, .5f);
            }
    }
}
