#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QToolButton>
#include <QLabel>
#include <QFutureWatcher>
#include <QProgressBar>
#include "library/TrackMetadata.h"
#include "engine/PlaybackEngine.h"
#include <memory>

class PlaybackEngine;
class LibraryManager;
class LibraryModel;
class LibraryView;
class NowPlayingView;
class QueueView;
class EQView;
class SettingsView;
class SystemTray;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onTrackActivated(const TrackMetadata& track);
    void onPlayPause();
    void onPrev();
    void onNext();
    void onPositionChanged(double ratio);
    void onDurationChanged(int64_t ms);
    void onTrackFinished();
    void onLibraryChanged();
    void onLibraryLoaded();
    void onVolumeChanged(float linear);
    void navigateTo(int pageIndex);

private:
    void setupUi();
    void setupSidebar();
    void setupContent();
    void setupConnections();
    void setupShortcuts();
    void saveGeometry();
    void restoreGeometry();

    void setNavButtonActive(int index);

    PlaybackEngine* engine_ = nullptr;
    LibraryManager* library_ = nullptr;
    LibraryModel* libraryModel_ = nullptr;
    SystemTray* tray_ = nullptr;

    QWidget* sidebar_ = nullptr;
    QToolButton* libBtn_ = nullptr;
    QToolButton* queueBtn_ = nullptr;
    QToolButton* eqBtn_ = nullptr;
    QToolButton* settingsBtn_ = nullptr;
    int currentNav_ = 0;

    QStackedWidget* content_ = nullptr;
    LibraryView* libraryView_ = nullptr;
    NowPlayingView* nowPlayingView_ = nullptr;
    QueueView* queueView_ = nullptr;
    EQView* eqView_ = nullptr;
    SettingsView* settingsView_ = nullptr;

    QWidget* rightPanel_ = nullptr;

    QList<TrackMetadata> queue_;
    int queueIndex_ = -1;

    QWidget* spinnerPage_ = nullptr;
    QLabel* spinnerLabel_ = nullptr;
    QProgressBar* spinnerBar_ = nullptr;
    QFutureWatcher<QList<TrackMetadata>>* libWatcher_ = nullptr;
};
