#pragma once

#include <QWidget>
#include <QVector>
#include <QString>
#include <QHash>

class WaveformSeekBar : public QWidget {
    Q_OBJECT
public:
    explicit WaveformSeekBar(QWidget* parent = nullptr);

    void setWaveformData(const QVector<float>& data);
    void setPosition(double ratio);
    double position() const { return position_; }
    void generateAsync(const QString& path);

signals:
    void seekRequested(double ratio);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QVector<float> waveform_;
    double position_ = 0.0;
    bool seeking_ = false;

    QHash<QString, QVector<float>> waveformCache_;
    static constexpr int kMaxCacheEntries = 200;
    QVector<float> cachedWaveform(const QString& path) const;
    void cacheWaveform(const QString& path, const QVector<float>& data);
};
