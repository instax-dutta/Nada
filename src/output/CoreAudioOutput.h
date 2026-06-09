#pragma once

#include "AudioOutput.h"
#include "../engine/RingBuffer.h"
#include <AudioToolbox/AudioToolbox.h>
#include <vector>

class CoreAudioOutput : public AudioOutput {
public:
    CoreAudioOutput() = default;
    ~CoreAudioOutput() override { close(); }
    bool open(int sampleRate, int channels) override;
    void write(const float* buf, int frames) override;
    void close() override;
    int latencyFrames() const override { return 0; }

private:
    static OSStatus renderCallback(void* inRefCon,
                                   AudioUnitRenderActionFlags* flags,
                                   const AudioTimeStamp* ts,
                                   UInt32 busNumber,
                                   UInt32 frames,
                                   AudioBufferList* data);

    AudioUnit audioUnit_ = nullptr;
    int sampleRate_ = 0;
    int channels_ = 0;
    bool opened_ = false;
    RingBuffer<float> ringBuffer_{44100 * 2 * 2};
};
