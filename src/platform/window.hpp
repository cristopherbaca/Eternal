#pragma once
#include "platform/terminal.hpp"
#include "renderer/framebuffer.hpp"
#include <memory>
namespace er {
// Native presentation uses the same framebuffer and simulation as the terminal.
class Window {
public:
    Window();
    ~Window();
    Input poll(bool menu=false,bool textEntry=false);
    std::pair<int,int> size(int detail=1) const;
    bool ready() const;
    std::pair<int,int> interfaceSize() const;
    bool present(const Framebuffer& frame,const Framebuffer& interface);
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
