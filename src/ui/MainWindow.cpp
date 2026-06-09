#include "MainWindow.h"
#include "Theme.h"
#include "LibraryModel.h"
#include "LibraryView.h"
#include "NowPlayingView.h"
#include "QueueView.h"
#include "EQView.h"
#include "SettingsView.h"
#include "SystemTray.h"
#include "WaveformSeekBar.h"
#include "engine/PlaybackEngine.h"
#include "library/LibraryManager.h"
#include "library/CoverArtCache.h"
#include "library/TagReader.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QSettings>
#include <QStandardPaths>
#include <algorithm>
#include <QDir>
#include <QFileInfo>
#include <QMimeData>
#include <QShortcut>
#include <QApplication>
#include <QIcon>
#include <QStackedWidget>
#include <QtConcurrent/QtConcurrentRun>

class AdaptiveStack : public QStackedWidget {
public:
    using QStackedWidget::QStackedWidget;
    QSize sizeHint() const override {
        return currentWidget() ? currentWidget()->sizeHint()
                               : QStackedWidget::sizeHint();
    }
    QSize minimumSizeHint() const override {
        return currentWidget() ? currentWidget()->minimumSizeHint()
                               : QStackedWidget::minimumSizeHint();
    }
};

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Nada");
    resize(1280, 800);
    setAcceptDrops(true);
    setPalette(Theme::darkPalette());

    engine_ = new PlaybackEngine(this);
    library_ = new LibraryManager(this);

    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                     + "/nada/library.db";
    library_->initialize(dbPath);

    libraryModel_ = new LibraryModel(this);

    setupUi();
    setupConnections();
    setupShortcuts();

    connect(library_, &LibraryManager::libraryChanged, this, &MainWindow::onLibraryChanged);

    libWatcher_ = new QFutureWatcher<QList<TrackMetadata>>(this);
    connect(libWatcher_, &QFutureWatcher<QList<TrackMetadata>>::finished, this, &MainWindow::onLibraryLoaded);
    QFuture<QList<TrackMetadata>> future = QtConcurrent::run(&LibraryManager::getAllTracks, library_);
    libWatcher_->setFuture(future);

    tray_ = new SystemTray(this);
    connect(tray_, &SystemTray::showWindow, this, [this]() {
        show();
        raise();
        activateWindow();
    });
    connect(tray_, &SystemTray::quit, qApp, &QApplication::quit);
    connect(tray_, &SystemTray::playPause, this, &MainWindow::onPlayPause);
    connect(tray_, &SystemTray::nextTrack, this, &MainWindow::onNext);
    connect(tray_, &SystemTray::prevTrack, this, &MainWindow::onPrev);
    tray_->show();

    restoreGeometry();
    navigateTo(0);
}

MainWindow::~MainWindow() {
    saveGeometry();
}

void MainWindow::setupUi() {
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* hbox = new QHBoxLayout(central);
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(0);

    setupSidebar();
    hbox->addWidget(sidebar_);

    setupContent();
    hbox->addWidget(content_, 1);

    rightPanel_ = new QWidget(this);
    rightPanel_->setFixedWidth(224);
    auto* rpLay = new QVBoxLayout(rightPanel_);
    rpLay->setContentsMargins(0, 0, 0, 0);
    rpLay->setSpacing(0);
    nowPlayingView_ = new NowPlayingView(this);
    rpLay->addWidget(nowPlayingView_);
    hbox->addWidget(rightPanel_);
}

void MainWindow::setupSidebar() {
    sidebar_ = new QWidget(this);
    sidebar_->setFixedWidth(56);
    sidebar_->setStyleSheet(
        "background-color: #111111;"
        "border-right: 0.5px solid rgba(30,30,30,0.5);");

    auto* vlay = new QVBoxLayout(sidebar_);
    vlay->setContentsMargins(8, 12, 8, 12);
    vlay->setSpacing(8);

    auto* logo = new QLabel("N", sidebar_);
    logo->setFixedSize(32, 32);
    logo->setAlignment(Qt::AlignCenter);
    logo->setStyleSheet(
        "background-color: #D4B982; color: #0A0A0A; "
        "font-size: 18px; font-weight: bold; border-radius: 8px;");
    vlay->addWidget(logo, 0, Qt::AlignCenter);

    auto makeNavBtn = [&](const QString& iconPath) -> QToolButton* {
        auto* btn = new QToolButton(sidebar_);
        btn->setIcon(QIcon(iconPath));
        btn->setIconSize(QSize(20, 20));
        btn->setFixedSize(40, 40);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setStyleSheet(
            "QToolButton { background: transparent; border: none; border-radius: 10px; }"
            "QToolButton:hover { background-color: #1a1a1a; }");
        return btn;
    };

    libBtn_ = makeNavBtn(":/icons/library.svg");
    connect(libBtn_, &QToolButton::clicked, this, [this]() { navigateTo(0); });
    vlay->addWidget(libBtn_, 0, Qt::AlignCenter);

    queueBtn_ = makeNavBtn(":/icons/queue.svg");
    connect(queueBtn_, &QToolButton::clicked, this, [this]() { navigateTo(1); });
    vlay->addWidget(queueBtn_, 0, Qt::AlignCenter);

    eqBtn_ = makeNavBtn(":/icons/eq.svg");
    connect(eqBtn_, &QToolButton::clicked, this, [this]() { navigateTo(2); });
    vlay->addWidget(eqBtn_, 0, Qt::AlignCenter);

    vlay->addStretch();

    settingsBtn_ = makeNavBtn(":/icons/settings.svg");
    connect(settingsBtn_, &QToolButton::clicked, this, [this]() { navigateTo(3); });
    vlay->addWidget(settingsBtn_, 0, Qt::AlignCenter);
}

