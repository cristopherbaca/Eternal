#include "platform/audio.hpp"
#include <array>
#include <atomic>
#include <cmath>
#ifdef __APPLE__
#include <AudioToolbox/AudioToolbox.h>
#endif
namespace er
{
    struct Audio::Impl
    {
        bool running = false;
#ifdef __APPLE__
        AudioQueueRef queue = nullptr;
        std::array<int, 64> events{};
        std::atomic<unsigned> read{0}, write{0};
        struct Voice
        {
            int type = 0, left = 0, total = 1;
            float phase = 0;
        };
        std::array<Voice, 12> voices{};
        static void fill(void *context, AudioQueueRef queue, AudioQueueBufferRef buffer)
        {
            auto &self = *static_cast<Impl *>(context);
            auto r = self.read.load(std::memory_order_relaxed), w = self.write.load(std::memory_order_acquire);
            while (r != w)
            {
                int type = self.events[r];
                r = (r + 1) % 64;
                for (auto &voice : self.voices)
                    if (voice.left == 0)
                    {
                        int samples = type == 3 ? 12000 : type == 0 ? 7000
                                                                    : 2500;
                        voice = {type, samples, samples, 0};
                        break;
                    }
            }
            self.read.store(r, std::memory_order_release);
            auto *samples = static_cast<float *>(buffer->mAudioData);
            for (unsigned i = 0; i < buffer->mAudioDataBytesCapacity / sizeof(float); ++i)
            {
                float value = 0;
                for (auto &v : self.voices)
                    if (v.left > 0)
                    {
                        float progress = 1 - float(v.left) / v.total;
                        float hz = v.type == 0 ? 230 + progress * 450 : v.type == 1 ? 90
                                                                    : v.type == 2   ? 150
                                                                                    : 700 - progress * 480;
                        v.phase += 6.2831853f * hz / 48000;
                        float envelope = std::sin(3.14159265f * progress) * float(v.left) / v.total;
                        value += std::sin(v.phase) * envelope * (v.type == 2 ? .035f : .09f);
                        --v.left;
                    }
                samples[i] = std::tanh(value);
            }
            buffer->mAudioDataByteSize = buffer->mAudioDataBytesCapacity;
            AudioQueueEnqueueBuffer(queue, buffer, 0, nullptr);
        }
        ~Impl()
        {
            if (queue)
                AudioQueueDispose(queue, true);
        }
#endif
    };
    Audio::Audio(bool enabled) : impl(std::make_unique<Impl>())
    {
#ifdef __APPLE__
        if (!enabled)
            return;
        AudioStreamBasicDescription format{};
        format.mSampleRate = 48000;
        format.mFormatID = kAudioFormatLinearPCM;
        format.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
        format.mBytesPerPacket = format.mBytesPerFrame = sizeof(float);
        format.mFramesPerPacket = 1;
        format.mChannelsPerFrame = 1;
        format.mBitsPerChannel = 32;
        if (AudioQueueNewOutput(&format, &Impl::fill, impl.get(), nullptr, nullptr, 0, &impl->queue) != noErr)
        {
            impl->queue = nullptr;
            return;
        }
        for (int i = 0; i < 3; ++i)
        {
            AudioQueueBufferRef buffer = nullptr;
            if (AudioQueueAllocateBuffer(impl->queue, 1024 * sizeof(float), &buffer) != noErr)
                return;
            Impl::fill(impl.get(), impl->queue, buffer);
        }
        impl->running = AudioQueueStart(impl->queue, nullptr) == noErr;
#else
        (void)enabled;
#endif
    }
    Audio::~Audio() = default;
    bool Audio::active() const { return impl->running; }
    void Audio::play(int event)
    {
#ifdef __APPLE__
        if (!impl->running)
            return;
        auto w = impl->write.load(std::memory_order_relaxed), next = (w + 1) % 64;
        if (next == impl->read.load(std::memory_order_acquire))
            return;
        impl->events[w] = event;
        impl->write.store(next, std::memory_order_release);
#else
        (void)event;
#endif
    }
}
