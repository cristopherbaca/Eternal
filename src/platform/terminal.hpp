#pragma once
#include <string>
#include <utility>
#include <chrono>
#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#endif
namespace er {
struct Input { std::string keys,movementKeys; int mouseX=-1,mouseY=-1,scroll=0; bool click=false,cursorReport=false,heldMovement=false,sprint=false; float orbitX=0,orbitY=0; };
class InputParser { public: Input feed(const std::string& bytes); private: std::string pending; std::chrono::steady_clock::time_point escapeSince{}; };
class Terminal {
public:
    explicit Terminal(bool enable=true); ~Terminal();
    Terminal(const Terminal&)=delete; Terminal& operator=(const Terminal&)=delete;
    Input poll(); std::pair<int,int> size() const; void write(const std::string& bytes); static bool running();
    bool frameReady(); void present(const std::string& bytes);
private:
    bool active=false; InputParser parser;
    bool synchronize=false,pendingFrame=false;
    std::chrono::steady_clock::time_point sentFrame{};
#ifdef _WIN32
    HANDLE in=INVALID_HANDLE_VALUE,out=INVALID_HANDLE_VALUE; DWORD oldIn=0,oldOut=0; UINT oldCodePage=0;
#else
    termios old{}; int oldFlags=0;
#endif
};
}
