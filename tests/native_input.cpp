#include "platform/window.hpp"
#include "platform/mac_runtime.hpp"
#include "game.hpp"
#include "platform/audio.hpp"
#include <iostream>
#include <cstring>
#include <fstream>
#include <thread>
#include <chrono>
#include <stdexcept>
using namespace er::mac;
static void check(bool ok, const char *message)
{
    if (!ok)
        throw std::runtime_error(message);
}
int main(int argc, char **argv)
{
    {
        Pool pool;
        try
        {
            if (argc > 1 && std::strcmp(argv[1], "--audio-only") == 0)
            {
                er::Audio audio;
                check(audio.active(), "audio queue starts on the system output device");
                for (int event = 0; event < 4; ++event)
                {
                    audio.play(event);
                    std::this_thread::sleep_for(std::chrono::milliseconds(180));
                }
                std::cout << "PASS: synthesized audio queue, events and shutdown\n";
                return 0;
            }
            bool rendererOnly = argc > 1 && std::strcmp(argv[1], "--renderer-only") == 0;
            auto depthCapture = (std::filesystem::temp_directory_path() / ("eternal-depth-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".png")).string();
            if (rendererOnly)
                setenv("ER_CAPTURE", depthCapture.c_str(), 1);
            er::Window window;
            er::Game game;
            game.screen = er::Screen::Title;
            er::Framebuffer world(320, 100), ui(160, 50);
            game.render(world, 16, &ui);
            window.present(world, ui);
            // Exercise GPU conversion and reallocations without depending on desktop focus.
            if (rendererOnly)
            {
                er::Terrain3D mesh;
                mesh.update(game.world, game.hero.pos);
                er::Camera camera;
                camera.perspective = true;
                camera.firstPerson = true;
                camera.terrain = &game.world;
                camera.center = game.hero.pos;
                auto hud = window.interfaceSize();
                check(window.size(1).first > hud.first, "world detail is finer than HUD text");
                for (int detail : {0, 2, 1, 0})
                {
                    auto dimensions = window.size(detail);
                    world.resize(dimensions.first, dimensions.second);
                    camera.width = world.width;
                    camera.height = world.height;
                    while (!window.ready())
                        std::this_thread::sleep_for(std::chrono::duration<double>(.001));
                    check(window.present(world, ui, &mesh, &camera), "density change submits GPU conversion");
                    while (!window.ready())
                        std::this_thread::sleep_for(std::chrono::duration<double>(.001));
                    check(window.gpuMilliseconds() > 0, "ASCII conversion completes successfully");
                    check(window.interfaceSize() == hud, "world density preserves HUD size");
                }
                // Identical captures prove hidden glyphs do not leak through terrain/sky.
                for (auto &cell : ui.cells)
                    cell = {U'\0', er::white, er::background};
                auto captureDepth = [&](float depth, bool glyphs)
                {
                    world.clear();
                    if (glyphs)
                        for (auto &cell : world.cells)
                            cell = {U'#', {255, 0, 255}, {255, 0, 255}, depth};
                    while (!window.ready())
                        std::this_thread::sleep_for(std::chrono::duration<double>(.001));
                    setenv("ER_CAPTURE", depthCapture.c_str(), 1);
                    check(window.present(world, ui, &mesh, &camera), "depth comparison capture submits");
                    std::ifstream in(depthCapture, std::ios::binary);
                    return std::string(std::istreambuf_iterator<char>(in), {});
                };
                auto baseline = captureDepth(0, false);
                auto hidden = captureDepth(2, true);
                check(!baseline.empty() && baseline == hidden, "occluded glyphs reproduce unobstructed terrain exactly");
                auto visible = captureDepth(.01f, true);
                check(!visible.empty() && baseline != visible, "near glyphs remain visible in front of terrain");
                std::filesystem::remove(depthCapture);
                window.raw3D(true);
                check(window.present(world, ui, &mesh, &camera), "raw preview after density changes");
                while (!window.ready())
                    std::this_thread::sleep_for(std::chrono::duration<double>(.001));
                check(window.present(world, ui), "2D fallback after GPU conversion");
                while (!window.ready())
                    std::this_thread::sleep_for(std::chrono::duration<double>(.001));
                check(window.gpuMilliseconds() > 0, "fallback completes successfully");
                std::cout << "PASS: all ASCII densities, buffer resize, GPU depth occlusion, independent HUD, raw view and 2D fallback\n";
                return 0;
            }
            for (int i = 0; i < 30; ++i)
            {
                window.poll(true);
                std::this_thread::sleep_for(std::chrono::duration<double>(.01));
            }
            id native = send<id>(NSApp, "mainWindow");
            if (!native)
            {
                id windows = send(NSApp, "windows");
                for (NSUInteger i = 0; i < send<NSUInteger>(windows, "count"); ++i)
                {
                    id candidate = at(windows, i);
                    if (send<bool>(send(candidate, "title"), "hasPrefix:", str("Eternal")))
                    {
                        native = candidate;
                        break;
                    }
                }
            }
            send<id>(native, "makeMainWindow");
            send<id>(native, "makeKeyAndOrderFront:", nullptr);
            check(native != nullptr, "game has a main window");
            check(send<id>(native, "firstResponder") == send<id>(native, "contentView"), "game view owns keyboard focus");
            auto key = [&](id text, unsigned short code, bool down)
            {
                id event = send<id>(klass("NSEvent"), "keyEventWithType:location:modifierFlags:timestamp:windowNumber:context:characters:charactersIgnoringModifiers:isARepeat:keyCode:", down ? NSEventTypeKeyDown : NSEventTypeKeyUp, NSZeroPoint, 0, 0.0, send<long>(native, "windowNumber"), nullptr, text, text, false, code);
                send<id>(NSApp, "postEvent:atStart:", event, false);
            };
            key(str("\r"), 36, true);
            auto input = window.poll(true);
            game.input(input);
            check(game.screen == er::Screen::Create, "native Enter opens world creation");
            if (const char *path = std::getenv("ER_CREATE_CAPTURE"))
            {
                while (!window.ready())
                    std::this_thread::sleep_for(std::chrono::duration<double>(.001));
                setenv("ER_CAPTURE", path, 1);
                game.render(world, 16, &ui);
                window.present(world, ui);
            }
            game.menuIndex = 1;
            game.editingField = true;
            key(str("Q"), 12, true);
            game.input(window.poll(true, true));
            key(str("w"), 13, true);
            game.input(window.poll(true, true));
            key(str("p"), 35, true);
            game.input(window.poll(true, true));
            check(game.draftName == "Qwp" && !game.quit, "name entry preserves case and does not trigger controls");
            key(str("w"), 13, false);
            window.poll(true, true);
            game.editingField = false;
            game.screen = er::Screen::Playing;
            key(str("w"), 13, true);
            key(str("d"), 2, true);
            input = window.poll();
            game.input(input);
            check(game.velocity.x > 0 && game.velocity.y < 0, "native held WD moves diagonally");
            key(str("w"), 13, false);
            input = window.poll();
            game.input(input);
            check(game.velocity.x > 0 && game.velocity.y == 0, "native key release keeps the remaining direction");
            key(str("d"), 2, false);
            game.input(window.poll());
            check(er::length(game.velocity) == 0, "native release stops movement");
            key(str("p"), 35, true);
            game.input(window.poll());
            check(game.screen == er::Screen::Paused, "native P opens pause menu");
            game.render(world, 16, &ui);
            NSSize size = send<NSRect>(send<id>(native, "contentView"), "bounds").size;
            NSPoint point = NSMakePoint(44.5 / 160 * size.width, (1 - 15.5 / 50) * size.height);
            id click = send<id>(klass("NSEvent"), "mouseEventWithType:location:modifierFlags:timestamp:windowNumber:context:eventNumber:clickCount:pressure:", NSEventTypeLeftMouseDown, point, 0, 0.0, send<long>(native, "windowNumber"), nullptr, 1, 1, 1.f);
            send<id>(NSApp, "postEvent:atStart:", click, false);
            input = window.poll(true);
            check(input.click && input.mouseX == 44 && input.mouseY == 15, "native click uses event position in UI coordinates");
            game.input(input);
            check(game.screen == er::Screen::Playing, "native click resumes from scaled pause menu");
            send<id>(NSApp, "postEvent:atStart:", click, false);
            input = window.poll(false);
            check(input.click && input.mouseX == 89 && input.mouseY == 31, "gameplay mouse uses world coordinates");
            game.input(input);
            game.tick(1.f / 60);
            check(game.hero.cooldown[0] > 0, "native mouse click casts an ability");
            key(str("w"), 13, true);
            window.poll();
            send<id>(native, "resignKeyWindow");
            check(window.poll().movementKeys.empty(), "losing focus clears held movement");
            send<id>(native, "makeKeyWindow");
            game.atlas.focus(game.hero.pos);
            game.population.tick(game.atlas, game.world, game.time, 0, false);
            check(!game.population.nearby.empty(), "native scene has residents");
            game.hero.pos = game.population.nearby.front().pos;
            game.camera.center = game.hero.pos;
            game.tick(1.f / 60);
            auto capture = [&](const char *variable)
            {
                if (const char *path = std::getenv(variable))
                {
                    while (!window.ready())
                        std::this_thread::sleep_for(std::chrono::duration<double>(.001));
                    setenv("ER_CAPTURE", path, 1);
                    game.render(world, 16, &ui);
                    window.present(world, ui);
                }
            };
            capture("ER_LIVING_CAPTURE");
            key(str("y"), 16, true);
            game.input(window.poll());
            check(game.screen == er::Screen::Talk, "native Y opens resident dialogue");
            game.menuIndex = 3;
            key(str("\r"), 36, true);
            game.input(window.poll(true));
            check(!game.quests.empty(), "native dialogue accepts a generated quest");
            capture("ER_TALK_CAPTURE");
            key(str("\x1b"), 53, true);
            game.input(window.poll(true));
            key(str("u"), 32, true);
            game.input(window.poll());
            check(game.screen == er::Screen::Progress, "native U opens mastery and crafting");
            capture("ER_PROGRESS_CAPTURE");
            game.screen = er::Screen::Playing;
            game.camera.perspective = true;
            game.camera.terrain = &game.world;
            world.resize(160, 50);
            game.render(world, 16, &ui);
            er::Terrain3D mesh;
            mesh.update(game.world, game.hero.pos);
            while (!window.ready())
                std::this_thread::sleep_for(std::chrono::duration<double>(.001));
            check(window.present(world, ui, &mesh, &game.camera), "native 3D terrain submits");
            while (!window.ready())
                std::this_thread::sleep_for(std::chrono::duration<double>(.001));
            check(window.terrainMilliseconds() > 0, "offscreen terrain pass completes on GPU");
            key(str("\xef\x9c\x8b"), 100, true);
            window.poll();
            check(window.raw3D(), "F8 toggles raw 3D preview");
            key(str("\xef\x9c\x8c"), 101, true);
            window.poll();
            check(window.debug3D(), "F9 toggles renderer diagnostics");
            auto p3 = game.camera.project(game.hero.pos + er::Vec2{3, 1});
            NSPoint aimPoint = NSMakePoint((p3.x + .5f) / world.width * size.width, (1 - (p3.y + .5f) / world.height) * size.height);
            id aimClick = send<id>(klass("NSEvent"), "mouseEventWithType:location:modifierFlags:timestamp:windowNumber:context:eventNumber:clickCount:pressure:", NSEventTypeLeftMouseDown, aimPoint, 0, 0.0, send<long>(native, "windowNumber"), nullptr, 2, 1, 1.f);
            send<id>(NSApp, "postEvent:atStart:", aimClick, false);
            game.hero.cooldown[0] = 0;
            game.hero.mp = game.hero.maxMp;
            game.input(window.poll());
            game.tick(1.f / 60);
            check(game.hero.cooldown[0] > 0 && er::distance(game.aim, game.hero.pos) < 10, "native perspective ray aiming still casts");
            game.camera.firstPerson = true;
            game.camera.center = game.hero.pos;
            auto eye = game.camera.view().eye;
            check(std::abs(eye[0] - game.hero.pos.x) < .001f && std::abs(eye[1] - game.hero.pos.y) < .001f, "first-person eye tracks the player");
            send<id>(NSApp, "activateIgnoringOtherApps:", true);
            send<id>(native, "makeKeyAndOrderFront:", nullptr);
            send<id>(send<id>(klass("NSRunningApplication"), "currentApplication"), "activateWithOptions:", 0);
            for (int i = 0; i < 100 && !send<bool>(native, "isKeyWindow"); ++i)
            {
                window.poll(true);
                send<id>(native, "makeKeyWindow");
                std::this_thread::sleep_for(std::chrono::duration<double>(.01));
            }
            auto locked = window.poll(false, false, true);

            if (send<bool>(native, "isKeyWindow"))
                check(window.mouseCaptured() && locked.mouseX == world.width / 2 && locked.mouseY == world.height / 2, "captured first-person mouse aims through center crosshair");
            else
                check(!window.mouseCaptured(), "inactive game never captures the desktop mouse");
            auto beforeLook = game.camera.lookPitch;
            locked.orbitX = 60;
            locked.orbitY = -30;
            game.input(locked);
            check(game.camera.lookPitch < beforeLook && game.camera.yaw > 0, "first-person mouse look changes yaw and pitch");
            key(str("\x1b"), 53, true);
            game.input(window.poll(false, false, true));
            window.poll(true, false, true);
            check(game.screen == er::Screen::Paused && !window.mouseCaptured(), "Esc pauses and releases first-person mouse capture");
            std::cout << "PASS: native focus, Enter, diagonal holds, key release, pause, menu click, mouse aim/cast, Metal 3D, raw toggle and diagnostics\n";
            return 0;
        }
        catch (const std::exception &error)
        {
            std::cerr << error.what() << '\n';
            return 1;
        }
    }
}
