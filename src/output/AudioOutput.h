#pragma once

#include <memory>

class AudioOutput {
public:
    virtual ~AudioOutput() = default;
    virtual bool open(int sampleRate, int channels) = 0;
    virtual void write(const float* buf, int frames)= 0;
    virtual void close()                            = 0;
    virtual int  latencyFrames()              const = 0;
    static std::unique_ptr<AudioOutput> create();
};
