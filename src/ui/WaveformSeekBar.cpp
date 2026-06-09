#include "WaveformSeekBar.h"
#include "Theme.h"
#include "../decoder/DecoderFactory.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QtConcurrent/QtConcurrentRun>
#include <QThreadPool>
#include <QFutureWatcher>
#include <cmath>

WaveformSeekBar::WaveformSeekBar(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(36);
    setMouseTracking(true);
}

void WaveformSeekBar::setWaveformData(const QVector<float>& data) {
    waveform_ = data;
    update();
}

void WaveformSeekBar::setPosition(double ratio) {
    if (!seeking_) {
        position_ = std::clamp(ratio, 0.0, 1.0);
        update();
    }
}

QVector<float> WaveformSeekBar::cachedWaveform(const QString& path) const {
    auto it = waveformCache_.find(path);
    return it != waveformCache_.end() ? it.value() : QVector<float>();
}

void WaveformSeekBar::cacheWaveform(const QString& path, const QVector<float>& data) {
    if (waveformCache_.size() >= kMaxCacheEntries) {
        waveformCache_.erase(waveformCache_.begin());
    }
    waveformCache_.insert(path, data);
}

void WaveformSeekBar::generateAsync(const QString& path) {
    QVector<float> cached = cachedWaveform(path);
    if (!cached.isEmpty()) {
        setWaveformData(cached);
        return;
    }

    auto future = QtConcurrent::run(QThreadPool::globalInstance(), [path]() {
        auto decoder = DecoderFactory::createForFile(path);
        if (!decoder || !decoder->open(path)) return QVector<float>();

        int64_t total = decoder->totalFrames();
        int ch = decoder->channels();
        constexpr int kPoints = 2000;
        int64_t blockSize = std::max<int64_t>(1, total / kPoints);
        int64_t blocks = (total + blockSize - 1) / blockSize;
        QVector<float> data(static_cast<int>(blocks));

        std::vector<float> buf(4096 * ch);
        int pointIdx = 0;
        double sumSq = 0.0;
        int count = 0;

        while (true) {
            int64_t read = decoder->decode(buf.data(), 4096);
            if (read <= 0) break;
            for (int64_t i = 0; i < read * ch; ++i) {
                sumSq += buf[i] * buf[i];
                count++;
                if ((count) % blockSize == 0 && pointIdx < data.size()) {
                    data[pointIdx++] = static_cast<float>(std::sqrt(sumSq / count));
                    sumSq = 0.0;
                    count = 0;
                }
            }
        }
        decoder->close();

        if (pointIdx < data.size()) {
            data.resize(pointIdx);
        }
        return data;
    });

    auto* watcher = new QFutureWatcher<QVector<float>>(this);
    connect(watcher, &QFutureWatcher<QVector<float>>::finished, this, [this, watcher, path]() {
        QVector<float> data = watcher->result();
        if (!data.isEmpty()) {
            cacheWaveform(path, data);
        }
        setWaveformData(data);
        watcher->deleteLater();
    });
    watcher->setFuture(future);
}

void WaveformSeekBar::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int mid = h / 2;

    if (waveform_.isEmpty()) {
        p.setPen(Theme::TEXT_SEC);
        p.setFont(Theme::monoFont());
        p.drawText(rect(), Qt::AlignCenter, "Generating waveform...");
        return;
    }

    int playedX = static_cast<int>(position_ * w);
    int count = waveform_.size();
    float halfH = mid - 2.0f;

    constexpr float kBarW = 2.0f;
    constexpr float kGap = 1.0f;
    float step = kBarW + kGap;

    p.setPen(Qt::NoPen);
    for (int i = 0; i < count; ++i) {
        float val = std::clamp(waveform_[i], 0.0f, 1.0f);
        float x = i * step;
        if (x > w) break;
        float barH = val * halfH;

        p.setBrush((x <= playedX) ? Theme::ACCENT : Theme::CARD_HOVER);
        p.drawRect(QRectF(x, mid - barH, kBarW, barH * 2.0f));
    }

    p.setPen(QPen(Theme::TEXT_PRI, 1.0f));
    p.drawLine(QPointF(playedX, 0.0f), QPointF(playedX, h));
}

void WaveformSeekBar::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        seeking_ = true;
        double ratio = std::clamp(static_cast<double>(event->position().x()) / width(), 0.0, 1.0);
        position_ = ratio;
        emit seekRequested(ratio);
        update();
    }
}

void WaveformSeekBar::mouseMoveEvent(QMouseEvent* event) {
    if (seeking_ && event->buttons() & Qt::LeftButton) {
        double ratio = std::clamp(static_cast<double>(event->position().x()) / width(), 0.0, 1.0);
        position_ = ratio;
        emit seekRequested(ratio);
        update();
    }
}
