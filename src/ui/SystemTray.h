#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>

class SystemTray : public QObject {
    Q_OBJECT
public:
    explicit SystemTray(QObject* parent = nullptr);

    void show();
    void setPlaying(bool playing);

signals:
    void showWindow();
    void quit();
    void playPause();
    void nextTrack();
    void prevTrack();

private:
    QSystemTrayIcon* icon_ = nullptr;
    QMenu* menu_ = nullptr;
    QAction* playPauseAction_ = nullptr;
};
