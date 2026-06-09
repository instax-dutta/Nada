#include "NowPlayingView.h"
#include "WaveformSeekBar.h"
#include "SlimSlider.h"
#include "PlayingIndicator.h"
#include "Theme.h"
#include "library/CoverArtCache.h"
#include "library/TagReader.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>
#include <QFileInfo>
#include <QIcon>
#include <QFontMetrics>
#include <QGraphicsDropShadowEffect>
#include <cmath>

static QIcon svgIcon(const QString& path) {
    return QIcon(path);
}

NowPlayingView::NowPlayingView(QWidget* parent)
    : QWidget(parent)
{
    setFixedWidth(224);
    setupUi();
}

void NowPlayingView::setupUi() {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(16, 24, 16, 8);
    lay->setSpacing(4);
    lay->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    coverArt_ = new QLabel(this);
    coverArt_->setFixedSize(192, 192);
    coverArt_->setAlignment(Qt::AlignCenter);
    coverArt_->setStyleSheet("background-color: #111111; border-radius: 12px;");
    auto* shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(16);
    shadow->setColor(QColor(0, 0, 0, 140));
    shadow->setOffset(0, 3);
    coverArt_->setGraphicsEffect(shadow);
    lay->addWidget(coverArt_, 0, Qt::AlignCenter);

    indicator_ = new PlayingIndicator(this);
    lay->addWidget(indicator_, 0, Qt::AlignCenter);
    indicator_->setVisible(false);

    titleLabel_ = new QLabel(this);
    titleLabel_->setFont(Theme::titleFont());
    titleLabel_->setStyleSheet("color: #F5F5F7;");
    titleLabel_->setAlignment(Qt::AlignCenter);
    titleLabel_->setWordWrap(false);
    titleLabel_->setFixedWidth(190);
    lay->addWidget(titleLabel_);

    artistAlbumLabel_ = new QLabel(this);
    artistAlbumLabel_->setFont(Theme::uiFont());
    artistAlbumLabel_->setStyleSheet("color: #86868B;");
    artistAlbumLabel_->setAlignment(Qt::AlignCenter);
    lay->addWidget(artistAlbumLabel_);

    formatBadge_ = new QLabel(this);
    formatBadge_->setFont(Theme::monoFont());
    formatBadge_->setAlignment(Qt::AlignCenter);
    formatBadge_->setStyleSheet(
        "color: #D4B982; background-color: #161616; "
        "border-radius: 2px; padding: 2px 8px; font-size: 10px;");
    formatBadge_->setFixedHeight(18);
    lay->addWidget(formatBadge_, 0, Qt::AlignCenter);

    seekBar_ = new WaveformSeekBar(this);
    seekBar_->setFixedHeight(48);
    lay->addWidget(seekBar_);

    auto* timeRow = new QHBoxLayout();
    timeElapsed_ = new QLabel("0:00", this);
    timeElapsed_->setFont(Theme::monoFont());
    timeElapsed_->setStyleSheet("color: #86868B; font-size: 10px;");
    timeRemaining_ = new QLabel("-0:00", this);
    timeRemaining_->setFont(Theme::monoFont());
    timeRemaining_->setStyleSheet("color: #86868B; font-size: 10px;");
    timeRemaining_->setAlignment(Qt::AlignRight);
    timeRow->addWidget(timeElapsed_);
    timeRow->addStretch();
    timeRow->addWidget(timeRemaining_);
    lay->addLayout(timeRow);

    auto* transportRow = new QHBoxLayout();
    transportRow->setSpacing(6);
    transportRow->setAlignment(Qt::AlignCenter);

    shuffleBtn_ = new QToolButton(this);
    shuffleBtn_->setIcon(svgIcon(":/icons/shuffle.svg"));
    shuffleBtn_->setIconSize(QSize(16, 16));
    shuffleBtn_->setCheckable(true);
    shuffleBtn_->setFixedSize(36, 36);
    shuffleBtn_->setStyleSheet(
        "QToolButton { background: transparent; border: none; border-radius: 8px; }"
        "QToolButton:checked { background-color: #221C12; }"
        "QToolButton:hover { background-color: #1E1E1E; }");
    transportRow->addWidget(shuffleBtn_);
    connect(shuffleBtn_, &QToolButton::clicked, this, &NowPlayingView::shuffleClicked);

    prevBtn_ = new QPushButton(this);
    prevBtn_->setIcon(svgIcon(":/icons/prev.svg"));
    prevBtn_->setIconSize(QSize(16, 16));
    prevBtn_->setFixedSize(36, 36);
    prevBtn_->setStyleSheet(
        "QPushButton { background: transparent; border: none; border-radius: 8px; }"
        "QPushButton:hover { background-color: #1E1E1E; }");
    transportRow->addWidget(prevBtn_);
    connect(prevBtn_, &QPushButton::clicked, this, &NowPlayingView::prevClicked);

    playPauseBtn_ = new QPushButton(this);
    playPauseBtn_->setIcon(svgIcon(":/icons/play.svg"));
    playPauseBtn_->setIconSize(QSize(20, 20));
    playPauseBtn_->setFixedSize(44, 44);
    playPauseBtn_->setStyleSheet(
        "QPushButton { background-color: #D4B982; border: none; border-radius: 22px; }"
        "QPushButton:hover { background-color: #DCC396; }"
        "QPushButton:pressed { background-color: #C4A66E; }");
    transportRow->addWidget(playPauseBtn_);
    connect(playPauseBtn_, &QPushButton::clicked, this, &NowPlayingView::playPauseClicked);

    nextBtn_ = new QPushButton(this);
    nextBtn_->setIcon(svgIcon(":/icons/next.svg"));
    nextBtn_->setIconSize(QSize(16, 16));
    nextBtn_->setFixedSize(36, 36);
    nextBtn_->setStyleSheet(
        "QPushButton { background: transparent; border: none; border-radius: 8px; }"
        "QPushButton:hover { background-color: #1E1E1E; }");
    transportRow->addWidget(nextBtn_);
    connect(nextBtn_, &QPushButton::clicked, this, &NowPlayingView::nextClicked);

    repeatBtn_ = new QToolButton(this);
    repeatBtn_->setIcon(svgIcon(":/icons/repeat.svg"));
    repeatBtn_->setIconSize(QSize(16, 16));
    repeatBtn_->setCheckable(true);
    repeatBtn_->setFixedSize(36, 36);
    repeatBtn_->setStyleSheet(
        "QToolButton { background: transparent; border: none; border-radius: 8px; }"
        "QToolButton:checked { background-color: #221C12; }"
        "QToolButton:hover { background-color: #1E1E1E; }");
    transportRow->addWidget(repeatBtn_);
    connect(repeatBtn_, &QToolButton::clicked, this, &NowPlayingView::repeatClicked);

    lay->addLayout(transportRow);

    auto* volRow = new QHBoxLayout();
    volRow->setContentsMargins(4, 0, 4, 0);
    auto* volIcon = new QLabel(this);
    volIcon->setPixmap(QIcon(":/icons/volume.svg").pixmap(14, 14));
    volRow->addWidget(volIcon);
    volumeSlider_ = new SlimSlider(Qt::Horizontal, this);
    volumeSlider_->setRange(0, 100);
    volumeSlider_->setValue(100);
    volRow->addWidget(volumeSlider_, 1);
    connect(volumeSlider_, &SlimSlider::valueChanged, this, [this](int v) {
        emit volumeChanged(v / 100.0f);
    });
    lay->addLayout(volRow);
}