void MainWindow::setupContent() {
    content_ = new AdaptiveStack(this);

    spinnerPage_ = new QWidget(this);
    auto* spLay = new QVBoxLayout(spinnerPage_);
    spinnerLabel_ = new QLabel("Loading library...", spinnerPage_);
    spinnerLabel_->setAlignment(Qt::AlignCenter);
    spinnerLabel_->setStyleSheet("color: #666666; font-size: 18px;");
    spinnerBar_ = new QProgressBar(spinnerPage_);
    spinnerBar_->setRange(0, 0);
    spinnerBar_->setFixedWidth(300);
    spinnerBar_->setTextVisible(false);
    spLay->addStretch();
    spLay->addWidget(spinnerLabel_, 0, Qt::AlignCenter);
    spLay->addWidget(spinnerBar_, 0, Qt::AlignCenter);
    spLay->addStretch();
    content_->addWidget(spinnerPage_);

    libraryView_ = new LibraryView(this);
    libraryView_->setModel(libraryModel_);
    content_->addWidget(libraryView_);

    queueView_ = new QueueView(this);
    content_->addWidget(queueView_);

    eqView_ = new EQView(engine_->dspChain(), this);
    content_->addWidget(eqView_);

    settingsView_ = new SettingsView(library_, this);
    content_->addWidget(settingsView_);

    content_->setCurrentIndex(0);
}

void MainWindow::setNavButtonActive(int index) {
    QToolButton* btns[] = {libBtn_, queueBtn_, eqBtn_, settingsBtn_};
    for (int i = 0; i < 4; ++i) {
        btns[i]->setStyleSheet(
            i == index
                ? "QToolButton { background-color: #221C12; border: none; border-radius: 6px; }"
                  "QToolButton:hover { background-color: #221C12; }"
                : "QToolButton { background: transparent; border: none; border-radius: 6px; }"
                  "QToolButton:hover { background-color: #1E1E1E; }");
    }
    currentNav_ = index;
}

void MainWindow::setupConnections() {
    connect(libraryView_, &LibraryView::trackActivated, this, &MainWindow::onTrackActivated);
    connect(libraryView_, &LibraryView::trackSelected, this, [this](const TrackMetadata& track) {
        nowPlayingView_->setTrack(track);
    });

    connect(engine_, &PlaybackEngine::positionChanged, this, &MainWindow::onPositionChanged);
    connect(engine_, &PlaybackEngine::durationChanged, this, &MainWindow::onDurationChanged);
    connect(engine_, &PlaybackEngine::trackFinished, this, &MainWindow::onTrackFinished);
    connect(engine_, &PlaybackEngine::stateChanged, this, [this](PlaybackEngine::State state) {
        bool playing = state == PlaybackEngine::State::Playing;
        nowPlayingView_->setState(playing, false, false);
        tray_->setPlaying(playing);
    });

    connect(nowPlayingView_, &NowPlayingView::playPauseClicked, this, &MainWindow::onPlayPause);
    connect(nowPlayingView_, &NowPlayingView::prevClicked, this, &MainWindow::onPrev);
    connect(nowPlayingView_, &NowPlayingView::nextClicked, this, &MainWindow::onNext);
    connect(nowPlayingView_, &NowPlayingView::volumeChanged, this, &MainWindow::onVolumeChanged);
    connect(nowPlayingView_->seekBar(), &WaveformSeekBar::seekRequested, engine_, &PlaybackEngine::seek);
}

