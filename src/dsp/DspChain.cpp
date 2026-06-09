#include "DspChain.h"
#include "ReplayGainProcessor.h"
#include "CrossfeedProcessor.h"
#include "SoftClipper.h"
#include "Dither.h"
#include <spdlog/spdlog.h>

DspChain::DspChain()
    : eq_(std::make_unique<ParametricEQ>())
    , rg_(std::make_unique<ReplayGainProcessor>())
    , cf_(std::make_unique<CrossfeedProcessor>())
{
}

DspChain::~DspChain() = default;

void DspChain::process(float* buf, int frames, int channels, int sr) {
    int samples = frames * channels;

    if (rgEnabled_.load(std::memory_order_relaxed)) {
        rg_->apply(buf, frames, channels);
    }

    if (eqEnabled_.load(std::memory_order_relaxed)) {
        eq_->process(buf, frames, channels, sr);
    }

    if (cfEnabled_.load(std::memory_order_relaxed) && channels >= 2) {
        cf_->process(buf, frames, channels, sr);
    }

    if (scEnabled_.load(std::memory_order_relaxed)) {
        applySoftClip(buf, samples, 0.85f);
    }

    if (ditherEnabled_.load(std::memory_order_relaxed) && outputBitDepth_ < 32) {
        applyDither(buf, samples, outputBitDepth_);
    }
}

void DspChain::setEqBand(int index, EQBand band) {
    std::lock_guard<std::mutex> lock(configMutex_);
    eq_->setBand(index, band);
}

void DspChain::setEqPreamp(float db) {
    (void)db;
}

void DspChain::setReplayGainMode(int mode) {
    std::lock_guard<std::mutex> lock(configMutex_);
    rg_->setMode(static_cast<ReplayGainProcessor::Mode>(mode));
}

void DspChain::setReplayGainValues(float trackDb, float trackPeak, float albumDb, float albumPeak) {
    std::lock_guard<std::mutex> lock(configMutex_);
    rg_->setTrackValues(trackDb, trackPeak);
    rg_->setAlbumValues(albumDb, albumPeak);
}

void DspChain::setCrossfeedLevel(int level) {
    std::lock_guard<std::mutex> lock(configMutex_);
    cf_->setLevel(static_cast<CrossfeedProcessor::Level>(level));
}

QJsonObject DspChain::toJson() const {
    QJsonObject obj;
    obj["eq"] = eq_->toJson();
    obj["eqEnabled"] = eqEnabled_.load();
    obj["rgEnabled"] = rgEnabled_.load();
    obj["cfEnabled"] = cfEnabled_.load();
    obj["scEnabled"] = scEnabled_.load();
    return obj;
}

void DspChain::fromJson(const QJsonObject& obj) {
    if (obj.contains("eq")) {
        eq_->fromJson(obj["eq"].toObject());
    }
    eqEnabled_.store(obj["eqEnabled"].toBool(false));
    rgEnabled_.store(obj["rgEnabled"].toBool(false));
    cfEnabled_.store(obj["cfEnabled"].toBool(false));
    scEnabled_.store(obj["scEnabled"].toBool(false));
}
