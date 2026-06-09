#pragma once

#include <QWidget>
#include <QComboBox>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>

class LibraryManager;
class ToggleSwitch;
class SlimSlider;

class SettingsView : public QWidget {
    Q_OBJECT
public:
    explicit SettingsView(LibraryManager* library, QWidget* parent = nullptr);

signals:
    void rescanRequested();

private slots:
    void onWatchFolderAdded();
    void onWatchFolderRemoved();
    void onRescanClicked();
    void onLibraryScanStarted();
    void onLibraryScanProgress(int done, int total);
    void onLibraryScanFinished(int added, int updated, int removed);

private:
    void setupUi();
    void loadSettings();
    void saveSetting(const QString& key, const QVariant& value);

    LibraryManager* library_;

    QListWidget* watchFolders_;
    QPushButton* addFolderBtn_;
    QPushButton* removeFolderBtn_;
    QPushButton* rescanBtn_;
    QProgressBar* scanProgress_;
    QLabel* scanStats_;

    QComboBox* bufferSize_;
    QComboBox* sampleRate_;

    ToggleSwitch* gapless_;
    SlimSlider* crossfadeDuration_;
    QLabel* crossfadeLabel_;
    QComboBox* rgMode_;

    ToggleSwitch* minimizeToTray_;
    ToggleSwitch* showVisualizer_;
    ToggleSwitch* showWaveform_;
};
