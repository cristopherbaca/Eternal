#pragma once
#include <string>
#include <vector>
#include <cstdint>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#endif
namespace er::net {
#ifdef _WIN32
using Socket=SOCKET; constexpr Socket invalid=INVALID_SOCKET;
#else
using Socket=int; constexpr Socket invalid=-1;
#endif
class Runtime { public: Runtime(); ~Runtime(); };
void closeSocket(Socket socket); void nonblocking(Socket socket);
Socket listenOn(const std::string& address,uint16_t port);
Socket connectTo(const std::string& host,uint16_t port);
class Connection {
public:
    Socket socket=invalid; bool closed=false;
    explicit Connection(Socket s=invalid):socket(s) {} ~Connection();
    Connection(const Connection&)=delete; Connection& operator=(const Connection&)=delete;
    Connection(Connection&& other) noexcept; Connection& operator=(Connection&& other) noexcept;
    void send(const std::string& message); void flush(); std::vector<std::string> receive(); void close();
private: std::string incoming,outgoing; size_t offset=0;
};
std::string safeText(const std::string& text,size_t limit);
}
