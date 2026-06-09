#include "SystemTray.h"
#include <QApplication>
#include <QIcon>

SystemTray::SystemTray(QObject* parent)
    : QObject(parent)
{
    icon_ = new QSystemTrayIcon(this);
    QIcon trayIcon(":/icons/tray.svg");
    if (trayIcon.isNull()) {
        QIcon appIcon(":/icons/app_icon.svg");
        if (appIcon.isNull())
            appIcon = QApplication::windowIcon();
        trayIcon = appIcon;
    }
    icon_->setIcon(trayIcon);
    icon_->setToolTip("Nada");

    menu_ = new QMenu();
    playPauseAction_ = menu_->addAction("Play/Pause");
    menu_->addAction("Next Track", this, &SystemTray::nextTrack);
    menu_->addAction("Previous Track", this, &SystemTray::prevTrack);
    menu_->addSeparator();
    menu_->addAction("Show Window", this, &SystemTray::showWindow);
    menu_->addAction("Quit", this, &SystemTray::quit);

    icon_->setContextMenu(menu_);

    connect(playPauseAction_, &QAction::triggered, this, &SystemTray::playPause);
    connect(icon_, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick) {
            emit showWindow();
        }
    });
}

void SystemTray::show() {
    icon_->show();
}

void SystemTray::setPlaying(bool playing) {
    playPauseAction_->setText(playing ? "Pause" : "Play");
}
