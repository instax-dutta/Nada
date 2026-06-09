#include "SettingsView.h"
#include "ToggleSwitch.h"
#include "SlimSlider.h"
#include "Theme.h"
#include "library/LibraryManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QFileDialog>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QScrollArea>

SettingsView::SettingsView(LibraryManager* library, QWidget* parent)
    : QWidget(parent)
    , library_(library)
{
    setupUi();
    loadSettings();

    connect(library_, &LibraryManager::scanStarted, this, &SettingsView::onLibraryScanStarted);
    connect(library_, &LibraryManager::scanProgress, this, &SettingsView::onLibraryScanProgress);
    connect(library_, &LibraryManager::scanFinished, this, &SettingsView::onLibraryScanFinished);
}

void SettingsView::setupUi() {
    auto* scrollContent = new QWidget(this);
    auto* mainLay = new QVBoxLayout(scrollContent);
    mainLay->setSpacing(16);

    auto* audioGroup = new QGroupBox("Audio", this);
    auto* audioForm = new QFormLayout(audioGroup);
    bufferSize_ = new QComboBox(this);
    bufferSize_->addItems({"256", "512", "1024", "2048"});
    audioForm->addRow("Buffer Size:", bufferSize_);

    sampleRate_ = new QComboBox(this);
    sampleRate_->addItems({"Auto", "44100", "48000", "88200", "96000", "176400", "192000"});
    audioForm->addRow("Sample Rate:", sampleRate_);
    mainLay->addWidget(audioGroup);

    auto* libGroup = new QGroupBox("Library", this);
    auto* libLay = new QVBoxLayout(libGroup);

    watchFolders_ = new QListWidget(this);
    libLay->addWidget(watchFolders_);

    auto* folderBtnRow = new QHBoxLayout();
    addFolderBtn_ = new QPushButton("Add Folder", this);
    removeFolderBtn_ = new QPushButton("Remove", this);
    folderBtnRow->addWidget(addFolderBtn_);
    folderBtnRow->addWidget(removeFolderBtn_);
    folderBtnRow->addStretch();
    libLay->addLayout(folderBtnRow);

    auto* scanRow = new QHBoxLayout();
    rescanBtn_ = new QPushButton("Rescan Library", this);
    scanProgress_ = new QProgressBar(this);
    scanProgress_->setVisible(false);
    scanRow->addWidget(rescanBtn_);
    scanRow->addWidget(scanProgress_, 1);
    libLay->addLayout(scanRow);

    scanStats_ = new QLabel(this);
    scanStats_->setStyleSheet("color: #888888;");
    libLay->addWidget(scanStats_);

    mainLay->addWidget(libGroup);

    auto* playbackGroup = new QGroupBox("Playback", this);
    auto* playbackForm = new QFormLayout(playbackGroup);

    gapless_ = new ToggleSwitch(this);
    playbackForm->addRow("Gapless playback:", gapless_);

    auto* cfRow = new QHBoxLayout();
    crossfadeDuration_ = new SlimSlider(Qt::Horizontal, this);
    crossfadeDuration_->setRange(0, 60);
    crossfadeLabel_ = new QLabel("0s", this);
    cfRow->addWidget(crossfadeDuration_);
    cfRow->addWidget(crossfadeLabel_);
    playbackForm->addRow("Crossfade:", cfRow);

    rgMode_ = new QComboBox(this);
    rgMode_->addItems({"Off", "Track", "Album"});
    playbackForm->addRow("ReplayGain:", rgMode_);

    mainLay->addWidget(playbackGroup);

    auto* uiGroup = new QGroupBox("Interface", this);
    auto* uiForm = new QFormLayout(uiGroup);

    minimizeToTray_ = new ToggleSwitch(this);
    uiForm->addRow("Minimize to tray:", minimizeToTray_);
    showVisualizer_ = new ToggleSwitch(this);
    uiForm->addRow("Show visualizer:", showVisualizer_);
    showWaveform_ = new ToggleSwitch(this);
    uiForm->addRow("Show waveform:", showWaveform_);

    mainLay->addWidget(uiGroup);
    mainLay->addStretch();

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidget(scrollContent);
    scrollArea->setWidgetResizable(true);

    auto* outerLay = new QVBoxLayout(this);
    outerLay->setContentsMargins(0, 0, 0, 0);
    outerLay->addWidget(scrollArea);

    connect(addFolderBtn_, &QPushButton::clicked, this, &SettingsView::onWatchFolderAdded);
    connect(removeFolderBtn_, &QPushButton::clicked, this, &SettingsView::onWatchFolderRemoved);
    connect(rescanBtn_, &QPushButton::clicked, this, &SettingsView::onRescanClicked);

    connect(crossfadeDuration_, &SlimSlider::valueChanged, this, [this](int v) {
        crossfadeLabel_->setText(QString("%1s").arg(v / 10.0, 0, 'f', 1));
        saveSetting("Playback/crossfadeDuration", v);
    });
    connect(minimizeToTray_, &ToggleSwitch::clicked, this, [this](bool v) {
        saveSetting("UI/minimizeToTray", v);
    });
    connect(showVisualizer_, &ToggleSwitch::clicked, this, [this](bool v) {
        saveSetting("UI/showVisualizer", v);
    });
    connect(showWaveform_, &ToggleSwitch::clicked, this, [this](bool v) {
        saveSetting("UI/showWaveform", v);
    });
}

