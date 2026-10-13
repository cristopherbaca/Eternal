#pragma once
#include <memory>
namespace er
{
    // Synthesized effects; no external media files or scripting runtime.
    class Audio
    {
    public:
        explicit Audio(bool enabled = true);
        ~Audio();
        void play(int event);
        bool active() const;

    private:
        struct Impl;
        std::unique_ptr<Impl> impl;
    };
}
