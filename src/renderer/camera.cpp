#include "renderer/camera.hpp"
#include "world/world.hpp"
namespace er
{
    float Camera::ground(Vec2 p) const
    {
        if (!terrain)
            return 0;
        if (!perspective)
            return terrain->visualHeight(p);
        // Match the mesh triangle diagonal exactly, including aiming and entity grounding.
        Vec2 base{std::floor(p.x / 4) * 4, std::floor(p.y / 4) * 4};
        float u = (p.x - base.x) / 4, v = (p.y - base.y) / 4;
        float b = terrain->elevation(base + Vec2{4, 0}), c = terrain->elevation(base + Vec2{0, 4});
        if (u + v <= 1)
            return terrain->elevation(base) * (1 - u - v) + b * u + c * v;
        return terrain->elevation(base + Vec2{4, 4}) * (u + v - 1) + b * (1 - v) + c * (1 - u);
    }
    View3D Camera::view() const
    {
        float pitch = firstPerson ? lookPitch : tilt;
        float ct = std::cos(pitch), st = std::sin(pitch), cy = std::cos(yaw), sy = std::sin(yaw);
        float standing = ground(center);
        // Wading/swimming keeps the first-person eye above the rendered water surface.
        if (firstPerson && terrain && terrain->water(center))
            standing = std::max(standing, terrain->waterLevel(center) - .8f);
        Vec3 forward{sy * ct, cy * ct, -st}, target{center.x, center.y, standing + 2.5f + jumpHeight};
        Vec3 eye = firstPerson ? Vec3{center.x, center.y, standing + eyeHeight + jumpHeight} : target - forward * (64 / std::max(.3f, zoom));
        return {{eye.x, eye.y, eye.z, 0}, {cy, -sy, 0, 0}, {sy * st, cy * st, ct, 0}, {forward.x, forward.y, forward.z, 0}, {1 / std::tan((firstPerson ? clamp(fov / zoom, 35 * pi / 180, 95 * pi / 180) : fov) * .5f), float(width) / (2 * height), firstPerson ? .08f : .5f, 280}};
    }
    float Camera::viewDepth(Vec2 p, float height) const
    {
        auto v = view();
        return (p.x - v.eye[0]) * v.forward[0] + (p.y - v.eye[1]) * v.forward[1] + (ground(p) + height - v.eye[2]) * v.forward[2];
    }
    float Camera::deviceDepth(Vec2 p, float height) const
    {
        if (!perspective)
            return 0;
        auto v = view();
        float z = viewDepth(p, height);
        if (z <= v.lens[2] || z >= v.lens[3])
            return 2;
        return v.lens[3] / (v.lens[3] - v.lens[2]) * (1 - v.lens[2] / z);
    }
    float Camera::spriteScale(Vec2 p, float physicalHeight) const
    {
        if (!perspective)
            return scale();
        auto v = view();
        float z = viewDepth(p, physicalHeight * .5f);
        if (z <= v.lens[2] || z >= v.lens[3])
            return 0;
        return std::min(float(height), physicalHeight * v.lens[0] * height / (10 * z));
    }
    Vec2 Camera::project(Vec2 p, float z) const
    {
        if (!perspective)
        {
            float s = scale();
            return {width * .5f + (p.x - center.x) * 2.2f * s, height * .55f + (p.y - center.y) * .9f * s - (z + ground(p) - ground(center)) * 1.5f * s};
        }
        auto v = view();
        Vec3 d{p.x - v.eye[0], p.y - v.eye[1], ground(p) + z - v.eye[2]};
        auto component = [&](const auto &axis)
        { return d.x * axis[0] + d.y * axis[1] + d.z * axis[2]; };
        float depth = component(v.forward);
        if (depth <= v.lens[2] || depth >= v.lens[3])
            return {-10000, -10000};
        return {(1 + component(v.right) * v.lens[0] / (depth * v.lens[1])) * width * .5f, (1 - component(v.up) * v.lens[0] / depth) * height * .5f};
    }
    bool Camera::pick(Vec2 screen, Vec2 &hit) const
    {
        if (!perspective)
        {
            hit = unproject(screen);
            return true;
        }
        auto v = view();
        float x = (screen.x / width * 2 - 1) * v.lens[1] / v.lens[0], y = (1 - screen.y / height * 2) / v.lens[0];
        Vec3 origin{v.eye[0], v.eye[1], v.eye[2]}, ray{v.forward[0] + v.right[0] * x + v.up[0] * y, v.forward[1] + v.right[1] * x + v.up[1] * y, v.forward[2] + v.right[2] * x + v.up[2] * y};
        auto above = [&](float t)
        { auto p=origin+ray*t; return p.z-ground({p.x,p.y}); };
        float previous = v.lens[2];
        if (above(previous) < 0)
            return false;
        for (float t = previous + 1; t <= v.lens[3]; t += 1)
        {
            if (above(t) <= 0)
            {
                float lo = previous, hi = t;
                for (int i = 0; i < 16; ++i)
                {
                    float mid = (lo + hi) * .5f;
                    if (above(mid) > 0)
                        lo = mid;
                    else
                        hi = mid;
                }
                auto p = origin + ray * ((lo + hi) * .5f);
                hit = {p.x, p.y};
                return true;
            }
            previous = t;
        }
        return false;
    }
    Vec2 Camera::unproject(Vec2 screen) const
    {
        if (perspective)
        {
            Vec2 hit;
            return pick(screen, hit) ? hit : center;
        }
        float s = scale();
        Vec2 p{center.x + (screen.x - width * .5f) / (2.2f * s), center.y + (screen.y - height * .55f) / (.9f * s)};
        float base = p.y, origin = ground(center);
        if (terrain)
            for (int i = 0; i < 3; ++i)
                p.y = base + (ground(p) - origin) * 1.5f / .9f;
        return p;
    }
}