void NowPlayingView::setTrack(const TrackMetadata& track) {
    currentTrack_ = track;
    durationMs_ = track.durationMs;
    positionRatio_ = 0.0;

    QFontMetrics fm(titleLabel_->font());
    titleLabel_->setText(fm.elidedText(track.displayTitle(), Qt::ElideRight, 190));
    QString albumStr = track.album.isEmpty() ? "" : " \u00B7 " + track.album;
    artistAlbumLabel_->setText(track.displayArtist() + albumStr);

    QString formatStr = track.format;
    if (track.bitDepth > 0)
        formatStr += QString(" %1bit").arg(track.bitDepth);
    if (track.sampleRate > 0)
        formatStr += QString(" \u00B7 %1kHz").arg(track.sampleRate / 1000.0, 0, 'f', 0);
    formatBadge_->setText(formatStr);

    CoverArtCache cache;
    QPixmap cover;
    if (!track.coverArtHash.isEmpty())
        cover = cache.get(track.coverArtHash);
    if (!cover.isNull()) {
        coverArt_->setPixmap(cover.scaled(192, 192, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        QByteArray raw = TagReader::readEmbeddedCover(track.path);
        if (!raw.isEmpty()) {
            QString hash = cache.store(raw);
            cover = cache.get(hash);
            if (!cover.isNull())
                coverArt_->setPixmap(cover.scaled(192, 192, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }

    seekBar_->generateAsync(track.path);
    updateTimeLabels();
}

void NowPlayingView::setPosition(double ratio) {
    positionRatio_ = ratio;
    seekBar_->setPosition(ratio);
    updateTimeLabels();
}

void NowPlayingView::setDuration(int64_t ms) {
    durationMs_ = ms;
    updateTimeLabels();
}

void NowPlayingView::setVolume(float linear) {
    volumeSlider_->setValue(static_cast<int>(linear * 100));
}

void NowPlayingView::setState(bool playing, bool shuffle, bool repeat) {
    playing_ = playing;
    playPauseBtn_->setIcon(svgIcon(playing ? ":/icons/pause.svg" : ":/icons/play.svg"));
    shuffleBtn_->setChecked(shuffle);
    repeatBtn_->setChecked(repeat);
    updateIndicator();
}

void NowPlayingView::updateIndicator() {
    indicator_->setVisible(playing_);
    if (playing_) indicator_->startAnimation();
    else indicator_->stopAnimation();
}

void NowPlayingView::updateTimeLabels() {
    int64_t elapsed = static_cast<int64_t>(positionRatio_ * durationMs_);
    timeElapsed_->setText(formatTime(elapsed));
    timeRemaining_->setText("-" + formatTime(durationMs_ - elapsed));
}

QString NowPlayingView::formatTime(int64_t ms) const {
    int totalSec = static_cast<int>(ms / 1000);
    int min = totalSec / 60;
    int sec = totalSec % 60;
    return QStringLiteral("%1:%2").arg(min).arg(sec, 2, 10, QLatin1Char('0'));
}
