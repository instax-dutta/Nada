#pragma once

#include "BiquadFilter.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <array>

struct EQBand {
    float freq = 1000.f;
    float gainDb = 0.f;
    float Q = 1.41f;
    enum Type { Peak, LowShelf, HighShelf, LowPass, HighPass, Notch } type = Peak;
    bool enabled = true;
};

class ParametricEQ {
public:
    ParametricEQ();

    void process(float* interleavedBuf, int frames, int channels, int sampleRate);
    void getFrequencyResponse(int points, float* freqsHz, float* magDb);

    void setBand(int index, const EQBand& band);
    EQBand band(int index) const;
    int bandCount() const { return 10; }

    QJsonObject toJson() const;
    void fromJson(const QJsonObject& obj);

private:
    void recalcFilters(int sampleRate);

    struct BandState {
        EQBand band;
        BiquadFilter filterL;
        BiquadFilter filterR;
    };

    std::array<BandState, 10> bands_;
    int currentSampleRate_ = 44100;
};
