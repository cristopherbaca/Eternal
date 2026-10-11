#include "net/socket.hpp"
#include <stdexcept>
#include <cerrno>
#include <cstring>
#ifndef _WIN32
#include <netinet/tcp.h>
#endif
namespace er::net {
Runtime::Runtime() {
#ifdef _WIN32
    WSADATA d{}; if(WSAStartup(MAKEWORD(2,2),&d)!=0) throw std::runtime_error("Winsock startup failed");
#endif
}
Runtime::~Runtime() {
#ifdef _WIN32
    WSACleanup();
#endif
}
void closeSocket(Socket s) { if(s==invalid) return;
#ifdef _WIN32
    closesocket(s);
#else
    ::close(s);
#endif
}
void nonblocking(Socket s) {
#ifdef _WIN32
    u_long enabled=1; ioctlsocket(s,FIONBIO,&enabled);
#else
    fcntl(s,F_SETFL,fcntl(s,F_GETFL)|O_NONBLOCK);
#endif
    int yes=1; setsockopt(s,IPPROTO_TCP,TCP_NODELAY,reinterpret_cast<const char*>(&yes),sizeof(yes));
#ifdef SO_NOSIGPIPE
    setsockopt(s,SOL_SOCKET,SO_NOSIGPIPE,&yes,sizeof(yes));
#endif
}
static bool wouldBlock() {
#ifdef _WIN32
    int e=WSAGetLastError(); return e==WSAEWOULDBLOCK||e==WSAEINPROGRESS||e==WSAEINTR;
#else
    return errno==EAGAIN||errno==EWOULDBLOCK||errno==EINPROGRESS||errno==EINTR;
#endif
}
Socket listenOn(const std::string& address,uint16_t port) {
    Socket s=::socket(AF_INET,SOCK_STREAM,0); if(s==invalid) throw std::runtime_error("Cannot create server socket");
    int yes=1; setsockopt(s,SOL_SOCKET,SO_REUSEADDR,reinterpret_cast<const char*>(&yes),sizeof(yes));
    sockaddr_in addr{}; addr.sin_family=AF_INET; addr.sin_port=htons(port);
    if(inet_pton(AF_INET,address.c_str(),&addr.sin_addr)!=1||::bind(s,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))!=0||::listen(s,16)!=0) { closeSocket(s); throw std::runtime_error("Cannot bind "+address+":"+std::to_string(port)); }
    nonblocking(s); return s;
}
Socket connectTo(const std::string& host,uint16_t port) {
    addrinfo hints{},*list=nullptr; hints.ai_family=AF_INET; hints.ai_socktype=SOCK_STREAM;
    if(getaddrinfo(host.c_str(),std::to_string(port).c_str(),&hints,&list)!=0) throw std::runtime_error("Cannot resolve server "+host);
    Socket connected=invalid;
    for(auto* a=list;a;a=a->ai_next) {
        Socket s=::socket(a->ai_family,a->ai_socktype,a->ai_protocol); if(s==invalid) continue; nonblocking(s);
        int rc=::connect(s,a->ai_addr,static_cast<int>(a->ai_addrlen));
        if(rc!=0&&wouldBlock()) {
            fd_set writeSet; FD_ZERO(&writeSet); FD_SET(s,&writeSet); timeval timeout{3,0};
            if(select(static_cast<int>(s)+1,nullptr,&writeSet,nullptr,&timeout)>0) {
                int error=0;
#ifdef _WIN32
                int size=sizeof(error);
#else
                socklen_t size=sizeof(error);
#endif
                getsockopt(s,SOL_SOCKET,SO_ERROR,reinterpret_cast<char*>(&error),&size); rc=error==0?0:-1;
            }
        }
        if(rc==0) { connected=s; break; } closeSocket(s);
    }
    freeaddrinfo(list); if(connected==invalid) throw std::runtime_error("Cannot connect to "+host+":"+std::to_string(port)); return connected;
}
Connection::~Connection() { closeSocket(socket); }
Connection::Connection(Connection&& other) noexcept { *this=std::move(other); }
Connection& Connection::operator=(Connection&& other) noexcept {
    if(this!=&other) { closeSocket(socket); socket=other.socket; other.socket=invalid; closed=other.closed; incoming=std::move(other.incoming); outgoing=std::move(other.outgoing); offset=other.offset; } return *this;
}
void Connection::close() { closeSocket(socket); socket=invalid; closed=true; }
void Connection::send(const std::string& message) {
    if(closed) return;
    if(message.size()>512*1024||outgoing.size()-offset>2*1024*1024) { close(); return; }
    if(offset) { outgoing.erase(0,offset); offset=0; }
    uint32_t length=htonl(static_cast<uint32_t>(message.size())); outgoing.append(reinterpret_cast<char*>(&length),4); outgoing+=message; flush();
}
void Connection::flush() {
    while(!closed&&offset<outgoing.size()) {
#ifdef MSG_NOSIGNAL
        constexpr int flags=MSG_NOSIGNAL;
#else
        constexpr int flags=0;
#endif
        int n=static_cast<int>(::send(socket,outgoing.data()+offset,static_cast<int>(outgoing.size()-offset),flags));
        if(n>0) offset+=size_t(n); else if(n<0&&wouldBlock()) break; else { close(); break; }
    }
    if(offset==outgoing.size()) { outgoing.clear(); offset=0; }
}
std::vector<std::string> Connection::receive() {
    std::vector<std::string> messages; char buf[16384];
    for(int i=0;i<64&&!closed;++i) { int n=static_cast<int>(recv(socket,buf,sizeof(buf),0)); if(n>0) incoming.append(buf,size_t(n)); else if(n<0&&wouldBlock()) break; else { close(); break; } if(incoming.size()>2*1024*1024) { close(); break; } }
    size_t consumed=0;
    while(!closed&&incoming.size()-consumed>=4) {
        uint32_t n=0; std::memcpy(&n,incoming.data()+consumed,4); n=ntohl(n);
        if(n>512*1024) { close(); break; } if(incoming.size()-consumed<4+n) break;
        messages.push_back(incoming.substr(consumed+4,n)); consumed+=4+n; if(messages.size()==128) break;
    }
    incoming.erase(0,consumed); return messages;
}
std::string safeText(const std::string& s,size_t limit) { std::string out; for(unsigned char c:s) if(c>=32&&c<127&&out.size()<limit) out+=char(c); return out; }
}
