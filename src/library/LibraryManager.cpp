#include "LibraryManager.h"
#include "LibraryScanner.h"
#include "Schema.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDateTime>
#include <QStandardPaths>
#include <QSettings>
#include <QDir>
#include <spdlog/spdlog.h>

LibraryManager::LibraryManager(QObject* parent)
    : QObject(parent)
    , scanner_(new LibraryScanner(this))
    , watcher_(new QFileSystemWatcher(this))
    , debounceTimer_(new QTimer(this))
{
    connect(scanner_, &LibraryScanner::scanStarted, this, &LibraryManager::scanStarted);
    connect(scanner_, &LibraryScanner::progress, this, &LibraryManager::scanProgress);
    connect(scanner_, &LibraryScanner::scanFinished, this, [this](int a, int u, int r) {
        emit scanFinished(a, u, r);
        emit libraryChanged();
    });

    debounceTimer_->setSingleShot(true);
    debounceTimer_->setInterval(2000);
    connect(debounceTimer_, &QTimer::timeout, this, [this]() {
        scanNow();
    });

    connect(watcher_, &QFileSystemWatcher::directoryChanged, this, &LibraryManager::onDirectoryChanged);
}

LibraryManager::~LibraryManager() {
    if (db_.isOpen()) db_.close();
}

bool LibraryManager::initialize(const QString& dbPath) {
    dbPath_ = dbPath;

    QDir().mkpath(QFileInfo(dbPath).absolutePath());

    db_ = QSqlDatabase::addDatabase("QSQLITE", "library_main");
    db_.setDatabaseName(dbPath);

    if (!db_.open()) {
        spdlog::error("LibraryManager: failed to open database: {}",
                      db_.lastError().text().toStdString());
        return false;
    }

    QSqlQuery query(db_);
    for (const auto& stmt : { Schema::CREATE_TRACKS, Schema::CREATE_PLAYLISTS,
                              Schema::CREATE_PLAYLIST_TRACKS,
                              Schema::CREATE_INDEX_ARTIST, Schema::CREATE_INDEX_ALBUM,
                              Schema::CREATE_INDEX_ALBUM_ARTIST, Schema::CREATE_INDEX_GENRE,
                              Schema::CREATE_INDEX_PATH }) {
        if (!query.exec(stmt)) {
            spdlog::error("LibraryManager: schema error: {}", query.lastError().text().toStdString());
            return false;
        }
    }

    spdlog::info("LibraryManager: database initialized at {}", dbPath.toStdString());

    QSettings settings;
    QStringList folders = settings.value("Nada/Library/watchFolders").toStringList();
    for (const QString& f : folders) {
        addWatchFolder(f);
    }

    return true;
}

void LibraryManager::addWatchFolder(const QString& path) {
    if (QDir(path).exists()) {
        watcher_->addPath(path);
        QSettings settings;
        QStringList folders = settings.value("Nada/Library/watchFolders").toStringList();
        if (!folders.contains(path)) {
            folders.append(path);
            settings.setValue("Nada/Library/watchFolders", folders);
        }
    }
}

void LibraryManager::removeWatchFolder(const QString& path) {
    watcher_->removePath(path);
    QSettings settings;
    QStringList folders = settings.value("Nada/Library/watchFolders").toStringList();
    folders.removeAll(path);
    settings.setValue("Nada/Library/watchFolders", folders);
}

void LibraryManager::scanNow() {
    QSettings settings;
    QStringList folders = settings.value("Nada/Library/watchFolders").toStringList();
    if (folders.isEmpty()) return;
    scanner_->cancelScan();
    scanner_->scanFolders(folders);
}

QList<TrackMetadata> LibraryManager::execQuery(const QString& sql, const QVariantList& params) {
    QList<TrackMetadata> results;
    QSqlQuery query(db_);
    query.prepare(sql);
    for (int i = 0; i < params.size(); ++i) {
        query.bindValue(i, params[i]);
    }
    if (!query.exec()) {
        spdlog::error("LibraryManager: query failed: {}", query.lastError().text().toStdString());
        return results;
    }
    while (query.next()) {
        TrackMetadata m;
        m.id = query.value("id").toInt();
        m.path = query.value("path").toString();
        m.title = query.value("title").toString();
        m.artist = query.value("artist").toString();
        m.album = query.value("album").toString();
        m.albumArtist = query.value("album_artist").toString();
        m.composer = query.value("composer").toString();
        m.genre = query.value("genre").toString();
        m.comment = query.value("comment").toString();
        m.year = query.value("year").toInt();
        m.trackNumber = query.value("track_number").toInt();
        m.trackTotal = query.value("track_total").toInt();
        m.discNumber = query.value("disc_number").toInt();
        m.discTotal = query.value("disc_total").toInt();
        m.durationMs = query.value("duration_ms").toLongLong();
        m.sampleRate = query.value("sample_rate").toInt();
        m.bitDepth = query.value("bit_depth").toInt();
        m.channels = query.value("channels").toInt();
        m.bitrateKbps = query.value("bitrate_kbps").toInt();
        m.format = query.value("format").toString();
        m.fileSize = query.value("file_size").toLongLong();
        m.replayGainTrackDb = query.value("replay_gain_track_db").toFloat();
        m.replayGainTrackPeak = query.value("replay_gain_track_peak").toFloat();
        m.replayGainAlbumDb = query.value("replay_gain_album_db").toFloat();
        m.replayGainAlbumPeak = query.value("replay_gain_album_peak").toFloat();
        m.coverArtHash = query.value("cover_art_hash").toString();
        m.dateAdded = query.value("date_added").toLongLong();
        m.dateModified = query.value("date_modified").toLongLong();
        m.playCount = query.value("play_count").toInt();
        m.rating = query.value("rating").toInt();
        results.append(m);
    }
    return results;
}

QList<TrackMetadata> LibraryManager::getAllTracks() {
    return execQuery("SELECT * FROM tracks ORDER BY artist, album, track_number");
}

QList<TrackMetadata> LibraryManager::getTracksByArtist(const QString& artist) {
    return execQuery("SELECT * FROM tracks WHERE artist = ? ORDER BY album, track_number", {artist});
}

QList<TrackMetadata> LibraryManager::getTracksByAlbum(const QString& album, const QString& artist) {
    return execQuery("SELECT * FROM tracks WHERE album = ? AND (artist = ? OR album_artist = ?) ORDER BY track_number",
                     {album, artist, artist});
}

QList<TrackMetadata> LibraryManager::searchTracks(const QString& query) {
    QString like = "%" + query + "%";
    return execQuery(
        "SELECT * FROM tracks WHERE title LIKE ? OR artist LIKE ? OR album LIKE ? ORDER BY artist, album, track_number",
        {like, like, like});
}

void LibraryManager::incrementPlayCount(int trackId) {
    QSqlQuery q(db_);
    q.prepare("UPDATE tracks SET play_count = play_count + 1, last_played = ? WHERE id = ?");
    q.bindValue(0, QDateTime::currentSecsSinceEpoch());
    q.bindValue(1, trackId);
    q.exec();
}

void LibraryManager::setRating(int trackId, int stars) {
    QSqlQuery q(db_);
    q.prepare("UPDATE tracks SET rating = ? WHERE id = ?");
    q.bindValue(0, std::clamp(stars, 0, 5));
    q.bindValue(1, trackId);
    q.exec();
}

void LibraryManager::onDirectoryChanged(const QString&) {
    debounceTimer_->start();
}
