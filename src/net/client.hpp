#pragma once
#include "net/socket.hpp"
#include "net/protocol.hpp"
#include <deque>
namespace er::net {
class Client {
public:
    Client(const std::string& address,const std::string& name,Role role);
    void input(const Input& input,Game& game);
    void update(float dt,Game& game);
    int id=0; bool ready=false;
private:
    Runtime runtime; Connection connection; std::deque<Snapshot> history; float age=0,silence=0;
    void chat(const std::string& line,Game& game);
};
}
