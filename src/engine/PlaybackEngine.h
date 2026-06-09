#pragma once

#include <QObject>
#include <QThread>
#include <memory>
#include <atomic>
#include <QString>
#include <functional>
#include <vector>

class AudioDecoder;
class AudioOutput;
template<typename T> class RingBuffer;
class DspChain;

class PlaybackEngine : public QObject {
    Q_OBJECT
public:
    enum class State { Stopped, Playing, Paused };

    explicit PlaybackEngine(QObject* parent = nullptr);
    ~PlaybackEngine() override;

    DspChain* dspChain() { return dsp_; }
    State state() const { return state_.load(); }

    using VisualizerCallback = std::function<void(const float*, int)>;
    void setVisualizerCallback(VisualizerCallback cb) { vizCb_ = std::move(cb); }
    void setCrossfadeDuration(float seconds) { crossfadeSecs_.store(seconds); }
    float crossfadeDuration() const { return crossfadeSecs_.load(); }

public slots:
    void play(const QString& path);
    void pause();
    void resume();
    void stop();
    void seek(double ratio);
    void setVolume(float linear);

signals:
    void stateChanged(State);
    void positionChanged(double ratio);
    void durationChanged(int64_t ms);
    void trackFinished();

private:
    void decoderLoop();
    void outputLoop();

    std::unique_ptr<AudioDecoder> decoder_;
    std::unique_ptr<AudioDecoder> nextDecoder_;
    std::unique_ptr<AudioOutput> output_;
    RingBuffer<float>* ringBuffer_ = nullptr;
    DspChain* dsp_ = nullptr;

    QThread* decoderThread_ = nullptr;
    QThread* outputThread_ = nullptr;

    std::atomic<bool> playing_{false};
    std::atomic<bool> paused_{false};
    std::atomic<State> state_{State::Stopped};
    std::atomic<float> volume_{1.0f};
    std::atomic<float> crossfadeSecs_{0.0f};

    QString currentPath_;
    int64_t totalFrames_ = 0;
    int sampleRate_ = 0;
    int channels_ = 0;

    VisualizerCallback vizCb_;

    std::vector<float> crossfadeBuf_;
    int64_t crossfadeFrames_ = 0;
};
