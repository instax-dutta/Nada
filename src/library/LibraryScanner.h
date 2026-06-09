#pragma once

#include <QObject>
#include <QStringList>
#include <atomic>

class LibraryScanner : public QObject {
    Q_OBJECT
public:
    explicit LibraryScanner(QObject* parent = nullptr);

    void scanFolders(const QStringList& roots);
    void cancelScan();

signals:
    void scanStarted();
    void progress(int done, int total);
    void scanFinished(int added, int updated, int removed);

private:
    std::atomic<bool> cancelled_{false};
};
