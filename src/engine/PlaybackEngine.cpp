#include "PlaybackEngine.h"
#include "RingBuffer.h"
#include "../decoder/DecoderFactory.h"
#include "../output/AudioOutput.h"
#include "../dsp/DspChain.h"
#include <QTimer>
#include <spdlog/spdlog.h>
#include <thread>
#include <cmath>

PlaybackEngine::PlaybackEngine(QObject* parent)
    : QObject(parent)
    , dsp_(new DspChain())
{
}

PlaybackEngine::~PlaybackEngine() {
    stop();
}

void PlaybackEngine::play(const QString& path) {
    if (crossfadeSecs_.load() > 0 && playing_.load()) {
        nextDecoder_ = DecoderFactory::createForFile(path);
        if (!nextDecoder_ || !nextDecoder_->open(path)) {
            spdlog::error("PlaybackEngine: crossfade preload failed {}", path.toStdString());
            return;
        }
        int64_t cfFrames = static_cast<int64_t>(crossfadeSecs_.load() * sampleRate_);
        crossfadeBuf_.resize(cfFrames * channels_);
        crossfadeFrames_ = nextDecoder_->decode(crossfadeBuf_.data(), cfFrames);
        currentPath_ = path;
        return;
    }

    stop();

    currentPath_ = path;
    decoder_ = DecoderFactory::createForFile(path);
    if (!decoder_ || !decoder_->open(path)) {
        spdlog::error("PlaybackEngine: failed to open {}", path.toStdString());
        return;
    }

    totalFrames_ = decoder_->totalFrames();
    sampleRate_ = decoder_->sampleRate();
    channels_ = decoder_->channels();

    int64_t durationMs = (totalFrames_ * 1000) / sampleRate_;
    emit durationChanged(durationMs);

    ringBuffer_ = new RingBuffer<float>(static_cast<size_t>(sampleRate_ * channels_ * 2));

    output_ = AudioOutput::create();
    if (!output_->open(sampleRate_, channels_)) {
        spdlog::error("PlaybackEngine: failed to open audio output");
        return;
    }

    playing_.store(true);
    paused_.store(false);

    decoderThread_ = new QThread(this);
    outputThread_ = new QThread(this);

    QObject::connect(decoderThread_, &QThread::started, this, [this]() {
        decoderLoop();
    }, Qt::DirectConnection);

    QObject::connect(outputThread_, &QThread::started, this, [this]() {
        outputLoop();
    }, Qt::DirectConnection);

    decoderThread_->start();
    outputThread_->start();

    emit stateChanged(State::Playing);
    state_.store(State::Playing);
}

void PlaybackEngine::pause() {
    paused_.store(true);
    state_.store(State::Paused);
    emit stateChanged(State::Paused);
}

void PlaybackEngine::resume() {
    paused_.store(false);
    state_.store(State::Playing);
    emit stateChanged(State::Playing);
}

void PlaybackEngine::stop() {
    playing_.store(false);
    paused_.store(false);
    state_.store(State::Stopped);

    if (decoderThread_) {
        decoderThread_->quit();
        decoderThread_->wait(3000);
        delete decoderThread_;
        decoderThread_ = nullptr;
    }
    if (outputThread_) {
        outputThread_->quit();
        outputThread_->wait(3000);
        delete outputThread_;
        outputThread_ = nullptr;
    }

    if (output_) {
        output_->close();
        output_.reset();
    }

    if (decoder_) {
        decoder_->close();
        decoder_.reset();
    }
    nextDecoder_.reset();

    if (ringBuffer_) {
        delete ringBuffer_;
        ringBuffer_ = nullptr;
    }

    crossfadeBuf_.clear();
    crossfadeFrames_ = 0;

    emit stateChanged(State::Stopped);
}

void PlaybackEngine::seek(double ratio) {
    if (!decoder_ || totalFrames_ <= 0) return;
    int64_t frame = static_cast<int64_t>(ratio * totalFrames_);
    decoder_->seek(frame);
    if (ringBuffer_) ringBuffer_->reset();
    emit positionChanged(ratio);
}

void PlaybackEngine::setVolume(float linear) {
    volume_.store(linear, std::memory_order_relaxed);
}

void PlaybackEngine::decoderLoop() {
    std::vector<float> tempBuf(4096 * channels_);

    while (playing_.load(std::memory_order_relaxed)) {
        if (paused_.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        size_t available = ringBuffer_->availableWrite();
        if (available == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        int64_t framesToRead = std::min<int64_t>(available / channels_, 4096);
        int64_t framesRead = decoder_->decode(tempBuf.data(), framesToRead);

        if (framesRead > 0) {
            ringBuffer_->write(tempBuf.data(), static_cast<size_t>(framesRead * channels_));
        }

        if (framesRead == 0) {
            playing_.store(false);
            QMetaObject::invokeMethod(this, [this]() {
                emit trackFinished();
            }, Qt::QueuedConnection);
            break;
        }
    }
}

void PlaybackEngine::outputLoop() {
    std::vector<float> tempBuf(4096 * channels_);
    int64_t totalFramesWritten = 0;

    while (playing_.load(std::memory_order_relaxed) ||
           ringBuffer_->availableRead() > 0) {
        if (paused_.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        size_t samplesToRead = ringBuffer_->availableRead();
        if (samplesToRead == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        size_t toProcess = std::min<size_t>(samplesToRead, 4096 * channels_);
        size_t samplesRead = ringBuffer_->read(tempBuf.data(), toProcess);
        int framesRead = static_cast<int>(samplesRead / channels_);

        if (framesRead > 0) {
            float vol = volume_.load(std::memory_order_relaxed);
            if (vol != 1.0f) {
                for (size_t i = 0; i < samplesRead; ++i) {
                    tempBuf[i] *= vol;
                }
            }

            if (crossfadeFrames_ > 0) {
                int cfFrames = std::min(framesRead, static_cast<int>(crossfadeFrames_));
                for (int i = 0; i < cfFrames; ++i) {
                    float fadeOut = 1.0f - (float)i / cfFrames;
                    float fadeIn = (float)i / cfFrames;
                    for (int c = 0; c < channels_; ++c) {
                        int idx = i * channels_ + c;
                        tempBuf[idx] = tempBuf[idx] * fadeOut + crossfadeBuf_[idx] * fadeIn;
                    }
                }
                crossfadeFrames_ -= cfFrames;
                if (crossfadeFrames_ <= 0) {
                    crossfadeBuf_.clear();
                    crossfadeFrames_ = 0;
                    QMetaObject::invokeMethod(this, [this]() {
                        play(currentPath_);
                    }, Qt::QueuedConnection);
                }
            }

            dsp_->process(tempBuf.data(), framesRead, channels_, sampleRate_);
            output_->write(tempBuf.data(), framesRead);
            totalFramesWritten += framesRead;

            if (vizCb_) {
                vizCb_(tempBuf.data(), framesRead);
            }

            double ratio = static_cast<double>(totalFramesWritten) / totalFrames_;
            QMetaObject::invokeMethod(this, [this, ratio]() {
                emit positionChanged(ratio);
            }, Qt::QueuedConnection);
        }
    }

    QMetaObject::invokeMethod(this, [this]() {
        stop();
    }, Qt::QueuedConnection);
}
