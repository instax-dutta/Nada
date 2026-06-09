#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include "library/TrackMetadata.h"

class WaveformSeekBar;
class SlimSlider;
class PlayingIndicator;

class NowPlayingView : public QWidget {
    Q_OBJECT
public:
    explicit NowPlayingView(QWidget* parent = nullptr);

    void setTrack(const TrackMetadata& track);
    void setPosition(double ratio);
    void setDuration(int64_t ms);
    void setVolume(float linear);
    void setState(bool playing, bool shuffle, bool repeat);

    WaveformSeekBar* seekBar() const { return seekBar_; }

signals:
    void playPauseClicked();
    void prevClicked();
    void nextClicked();
    void shuffleClicked();
    void repeatClicked();
    void volumeChanged(float linear);

private:
    void setupUi();
    QString formatTime(int64_t ms) const;
    void updateTimeLabels();
    void updateIndicator();

    QLabel* coverArt_ = nullptr;
    QLabel* titleLabel_ = nullptr;
    QLabel* artistAlbumLabel_ = nullptr;
    QLabel* formatBadge_ = nullptr;
    WaveformSeekBar* seekBar_ = nullptr;
    QLabel* timeElapsed_ = nullptr;
    QLabel* timeRemaining_ = nullptr;

    QToolButton* shuffleBtn_ = nullptr;
    QPushButton* prevBtn_ = nullptr;
    QPushButton* playPauseBtn_ = nullptr;
    QPushButton* nextBtn_ = nullptr;
    QToolButton* repeatBtn_ = nullptr;

    SlimSlider* volumeSlider_ = nullptr;
    PlayingIndicator* indicator_ = nullptr;

    TrackMetadata currentTrack_;
    int64_t durationMs_ = 0;
    double positionRatio_ = 0.0;
    bool playing_ = false;
};