void SettingsView::loadSettings() {
    QSettings s;
    s.beginGroup("Nada");

    s.beginGroup("Audio");
    bufferSize_->setCurrentText(s.value("bufferSize", "1024").toString());
    sampleRate_->setCurrentText(s.value("sampleRate", "Auto").toString());
    s.endGroup();

    s.beginGroup("Library");
    QStringList folders = s.value("watchFolders").toStringList();
    for (const auto& f : folders) watchFolders_->addItem(f);
    int totalTracks = s.value("totalTracks", 0).toInt();
    int totalAlbums = s.value("totalAlbums", 0).toInt();
    int totalHours = s.value("totalHours", 0).toInt();
    scanStats_->setText(QString("%1 tracks \u00B7 %2 albums \u00B7 %3 hours")
                            .arg(totalTracks).arg(totalAlbums).arg(totalHours));
    s.endGroup();

    s.beginGroup("Playback");
    gapless_->setChecked(s.value("gapless", true).toBool());
    int cf = s.value("crossfadeDuration", 0).toInt();
    crossfadeDuration_->setValue(cf);
    crossfadeLabel_->setText(QString("%1s").arg(cf / 10.0, 0, 'f', 1));
    rgMode_->setCurrentIndex(s.value("replayGainMode", 0).toInt());
    s.endGroup();

    s.beginGroup("UI");
    minimizeToTray_->setChecked(s.value("minimizeToTray", false).toBool());
    showVisualizer_->setChecked(s.value("showVisualizer", true).toBool());
    showWaveform_->setChecked(s.value("showWaveform", true).toBool());
    s.endGroup();

    s.endGroup();
}

void SettingsView::saveSetting(const QString& key, const QVariant& value) {
    QSettings s;
    s.setValue("Nada/" + key, value);
}

void SettingsView::onWatchFolderAdded() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Music Folder");
    if (dir.isEmpty()) return;
    watchFolders_->addItem(dir);
    library_->addWatchFolder(dir);
}

void SettingsView::onWatchFolderRemoved() {
    auto item = watchFolders_->currentItem();
    if (!item) return;
    library_->removeWatchFolder(item->text());
    delete item;
}

void SettingsView::onRescanClicked() {
    library_->scanNow();
}

void SettingsView::onLibraryScanStarted() {
    scanProgress_->setValue(0);
    scanProgress_->setVisible(true);
    rescanBtn_->setEnabled(false);
}

void SettingsView::onLibraryScanProgress(int done, int total) {
    if (total > 0) scanProgress_->setValue(done * 100 / total);
}

void SettingsView::onLibraryScanFinished(int added, int updated, int removed) {
    scanProgress_->setVisible(false);
    rescanBtn_->setEnabled(true);
    scanStats_->setText(QString("Added: %1  Updated: %2  Removed: %3")
                            .arg(added).arg(updated).arg(removed));
}
