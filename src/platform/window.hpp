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
    Input poll();
    std::pair<int,int> size(int detail=1) const;
    bool ready() const;
    bool present(const Framebuffer& frame);
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
}
