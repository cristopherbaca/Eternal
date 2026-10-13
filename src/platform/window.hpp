#pragma once
#include "platform/terminal.hpp"
#include "renderer/framebuffer.hpp"
#include <memory>
#include "renderer/terrain3d.hpp"
namespace er
{
    // Native presentation uses the same framebuffer and simulation as the terminal.
    class Window
    {
    public:
        Window();
        ~Window();
        Input poll(bool menu = false, bool textEntry = false, bool firstPerson = false);
        std::pair<int, int> size(int detail = 1) const;
        bool ready() const;
        std::pair<int, int> interfaceSize() const;
        bool present(const Framebuffer &frame, const Framebuffer &interface, const Terrain3D *terrain = nullptr, const Camera *camera = nullptr, float backdrop = 1);
        void raw3D(bool enabled);
        bool raw3D() const;
        bool debug3D() const;
        bool mouseCaptured() const;
        double gpuMilliseconds() const;
        double terrainMilliseconds() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl;
    };
}
