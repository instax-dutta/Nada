#pragma once

#include "AudioDecoder.h"

class WavDecoder : public AudioDecoder {
public:
    ~WavDecoder() override { close(); }
    bool open(const QString& path) override;
    int64_t totalFrames() const override { return totalFrames_; }
    int sampleRate() const override { return sampleRate_; }
    int channels() const override { return channels_; }
    int bitDepth() const override { return bitDepth_; }
    int64_t decode(float* buf, int64_t frames) override;
    void seek(int64_t frame) override;
    void close() override;

private:
    void* wav_ = nullptr;
    int64_t totalFrames_ = 0;
    int sampleRate_ = 0;
    int channels_ = 0;
    int bitDepth_ = 0;
};
