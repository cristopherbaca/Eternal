#include "platform/terminal.hpp"
#include <csignal>
#include <cstdio>
#include <stdexcept>
#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cerrno>
#endif
namespace er {
namespace {
volatile std::sig_atomic_t alive=1;
constexpr const char restore[]="\x1b[0m\x1b[?1003l\x1b[?1006l\x1b[?25h\x1b[?7h\x1b[?1049l";
void stop(int) { alive=0; }
#ifndef _WIN32
termios saved{}; bool savedValid=false;
void crash(int sig) { if(savedValid) tcsetattr(STDIN_FILENO,TCSANOW,&saved); auto ignored=::write(STDOUT_FILENO,restore,sizeof(restore)-1); (void)ignored; _exit(128+sig); }
#else
BOOL WINAPI ctrl(DWORD) { alive=0; return TRUE; }
#endif
}
Input InputParser::feed(const std::string& bytes) {
    pending+=bytes; Input result; size_t i=0;
    while(i<pending.size()) {
        if(pending[i]!='\x1b') { result.keys+=pending[i++]; continue; }
        if(i+1==pending.size()) break;
        if(pending[i+1]!='[') { ++i; continue; }
        size_t end=i+2; while(end<pending.size()&&(pending[end]<'@'||pending[end]>'~')) ++end;
        if(end==pending.size()) break;
        const char last=pending[end];
        if(pending[i+2]=='<') {
            int b=0,x=0,y=0;
            if(std::sscanf(pending.substr(i+3,end-i-3).c_str(),"%d;%d;%d",&b,&x,&y)==3) {
                result.mouseX=x-1; result.mouseY=y-1;
                if(b&64) result.scroll+=(b&1)?-1:1; else if(!(b&32)&&(b&3)==0&&last=='M') result.click=true;
            }
        } else if(last=='A') result.keys+='^'; else if(last=='B') result.keys+='v'; else if(last=='C') result.keys+='>'; else if(last=='D') result.keys+='<';
        i=end+1;
    } pending.erase(0,i); if(pending.size()>128) pending.clear(); return result;
}
Terminal::Terminal(bool enable) {
    if(!enable) return;
#ifdef _WIN32
    in=GetStdHandle(STD_INPUT_HANDLE); out=GetStdHandle(STD_OUTPUT_HANDLE);
    if(!GetConsoleMode(in,&oldIn)||!GetConsoleMode(out,&oldOut)) throw std::runtime_error("Run in a terminal, or use --headless");
    if(!SetConsoleMode(out,oldOut|ENABLE_VIRTUAL_TERMINAL_PROCESSING)) throw std::runtime_error("VT terminal required");
    SetConsoleMode(in,ENABLE_WINDOW_INPUT|ENABLE_MOUSE_INPUT|ENABLE_EXTENDED_FLAGS); SetConsoleCtrlHandler(ctrl,TRUE);
#else
    if(!isatty(STDIN_FILENO)||!isatty(STDOUT_FILENO)) throw std::runtime_error("Run in a terminal, or use --headless");
    if(tcgetattr(STDIN_FILENO,&old)!=0) throw std::runtime_error("Cannot read terminal state");
    saved=old; savedValid=true; termios raw=old;
    raw.c_lflag&=static_cast<tcflag_t>(~(ICANON|ECHO|IEXTEN)); raw.c_iflag&=static_cast<tcflag_t>(~(IXON|ICRNL)); raw.c_cc[VMIN]=0; raw.c_cc[VTIME]=0;
    tcsetattr(STDIN_FILENO,TCSANOW,&raw); oldFlags=fcntl(STDIN_FILENO,F_GETFL); fcntl(STDIN_FILENO,F_SETFL,oldFlags|O_NONBLOCK);
    std::signal(SIGSEGV,crash); std::signal(SIGABRT,crash); std::signal(SIGTERM,stop); std::signal(SIGHUP,stop);
#endif
    alive=1; std::signal(SIGINT,stop); active=true; write("\x1b[?1049h\x1b[?25l\x1b[?7l\x1b[?1003h\x1b[?1006h\x1b[2J");
}
Terminal::~Terminal() {
    if(!active) return; write(restore);
#ifdef _WIN32
    SetConsoleMode(in,oldIn); SetConsoleMode(out,oldOut); SetConsoleCtrlHandler(ctrl,FALSE);
#else
    tcsetattr(STDIN_FILENO,TCSANOW,&old); fcntl(STDIN_FILENO,F_SETFL,oldFlags); savedValid=false;
#endif
}
bool Terminal::running() { return alive!=0; }
std::pair<int,int> Terminal::size() const {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO s{}; if(active&&GetConsoleScreenBufferInfo(out,&s)) return {s.srWindow.Right-s.srWindow.Left+1,s.srWindow.Bottom-s.srWindow.Top+1};
#else
    winsize s{}; if(active&&ioctl(STDOUT_FILENO,TIOCGWINSZ,&s)==0&&s.ws_col&&s.ws_row) return {s.ws_col,s.ws_row};
#endif
    return {200,60};
}
Input Terminal::poll() {
    if(!active) return {};
#ifdef _WIN32
    Input r; DWORD n=0; GetNumberOfConsoleInputEvents(in,&n);
    while(n--) {
        INPUT_RECORD e{}; DWORD got=0; ReadConsoleInputW(in,&e,1,&got);
        if(e.EventType==KEY_EVENT&&e.Event.KeyEvent.bKeyDown) {
            auto k=e.Event.KeyEvent; char c=k.uChar.AsciiChar;
            if(c) r.keys+=c; else if(k.wVirtualKeyCode==VK_UP) r.keys+='^'; else if(k.wVirtualKeyCode==VK_DOWN) r.keys+='v'; else if(k.wVirtualKeyCode==VK_LEFT) r.keys+='<'; else if(k.wVirtualKeyCode==VK_RIGHT) r.keys+='>';
        } else if(e.EventType==MOUSE_EVENT) {
            auto m=e.Event.MouseEvent; r.mouseX=m.dwMousePosition.X; r.mouseY=m.dwMousePosition.Y;
            r.click=m.dwEventFlags==0&&(m.dwButtonState&FROM_LEFT_1ST_BUTTON_PRESSED);
            if(m.dwEventFlags==MOUSE_WHEELED) r.scroll=static_cast<SHORT>(HIWORD(m.dwButtonState))>0?1:-1;
        }
    } return r;
#else
    std::string s; char buf[4096]; ssize_t n; while((n=::read(STDIN_FILENO,buf,sizeof(buf)))>0) s.append(buf,size_t(n)); return parser.feed(s);
#endif
}
void Terminal::write(const std::string& s) {
    if(!active||s.empty()) return;
#ifdef _WIN32
    DWORD written=0; WriteFile(out,s.data(),static_cast<DWORD>(s.size()),&written,nullptr);
#else
    size_t offset=0; while(offset<s.size()) { ssize_t n=::write(STDOUT_FILENO,s.data()+offset,s.size()-offset); if(n>0) offset+=size_t(n); else if(errno!=EINTR) break; }
#endif
}
}
