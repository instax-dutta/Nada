#include "ReplayGainScanner.h"
#include "../decoder/DecoderFactory.h"
#include "../dsp/BiquadFilter.h"
#include <QThreadPool>
#include <QtConcurrent/QtConcurrentRun>
#include <spdlog/spdlog.h>
#include <cmath>
#include <vector>

static float computeKWeightedMoment(const float* buf, int64_t frames, int sampleRate) {

    int blockSize = static_cast<int>(0.4 * sampleRate);

    BiquadFilter stage1, stage2;

    double w0_hs = 2.0 * M_PI * 1681.974 / sampleRate;
    double A_hs = std::pow(10.0, 3.9998 / 40.0);
    double S_hs = 1.0;
    double alpha_hs = std::sin(w0_hs) / 2.0 * std::sqrt((A_hs + 1.0 / A_hs) * (1.0 / S_hs - 1.0) + 2.0);
    double cosW0_hs = std::cos(w0_hs);
    BiquadCoeffs c1;
    c1.b0 = A_hs * ((A_hs + 1.0) - (A_hs - 1.0) * cosW0_hs + 2.0 * std::sqrt(A_hs) * alpha_hs);
    c1.b1 = 2.0 * A_hs * ((A_hs - 1.0) - (A_hs + 1.0) * cosW0_hs);
    c1.b2 = A_hs * ((A_hs + 1.0) - (A_hs - 1.0) * cosW0_hs - 2.0 * std::sqrt(A_hs) * alpha_hs);
    c1.a1 = -2.0 * ((A_hs - 1.0) + (A_hs + 1.0) * cosW0_hs);
    c1.a2 = (A_hs + 1.0) + (A_hs - 1.0) * cosW0_hs - 2.0 * std::sqrt(A_hs) * alpha_hs;
    double a0_1 = (A_hs + 1.0) + (A_hs - 1.0) * cosW0_hs + 2.0 * std::sqrt(A_hs) * alpha_hs;
    c1.b0 /= a0_1; c1.b1 /= a0_1; c1.b2 /= a0_1;
    c1.a1 /= a0_1; c1.a2 /= a0_1;
    stage1.setCoeffs(c1);

    double w0_hp = 2.0 * M_PI * 38.13547 / sampleRate;
    double Q_hp = 0.5;
    double alpha_hp = std::sin(w0_hp) / (2.0 * Q_hp);
    double cosW0_hp = std::cos(w0_hp);
    BiquadCoeffs c2;
    c2.b0 = (1.0 + cosW0_hp) / 2.0;
    c2.b1 = -(1.0 + cosW0_hp);
    c2.b2 = (1.0 + cosW0_hp) / 2.0;
    c2.a1 = -2.0 * cosW0_hp;
    c2.a2 = 1.0 - alpha_hp;
    double a0_2 = 1.0 + alpha_hp;
    c2.b0 /= a0_2; c2.b1 /= a0_2; c2.b2 /= a0_2;
    c2.a1 /= a0_2; c2.a2 /= a0_2;
    stage2.setCoeffs(c2);

    std::vector<float> channel1(frames);
    for (int64_t i = 0; i < frames; ++i) {
        channel1[i] = buf[i * 2];
    }

    std::vector<double> blockMeans;
    blockMeans.reserve(frames / blockSize + 1);

    for (int64_t i = 0; i < frames; ++i) {
        float s = channel1[i];
        s = stage1.process(s);
        s = stage2.process(s);
        channel1[i] = s;
    }

    for (int64_t i = 0; i + blockSize <= frames; i += blockSize) {
        double sumSq = 0.0;
        for (int j = 0; j < blockSize; ++j) {
            float s = channel1[i + j];
            sumSq += s * s;
        }
        double meanSq = sumSq / blockSize;
        if (meanSq > 0.0) {
            double lufs = -0.691 + 10.0 * std::log10(meanSq);
            if (lufs > -70.0) {
                blockMeans.push_back(meanSq);
            }
        }
    }

    if (blockMeans.empty()) {
        return -100.0f;
    }

    double total = 0.0;
    for (double m : blockMeans) total += m;
    double integratedMeanSq = total / blockMeans.size();
    double lufs = -0.691 + 10.0 * std::log10(integratedMeanSq);
    return static_cast<float>(lufs);
}

ReplayGainScanner::ReplayGainScanner(QObject* parent)
    : QObject(parent)
{
}

QFuture<ReplayGainResult> ReplayGainScanner::scan(const QString& path) {
    return QtConcurrent::run(QThreadPool::globalInstance(), [this, path]() {
        ReplayGainResult result = scanInternal(path);
        QMetaObject::invokeMethod(this, [this, path, result]() {
            emit scanFinished(path, result);
        }, Qt::QueuedConnection);
        return result;
    });
}

ReplayGainResult ReplayGainScanner::scanInternal(const QString& path) {
    ReplayGainResult result;

    auto decoder = DecoderFactory::createForFile(path);
    if (!decoder || !decoder->open(path)) {
        spdlog::error("ReplayGainScanner: failed to open {}", path.toStdString());
        return result;
    }

    int64_t totalFrames = decoder->totalFrames();
    int channels = decoder->channels();
    int sampleRate = decoder->sampleRate();

    int64_t framesRead = 0;
    float peak = 0.0f;
    std::vector<float> allPcm;
    allPcm.reserve(static_cast<size_t>(totalFrames * channels));

    std::vector<float> decodeBuf(4096 * channels);
    while (true) {
        int64_t read = decoder->decode(decodeBuf.data(), 4096);
        if (read <= 0) break;
        for (int64_t i = 0; i < read * channels; ++i) {
            float absVal = std::abs(decodeBuf[i]);
            if (absVal > peak) peak = absVal;
        }
        allPcm.insert(allPcm.end(), decodeBuf.data(), decodeBuf.data() + read * channels);
        framesRead += read;
    }
    decoder->close();

    if (framesRead == 0) return result;

    float loudnessLUFS = computeKWeightedMoment(allPcm.data(), framesRead, sampleRate);
    result.gainDb = -18.0f - loudnessLUFS;
    result.peakLinear = peak;
    result.valid = true;

    spdlog::info("ReplayGainScanner: {} => {} LUFS, gain {} dB, peak {}",
                 path.toStdString(), loudnessLUFS, result.gainDb, peak);
    return result;
}
