#pragma once
#include "npc/population.hpp"
namespace er {
enum class QuestKind { Dungeon, Delivery, Escort };
struct Quest {
    NpcId giver{}; PlaceId target{}; int serial=0;
    bool complete=false,rewarded=false;
    std::string title;
    QuestKind kind=QuestKind::Dungeon;
    NpcId contact{}; Vec2 escortPos{};
};
}
