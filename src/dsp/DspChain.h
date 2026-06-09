#pragma once

#include <QJsonObject>
#include <memory>
#include <atomic>
#include <mutex>
#include "ParametricEQ.h"

class ReplayGainProcessor;
class CrossfeedProcessor;

class DspChain {
public:
    DspChain();
    ~DspChain();

    void process(float* buf, int frames, int channels, int sr);

    void setEqEnabled(bool v)      { eqEnabled_.store(v, std::memory_order_relaxed); }
    void setReplayGainEnabled(bool v) { rgEnabled_.store(v, std::memory_order_relaxed); }
    void setCrossfeedEnabled(bool v)  { cfEnabled_.store(v, std::memory_order_relaxed); }
    void setSoftClipEnabled(bool v)   { scEnabled_.store(v, std::memory_order_relaxed); }
    void setDitherEnabled(bool v, int bitDepth) {
        ditherEnabled_.store(v, std::memory_order_relaxed);
        outputBitDepth_ = bitDepth;
    }

    void setEqBand(int index, EQBand band);
    void setEqPreamp(float db);
    void setReplayGainMode(int mode);
    void setReplayGainValues(float trackDb, float trackPeak, float albumDb, float albumPeak);
    void setCrossfeedLevel(int level);

    QJsonObject toJson() const;
    void fromJson(const QJsonObject&);

    ParametricEQ& eq() { return *eq_; }

private:
    std::unique_ptr<ParametricEQ> eq_;
    std::unique_ptr<ReplayGainProcessor> rg_;
    std::unique_ptr<CrossfeedProcessor> cf_;

    std::atomic<bool> eqEnabled_{false};
    std::atomic<bool> rgEnabled_{false};
    std::atomic<bool> cfEnabled_{false};
    std::atomic<bool> scEnabled_{false};
    std::atomic<bool> ditherEnabled_{false};
    int outputBitDepth_ = 32;

    std::mutex configMutex_;
};
