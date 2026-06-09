#pragma once

#include <cmath>

inline float softClip(float sample, float threshold) {
    if (threshold <= 0.0f) return sample;
    return std::tanh(sample / threshold) * threshold;
}

inline void applySoftClip(float* buf, int samples, float threshold) {
    if (threshold <= 0.0f) return;
    for (int i = 0; i < samples; ++i) {
        buf[i] = std::tanh(buf[i] / threshold) * threshold;
    }
}
