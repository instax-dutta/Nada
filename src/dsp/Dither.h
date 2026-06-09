#pragma once

#include <algorithm>
#include <random>

inline void applyDither(float* buf, int samples, int outputBitDepth) {
    if (outputBitDepth >= 32) return;
    float lsb = 1.0f / static_cast<float>(1 << (outputBitDepth - 1));
    static std::mt19937 rng(std::random_device{}());
    static std::uniform_real_distribution<float> dist(0.f, 1.f);
    for (int i = 0; i < samples; i++) {
        float noise = (dist(rng) - dist(rng)) * lsb;
        buf[i] = std::clamp(buf[i] + noise, -1.f, 1.f);
    }
}
