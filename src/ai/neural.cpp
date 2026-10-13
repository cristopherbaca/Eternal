#include "ai/neural.hpp"
#include <algorithm>
#include <cmath>
#include <istream>
#include <ostream>
namespace er {
uint32_t CombatNetwork::random() {
    randomState^=randomState<<13; randomState^=randomState>>17; randomState^=randomState<<5;
    return randomState;
}
CombatNetwork::CombatNetwork(uint32_t seed):randomState(seed?seed:1) {
    for(auto& w:first) w=(float(random()%10001)/5000.f-1)*.15f;
    for(auto& w:second) w=(float(random()%10001)/5000.f-1)*.05f;
}
std::array<float,CombatNetwork::hidden> CombatNetwork::activate(const Observation& x) const {
    std::array<float,hidden> h{};
    for(int j=0;j<hidden;++j) {
        float z=first[j*(inputs+1)+inputs];
        for(int i=0;i<inputs;++i) z+=first[j*(inputs+1)+i]*x[i];
        h[j]=std::tanh(z);
    }
    return h;
}
CombatNetwork::Scores CombatNetwork::predict(const Observation& x) const {
    auto h=activate(x); Scores scores{};
    for(int a=0;a<actions;++a) {
        scores[a]=second[a*(hidden+1)+hidden];
        for(int j=0;j<hidden;++j) scores[a]+=second[a*(hidden+1)+j]*h[j];
    }
    return scores;
}
int CombatNetwork::choose(const Observation& x,const Scores& prior,const std::array<bool,actions>& allowed) {
    auto scores=predict(x); int best=-1; std::array<int,actions> valid{}; int count=0;
    for(int a=0;a<actions;++a) if(allowed[a]) {
        valid[count++]=a;
        if(best<0||scores[a]+prior[a]>scores[best]+prior[best]) best=a;
    }
    // Small persistent exploration, independent of render frame rate.
    if(count&&random()%100<6) return valid[random()%uint32_t(count)];
    return best;
}
void CombatNetwork::learn(const Observation& x,int action,float reward) {
    if(action<0||action>=actions||!std::isfinite(reward)) return;
    for(float v:x) if(!std::isfinite(v)||std::abs(v)>1) return;
    auto h=activate(x); float error=std::clamp(std::clamp(reward,-1.f,1.f)-predict(x)[action],-1.f,1.f);
    const int base=action*(hidden+1);
    for(int j=0;j<hidden;++j) {
        float gradient=.025f*error*second[base+j]*(1-h[j]*h[j]);
        for(int i=0;i<inputs;++i) first[j*(inputs+1)+i]=std::clamp(first[j*(inputs+1)+i]+gradient*x[i],-3.f,3.f);
        first[j*(inputs+1)+inputs]=std::clamp(first[j*(inputs+1)+inputs]+gradient,-3.f,3.f);
        second[base+j]=std::clamp(second[base+j]+.025f*error*h[j],-3.f,3.f);
    }
    second[base+hidden]=std::clamp(second[base+hidden]+.025f*error,-3.f,3.f);
    if(updates<1000000000000ULL) ++updates;
}
void CombatNetwork::save(std::ostream& out) const {
    out<<randomState<<' '<<updates<<'\n';
    for(float w:first) out<<w<<' '; out<<'\n';
    for(float w:second) out<<w<<' '; out<<'\n';
}
bool CombatNetwork::load(std::istream& in) {
    CombatNetwork candidate;
    if(!(in>>candidate.randomState>>candidate.updates)||!candidate.randomState||candidate.updates>1000000000000ULL) return false;
    auto read=[&](auto& weights) { for(float& w:weights) if(!(in>>w)||!std::isfinite(w)||std::abs(w)>3) return false; return true; };
    if(!read(candidate.first)||!read(candidate.second)) return false;
    *this=candidate; return true;
}
}
