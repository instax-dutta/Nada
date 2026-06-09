#include "ParametricEQ.h"
#include <cmath>
#include <spdlog/spdlog.h>

static const float kDefaultFreqs[10] = {32, 64, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};

static BiquadCoeffs makeCoeffs(const EQBand& band, double sr) {
    switch (band.type) {
    case EQBand::Peak:      return BiquadFilter::makePeak(band.freq, sr, band.gainDb, band.Q);
    case EQBand::LowShelf:  return BiquadFilter::makeLowShelf(band.freq, sr, band.gainDb, band.Q);
    case EQBand::HighShelf: return BiquadFilter::makeHighShelf(band.freq, sr, band.gainDb, band.Q);
    case EQBand::LowPass:   return BiquadFilter::makeLowPass(band.freq, sr, band.Q);
    case EQBand::HighPass:  return BiquadFilter::makeHighPass(band.freq, sr, band.Q);
    case EQBand::Notch:     return BiquadFilter::makeNotch(band.freq, sr, band.Q);
    }
    return {};
}

ParametricEQ::ParametricEQ() {
    for (int i = 0; i < 10; ++i) {
        bands_[i].band.freq = kDefaultFreqs[i];
        bands_[i].band.gainDb = 0.f;
        bands_[i].band.Q = 1.41f;
        bands_[i].band.type = EQBand::Peak;
        bands_[i].band.enabled = true;
    }
}

void ParametricEQ::setBand(int index, const EQBand& band) {
    if (index < 0 || index >= 10) return;
    bands_[index].band = band;
    double sr = currentSampleRate_;
    BiquadCoeffs coeffs = makeCoeffs(band, sr);
    bands_[index].filterL.setCoeffs(coeffs);
    bands_[index].filterR.setCoeffs(coeffs);
}

EQBand ParametricEQ::band(int index) const {
    if (index < 0 || index >= 10) return {};
    return bands_[index].band;
}

void ParametricEQ::process(float* buf, int frames, int channels, int sampleRate) {
    if (channels > 2) channels = 2;

    if (sampleRate != currentSampleRate_) {
        currentSampleRate_ = sampleRate;
        recalcFilters(sampleRate);
    }

    for (int f = 0; f < frames; ++f) {
        for (int ch = 0; ch < channels; ++ch) {
            float& s = buf[f * 2 + ch];
            for (int b = 0; b < 10; ++b) {
                if (!bands_[b].band.enabled) continue;
                BiquadFilter& filter = (ch == 0) ? bands_[b].filterL : bands_[b].filterR;
                s = filter.process(s);
            }
        }
    }
}

void ParametricEQ::getFrequencyResponse(int points, float* freqsHz, float* magDb) {
    int sr = currentSampleRate_;
    for (int i = 0; i < points; ++i) {
        double freq = 20.0 * std::pow(1000.0, static_cast<double>(i) / (points - 1));
        if (freq > sr / 2.0) freq = sr / 2.0;
        freqsHz[i] = static_cast<float>(freq);

        double w = 2.0 * M_PI * freq / sr;
        double re = 1.0;

        for (int b = 0; b < 10; ++b) {
            if (!bands_[b].band.enabled) continue;
            BiquadCoeffs c = makeCoeffs(bands_[b].band, sr);
            double cosW = std::cos(w);
            double sinW = std::sin(w);
            double H_re = (c.b0 + c.b1 * cosW + c.b2 * std::cos(2.0 * w))
                        / (1.0 + c.a1 * cosW + c.a2 * std::cos(2.0 * w));
            double H_im = (c.b1 * sinW + c.b2 * std::sin(2.0 * w))
                        / (1.0 + c.a1 * cosW + c.a2 * std::cos(2.0 * w));
            double mag = std::sqrt(H_re * H_re + H_im * H_im);
            re *= mag;
        }
        magDb[i] = 20.0f * static_cast<float>(std::log10(re));
    }
}

void ParametricEQ::recalcFilters(int sampleRate) {
    for (int b = 0; b < 10; ++b) {
        BiquadCoeffs coeffs = makeCoeffs(bands_[b].band, sampleRate);
        bands_[b].filterL.setCoeffs(coeffs);
        bands_[b].filterR.setCoeffs(coeffs);
    }
}

QJsonObject ParametricEQ::toJson() const {
    QJsonArray arr;
    for (int i = 0; i < 10; ++i) {
        QJsonObject bandObj;
        bandObj["freq"] = bands_[i].band.freq;
        bandObj["gainDb"] = bands_[i].band.gainDb;
        bandObj["Q"] = bands_[i].band.Q;
        bandObj["type"] = static_cast<int>(bands_[i].band.type);
        bandObj["enabled"] = bands_[i].band.enabled;
        arr.append(bandObj);
    }
    QJsonObject obj;
    obj["bands"] = arr;
    return obj;
}

void ParametricEQ::fromJson(const QJsonObject& obj) {
    QJsonArray arr = obj["bands"].toArray();
    int n = arr.size() < 10 ? arr.size() : 10;
    for (int i = 0; i < n; ++i) {
        QJsonObject bandObj = arr[i].toObject();
        bands_[i].band.freq = bandObj["freq"].toDouble(1000.0);
        bands_[i].band.gainDb = bandObj["gainDb"].toDouble(0.0);
        bands_[i].band.Q = bandObj["Q"].toDouble(1.41);
        bands_[i].band.type = static_cast<EQBand::Type>(bandObj["type"].toInt(0));
        bands_[i].band.enabled = bandObj["enabled"].toBool(true);
    }
    recalcFilters(currentSampleRate_);
}
