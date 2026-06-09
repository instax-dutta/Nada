#pragma once

#include "../dsp/BiquadFilter.h"

class CrossfeedProcessor {
public:
    enum Level { Off, Low, Mid, High };

    CrossfeedProcessor() = default;

    void setLevel(Level level) {
        level_ = level;
        switch (level) {
        case Off:  enabled_ = false; return;
        case Low:  cutoff_ = 700;    mix_ = 0.25f; break;
        case Mid:  cutoff_ = 1500;   mix_ = 0.35f; break;
        case High: cutoff_ = 2500;   mix_ = 0.45f; break;
        }
        enabled_ = true;
    }

    Level level() const { return level_; }
    bool enabled() const { return enabled_; }

    void process(float* buf, int frames, int channels, int sampleRate) {
        if (!enabled_ || channels < 2) return;

        if (sampleRate != currentSr_) {
            currentSr_ = sampleRate;
            lpfL_.setCoeffs(BiquadFilter::makeLowPass(cutoff_, sampleRate, 0.707f));
            lpfR_.setCoeffs(BiquadFilter::makeLowPass(cutoff_, sampleRate, 0.707f));
        }

        for (int i = 0; i < frames; ++i) {
            float L = buf[i * 2];
            float R = buf[i * 2 + 1];
            float diff = L - R;
            float filteredL = lpfL_.process(diff);
            float filteredR = lpfR_.process(diff);
            buf[i * 2]     = L + filteredR * mix_;
            buf[i * 2 + 1] = R + filteredL * mix_;
        }
    }

private:
    Level level_ = Off;
    bool enabled_ = false;
    float cutoff_ = 700;
    float mix_ = 0.25f;
    int currentSr_ = 44100;
    BiquadFilter lpfL_;
    BiquadFilter lpfR_;
};
