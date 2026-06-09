#pragma once

#include <cmath>

class ReplayGainProcessor {
public:
    enum Mode { Disabled, TrackGain, AlbumGain };

    void setMode(Mode m) { mode_ = m; }
    Mode mode() const { return mode_; }

    void setTrackValues(float gainDb, float peakLinear) {
        trackGainDb_ = gainDb;
        trackPeak_ = peakLinear;
    }

    void setAlbumValues(float gainDb, float peakLinear) {
        albumGainDb_ = gainDb;
        albumPeak_ = peakLinear;
    }

    void setPreamp(float preampDb) { preampDb_ = preampDb; }

    void apply(float* buf, int frames, int channels) {
        if (mode_ == Disabled) return;

        float gain = (mode_ == TrackGain) ? trackGainDb_ : albumGainDb_;
        float peak = (mode_ == TrackGain) ? trackPeak_ : albumPeak_;
        float linear = std::pow(10.0f, (gain + preampDb_) / 20.0f);
        if (linear * peak > 1.0f) linear = 1.0f / peak;

        int samples = frames * channels;
        for (int i = 0; i < samples; ++i) {
            buf[i] *= linear;
        }
    }

private:
    Mode mode_ = Disabled;
    float trackGainDb_ = 0.f;
    float trackPeak_ = 1.f;
    float albumGainDb_ = 0.f;
    float albumPeak_ = 1.f;
    float preampDb_ = 0.f;
};
