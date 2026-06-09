#pragma once

#include <QObject>
#include <QStringList>
#include <QSqlDatabase>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QFuture>
#include "TrackMetadata.h"

class LibraryScanner;

class LibraryManager : public QObject {
    Q_OBJECT
public:
    explicit LibraryManager(QObject* parent = nullptr);
    ~LibraryManager() override;

    bool initialize(const QString& dbPath);
    void addWatchFolder(const QString& path);
    void removeWatchFolder(const QString& path);
    void scanNow();

    QList<TrackMetadata> getAllTracks();
    QList<TrackMetadata> getTracksByArtist(const QString& artist);
    QList<TrackMetadata> getTracksByAlbum(const QString& album, const QString& artist);
    QList<TrackMetadata> searchTracks(const QString& query);

    void incrementPlayCount(int trackId);
    void setRating(int trackId, int stars);

    QString databasePath() const { return dbPath_; }

signals:
    void scanStarted();
    void scanProgress(int done, int total);
    void scanFinished(int added, int updated, int removed);
    void libraryChanged();

private slots:
    void onDirectoryChanged(const QString& path);

private:
    QList<TrackMetadata> execQuery(const QString& sql, const QVariantList& params = {});

    QString dbPath_;
    QSqlDatabase db_;
    LibraryScanner* scanner_ = nullptr;
    QFileSystemWatcher* watcher_ = nullptr;
    QTimer* debounceTimer_ = nullptr;
};