void MainWindow::setupShortcuts() {
    auto* space = new QShortcut(QKeySequence(Qt::Key_Space), this);
    connect(space, &QShortcut::activated, this, &MainWindow::onPlayPause);

    auto* left = new QShortcut(QKeySequence(Qt::Key_Left), this);
    connect(left, &QShortcut::activated, this, [this]() {
        engine_->seek(std::max(0.0, nowPlayingView_->seekBar()->position() - 5.0 / 60.0));
    });

    auto* right = new QShortcut(QKeySequence(Qt::Key_Right), this);
    connect(right, &QShortcut::activated, this, [this]() {
        engine_->seek(std::min(1.0, nowPlayingView_->seekBar()->position() + 5.0 / 60.0));
    });

    auto* n = new QShortcut(QKeySequence(Qt::Key_N), this);
    connect(n, &QShortcut::activated, this, &MainWindow::onNext);

    auto* p = new QShortcut(QKeySequence(Qt::Key_P), this);
    connect(p, &QShortcut::activated, this, &MainWindow::onPrev);

    auto* ctrlF = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_F), this);
    connect(ctrlF, &QShortcut::activated, this, [this]() {
        navigateTo(0);
    });

    auto* ctrlQ = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), this);
    connect(ctrlQ, &QShortcut::activated, qApp, &QApplication::quit);

    auto* f11 = new QShortcut(QKeySequence(Qt::Key_F11), this);
    connect(f11, &QShortcut::activated, this, [this]() {
        if (isFullScreen()) showNormal(); else showFullScreen();
    });
}

void MainWindow::navigateTo(int pageIndex) {
    setNavButtonActive(pageIndex);
    int contentIndex = pageIndex + 1;
    if (contentIndex < content_->count())
        content_->setCurrentIndex(contentIndex);
}

void MainWindow::onTrackActivated(const TrackMetadata& track) {
    queue_.clear();
    queueIndex_ = 0;
    queue_.append(track);
    engine_->play(track.path);
    nowPlayingView_->setTrack(track);
}

void MainWindow::onPlayPause() {
    auto state = engine_->state();
    if (state == PlaybackEngine::State::Playing) engine_->pause();
    else if (state == PlaybackEngine::State::Paused) engine_->resume();
    else if (!queue_.isEmpty()) engine_->play(queue_[queueIndex_].path);
}

void MainWindow::onPrev() {
    if (queue_.isEmpty()) return;
    queueIndex_ = (queueIndex_ - 1 + queue_.size()) % queue_.size();
    engine_->play(queue_[queueIndex_].path);
    nowPlayingView_->setTrack(queue_[queueIndex_]);
}

void MainWindow::onNext() {
    if (queue_.isEmpty()) return;
    queueIndex_ = (queueIndex_ + 1) % queue_.size();
    engine_->play(queue_[queueIndex_].path);
    nowPlayingView_->setTrack(queue_[queueIndex_]);
}

void MainWindow::onPositionChanged(double ratio) {
    nowPlayingView_->setPosition(ratio);
}

void MainWindow::onDurationChanged(int64_t ms) {
    nowPlayingView_->setDuration(ms);
}

void MainWindow::onTrackFinished() {
    if (queue_.size() > 1) onNext();
}

void MainWindow::onVolumeChanged(float linear) {
    engine_->setVolume(linear);
    nowPlayingView_->setVolume(linear);
}

void MainWindow::onLibraryChanged() {
    libraryModel_->setTracks(library_->getAllTracks());
}

void MainWindow::closeEvent(QCloseEvent* event) {
    saveGeometry();
    hide();
    event->ignore();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent* event) {
    for (const auto& url : event->mimeData()->urls()) {
        QString path = url.toLocalFile();
        if (QFileInfo::exists(path)) {
            TrackMetadata meta;
            meta.path = path;
            meta.title = QFileInfo(path).baseName();
            queue_.append(meta);
            queueView_->addToQueue(meta);
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    QMainWindow::keyPressEvent(event);
}

void MainWindow::saveGeometry() {
    QSettings settings;
    settings.setValue("Nada/UI/geometry", QMainWindow::saveGeometry());
    settings.setValue("Nada/UI/windowState", QMainWindow::saveState());
}

void MainWindow::restoreGeometry() {
    QSettings settings;
    auto geom = settings.value("Nada/UI/geometry").toByteArray();
    if (!geom.isEmpty()) QMainWindow::restoreGeometry(geom);
    auto state = settings.value("Nada/UI/windowState").toByteArray();
    if (!state.isEmpty()) QMainWindow::restoreState(state);
}

void MainWindow::onLibraryLoaded() {
    auto tracks = libWatcher_->result();
    libraryModel_->setTracks(tracks);
    content_->setCurrentIndex(1);
    navigateTo(0);
}
