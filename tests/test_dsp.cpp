#include <catch2/catch_test_macros.hpp>
#include "dsp/BiquadFilter.h"
#include "dsp/ParametricEQ.h"
#include "dsp/CrossfeedProcessor.h"
#include "dsp/SoftClipper.h"
#include "dsp/DspChain.h"
#include <cmath>
#include <vector>
#include <cstring>

constexpr int SR = 48000;
constexpr int CH = 2;

static std::vector<float> generateSine(int freq, int frames, float amplitude = 0.5f) {
    std::vector<float> buf(frames * CH);
    for (int i = 0; i < frames; ++i) {
        float s = amplitude * std::sin(2.0f * M_PI * freq * i / SR);
        buf[i * 2] = s;
        buf[i * 2 + 1] = s;
    }
    return buf;
}

static float measureRMS(const float* buf, int samples) {
    double sumSq = 0.0;
    for (int i = 0; i < samples; ++i) {
        sumSq += buf[i] * buf[i];
    }
    return std::sqrt(static_cast<float>(sumSq / samples));
}

static float measureLevelDb(const float* buf, int samples) {
    float rms = measureRMS(buf, samples);
    if (rms < 1e-10f) return -100.f;
    return 20.0f * std::log10(rms);
}

static bool approxEqual(float a, float b, float margin) {
    return std::abs(a - b) <= margin;
}

TEST_CASE("Peak filter boosts 1kHz by ~6dB", "[dsp][eq]") {
    int frames = SR / 10;
    auto input = generateSine(1000, frames);

    ParametricEQ eq;
    EQBand band;
    band.freq = 1000;
    band.gainDb = 6.0f;
    band.Q = 10.0f;
    band.type = EQBand::Peak;
    band.enabled = true;
    eq.setBand(4, band);

    auto output = input;
    eq.process(output.data(), frames, CH, SR);

    float beforeDb = measureLevelDb(input.data(), frames * CH);
    float afterDb = measureLevelDb(output.data(), frames * CH);
    float boostDb = afterDb - beforeDb;

    REQUIRE(approxEqual(boostDb, 6.0f, 0.5f));
}

TEST_CASE("Disabled DspChain is bit-transparent", "[dsp][chain]") {
    DspChain chain;
    int frames = 1000;
    auto input = generateSine(440, frames);
    auto output = input;

    chain.process(output.data(), frames, CH, SR);

    for (int i = 0; i < frames * CH; ++i) {
        REQUIRE(output[i] == input[i]);
    }
}

TEST_CASE("Crossfeed processes stereo content", "[dsp][crossfeed]") {
    CrossfeedProcessor cf;
    int frames = SR / 10;

    // Generate signal where L and R are different
    std::vector<float> input(frames * CH);
    for (int i = 0; i < frames; ++i) {
        input[i * 2]     = 0.5f * std::sin(2.0f * M_PI * 440 * i / SR);
        input[i * 2 + 1] = 0.3f * std::sin(2.0f * M_PI * 880 * i / SR);
    }

    auto before = input;
    auto after = input;
    cf.setLevel(CrossfeedProcessor::Low);
    cf.process(after.data(), frames, CH, SR);

    bool changed = false;
    for (int i = 0; i < frames * CH; ++i) {
        if (std::abs(after[i] - before[i]) > 1e-6f) {
            changed = true;
            break;
        }
    }

    REQUIRE(changed);
}

TEST_CASE("Soft clipper reduces peak amplitude", "[dsp][clipper]") {
    int frames = SR;
    auto input = generateSine(440, frames, 2.0f);
    auto output = input;

    applySoftClip(output.data(), frames * CH, 0.85f);

    float outPeak = 0.f;
    for (int i = 0; i < frames * CH; ++i) {
        if (std::abs(output[i]) > outPeak) outPeak = std::abs(output[i]);
    }

    REQUIRE(outPeak <= 0.85f + 1e-6f);
}

TEST_CASE("Biquad filter peak response", "[dsp][biquad]") {
    BiquadFilter filt;
    auto coeffs = BiquadFilter::makePeak(1000, SR, 6.0, 1.0);
    filt.setCoeffs(coeffs);

    int totalSamples = SR / 5;
    int settleSamples = SR / 50;
    std::vector<float> input(totalSamples);
    std::vector<float> output(totalSamples);
    for (int i = 0; i < totalSamples; ++i) {
        input[i] = 0.5f * std::sin(2.0f * M_PI * 1000 * i / SR);
        output[i] = filt.process(input[i]);
    }

    int measureSamples = totalSamples - settleSamples;
    float beforeDb = measureLevelDb(input.data() + settleSamples, measureSamples);
    float afterDb = measureLevelDb(output.data() + settleSamples, measureSamples);
    float boostDb = afterDb - beforeDb;

    REQUIRE(approxEqual(boostDb, 6.0f, 0.25f));
}
