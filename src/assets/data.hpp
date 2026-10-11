#pragma once
#include "entities/entity.hpp"
#include <filesystem>
namespace er {
struct ClassDef { float hp=0,mp=0,armor=0,power=0,speed=0; Color color{}; };
struct Ability { std::string name,effect; float cost=0,cooldown=0,power=0,radius=0,speed=0,range=0,duration=0; };
class Data {
public:
    std::array<ClassDef,5> classes{}; std::array<std::array<Ability,6>,5> abilities{};
    explicit Data(const std::filesystem::path& path={});
    void applyClass(Entity& e,Role role) const;
};
Role parseRole(const std::string& name);
}
