#include "CoreAudioOutput.h"
#include <spdlog/spdlog.h>
#include <mach/mach_time.h>
#include <algorithm>

OSStatus CoreAudioOutput::renderCallback(void* inRefCon,
                                          AudioUnitRenderActionFlags* flags,
                                          const AudioTimeStamp* ts,
                                          UInt32 busNumber,
                                          UInt32 frames,
                                          AudioBufferList* data)
{
    auto* self = static_cast<CoreAudioOutput*>(inRefCon);
    if (!self) return kAudioHardwareUnspecifiedError;

    // Interleaved: all channels packed in mBuffers[0], mDataByteSize = frames * channels * 4
    float* buf = static_cast<float*>(data->mBuffers[0].mData);
    size_t samples = static_cast<size_t>(frames) * static_cast<size_t>(self->channels_);
    size_t got = self->ringBuffer_.read(buf, samples);

    // Zero-fill any underrun so we output silence rather than stale data
    if (got < samples) {
        std::fill(buf + got, buf + samples, 0.f);
    }

    return noErr;
}

bool CoreAudioOutput::open(int sampleRate, int channels) {
    close();

    AudioComponentDescription desc = {};
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;

    AudioComponent comp = AudioComponentFindNext(nullptr, &desc);
    if (!comp) {
        spdlog::error("CoreAudio: failed to find output component");
        return false;
    }

    OSStatus err = AudioComponentInstanceNew(comp, &audioUnit_);
    if (err != noErr) {
        spdlog::error("CoreAudio: failed to create AudioUnit, err={}", err);
        return false;
    }

    // Interleaved float32: LRLRLR... in a single buffer.
    // Non-interleaved caused render callback to read 2x samples into mBuffers[0]
    // while mBuffers[1] was left zeroed — pure noise/distortion on every track.
    AudioStreamBasicDescription streamDesc = {};
    streamDesc.mSampleRate       = sampleRate;
    streamDesc.mFormatID         = kAudioFormatLinearPCM;
    streamDesc.mFormatFlags      = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    streamDesc.mChannelsPerFrame = static_cast<UInt32>(channels);
    streamDesc.mFramesPerPacket  = 1;
    streamDesc.mBitsPerChannel   = 32;
    streamDesc.mBytesPerFrame    = static_cast<UInt32>(channels) * sizeof(float);
    streamDesc.mBytesPerPacket   = static_cast<UInt32>(channels) * sizeof(float);

    err = AudioUnitSetProperty(audioUnit_,
                               kAudioUnitProperty_StreamFormat,
                               kAudioUnitScope_Input,
                               0,
                               &streamDesc,
                               sizeof(streamDesc));
    if (err != noErr) {
        spdlog::error("CoreAudio: failed to set stream format, err={}", err);
        close();
        return false;
    }

    AURenderCallbackStruct callback;
    callback.inputProc = renderCallback;
    callback.inputProcRefCon = this;
    err = AudioUnitSetProperty(audioUnit_,
                               kAudioUnitProperty_SetRenderCallback,
                               kAudioUnitScope_Input,
                               0,
                               &callback,
                               sizeof(callback));
    if (err != noErr) {
        spdlog::error("CoreAudio: failed to set render callback, err={}", err);
        close();
        return false;
    }

    err = AudioUnitInitialize(audioUnit_);
    if (err != noErr) {
        spdlog::error("CoreAudio: failed to initialize AudioUnit, err={}", err);
        close();
        return false;
    }

    sampleRate_ = sampleRate;
    channels_ = channels;
    opened_ = true;

    err = AudioOutputUnitStart(audioUnit_);
    if (err != noErr) {
        spdlog::error("CoreAudio: failed to start AudioUnit, err={}", err);
        close();
        return false;
    }

    spdlog::info("CoreAudio: opened {}Hz {}ch", sampleRate, channels);
    return true;
}

void CoreAudioOutput::write(const float* buf, int frames) {
    if (!opened_) return;
    size_t samples = static_cast<size_t>(frames) * channels_;
    size_t written = 0;
    while (written < samples) {
        written += ringBuffer_.write(buf + written, samples - written);
        if (ringBuffer_.availableWrite() == 0) {
            mach_wait_until(mach_absolute_time() + 100000);
        }
    }
}

void CoreAudioOutput::close() {
    if (audioUnit_) {
        AudioOutputUnitStop(audioUnit_);
        AudioUnitUninitialize(audioUnit_);
        AudioComponentInstanceDispose(audioUnit_);
        audioUnit_ = nullptr;
    }
    ringBuffer_.reset();
    opened_ = false;
}
