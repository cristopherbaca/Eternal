#pragma once
#include <array>
#include <cstdint>
#include <iosfwd>
namespace er {
// A contextual bandit: predict the next short combat exchange's reward.
// Shared by archetype; transient credit assignment lives on each mob.
class CombatNetwork {
public:
    static constexpr int inputs=7, hidden=12, actions=5;
    using Observation=std::array<float,inputs>;
    using Scores=std::array<float,actions>;
    explicit CombatNetwork(uint32_t seed=1);
    Scores predict(const Observation&) const;
    int choose(const Observation&,const Scores& prior,const std::array<bool,actions>& allowed);
    void learn(const Observation&,int action,float reward);
    void save(std::ostream&) const;
    bool load(std::istream&);
    uint64_t updates=0;
private:
    std::array<float,hidden*(inputs+1)> first{};
    std::array<float,actions*(hidden+1)> second{};
    uint32_t randomState=1;
    uint32_t random();
    std::array<float,hidden> activate(const Observation&) const;
};
struct CombatExperience {
    CombatNetwork::Observation observation{};
    float timer=0,hp=0,distance=0,reward=0;
    int action=-1,target=0;
    bool learning=true;
};
}
