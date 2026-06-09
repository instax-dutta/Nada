#include "PlaylistManager.h"
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSqlError>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDateTime>
#include <spdlog/spdlog.h>

PlaylistManager::PlaylistManager(const QString& dbConnectionName, QObject* parent)
    : QObject(parent)
    , dbConnName_(dbConnectionName)
{
}

QList<PlaylistManager::PlaylistInfo> PlaylistManager::getPlaylists() {
    QList<PlaylistInfo> result;
    QSqlQuery q(QSqlDatabase::database(dbConnName_));
    q.exec("SELECT id, name FROM playlists ORDER BY name");
    while (q.next()) {
        result.append({q.value(0).toInt(), q.value(1).toString()});
    }
    return result;
}

int PlaylistManager::createPlaylist(const QString& name) {
    QSqlQuery q(QSqlDatabase::database(dbConnName_));
    q.prepare("INSERT INTO playlists (name, created) VALUES (?, ?)");
    q.bindValue(0, name);
    q.bindValue(1, QDateTime::currentSecsSinceEpoch());
    if (!q.exec()) {
        spdlog::error("PlaylistManager: create failed: {}", q.lastError().text().toStdString());
        return -1;
    }
    return q.lastInsertId().toInt();
}

void PlaylistManager::deletePlaylist(int id) {
    QSqlQuery q(QSqlDatabase::database(dbConnName_));
    q.prepare("DELETE FROM playlists WHERE id = ?");
    q.bindValue(0, id);
    q.exec();
}

void PlaylistManager::renamePlaylist(int id, const QString& name) {
    QSqlQuery q(QSqlDatabase::database(dbConnName_));
    q.prepare("UPDATE playlists SET name = ? WHERE id = ?");
    q.bindValue(0, name);
    q.bindValue(1, id);
    q.exec();
}

void PlaylistManager::addTrack(int playlistId, int trackId, int position) {
    QSqlQuery q(QSqlDatabase::database(dbConnName_));
    if (position < 0) {
        q.prepare("SELECT COALESCE(MAX(position), -1) + 1 FROM playlist_tracks WHERE playlist_id = ?");
        q.bindValue(0, playlistId);
        q.exec();
        if (q.next()) position = q.value(0).toInt();
        else position = 0;
    }
    q.prepare("INSERT OR IGNORE INTO playlist_tracks (playlist_id, track_id, position) VALUES (?, ?, ?)");
    q.bindValue(0, playlistId);
    q.bindValue(1, trackId);
    q.bindValue(2, position);
    q.exec();
}

void PlaylistManager::removeTrack(int playlistId, int trackId) {
    QSqlQuery q(QSqlDatabase::database(dbConnName_));
    q.prepare("DELETE FROM playlist_tracks WHERE playlist_id = ? AND track_id = ?");
    q.bindValue(0, playlistId);
    q.bindValue(1, trackId);
    q.exec();
}

void PlaylistManager::reorderTrack(int playlistId, int trackId, int newPosition) {
    QSqlQuery q(QSqlDatabase::database(dbConnName_));
    q.prepare("UPDATE playlist_tracks SET position = ? WHERE playlist_id = ? AND track_id = ?");
    q.bindValue(0, newPosition);
    q.bindValue(1, playlistId);
    q.bindValue(2, trackId);
    q.exec();
}

QList<TrackMetadata> PlaylistManager::getPlaylistTracks(int playlistId) {
    QList<TrackMetadata> result;
    QSqlQuery q(QSqlDatabase::database(dbConnName_));
    q.prepare(R"(
        SELECT t.* FROM tracks t
        JOIN playlist_tracks pt ON t.id = pt.track_id
        WHERE pt.playlist_id = ?
        ORDER BY pt.position
    )");
    q.bindValue(0, playlistId);
    if (!q.exec()) return result;

    while (q.next()) {
        TrackMetadata m;
        m.id = q.value("id").toInt();
        m.path = q.value("path").toString();
        m.title = q.value("title").toString();
        m.artist = q.value("artist").toString();
        m.album = q.value("album").toString();
        m.albumArtist = q.value("album_artist").toString();
        m.durationMs = q.value("duration_ms").toLongLong();
        m.format = q.value("format").toString();
        m.trackNumber = q.value("track_number").toInt();
        m.coverArtHash = q.value("cover_art_hash").toString();
        result.append(m);
    }
    return result;
}

int PlaylistManager::importM3U(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        spdlog::error("PlaylistManager: cannot open M3U: {}", path.toStdString());
        return -1;
    }

    QString baseDir = QFileInfo(path).absolutePath();
    QString name = QFileInfo(path).completeBaseName();
    int playlistId = createPlaylist(name);
    if (playlistId < 0) return -1;

    QTextStream in(&file);
    int position = 0;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;
        QFileInfo fi(line);
        QString absPath = fi.isAbsolute() ? line : baseDir + "/" + line;
        if (!fi.exists()) continue;

        QSqlQuery q(QSqlDatabase::database(dbConnName_));
        q.prepare("SELECT id FROM tracks WHERE path = ?");
        q.bindValue(0, absPath);
        if (q.exec() && q.next()) {
            int trackId = q.value(0).toInt();
            QSqlQuery ins(QSqlDatabase::database(dbConnName_));
            ins.prepare("INSERT INTO playlist_tracks (playlist_id, track_id, position) VALUES (?, ?, ?)");
            ins.bindValue(0, playlistId);
            ins.bindValue(1, trackId);
            ins.bindValue(2, position++);
            ins.exec();
        }
    }

    return playlistId;
}

bool PlaylistManager::exportM3U(int playlistId, const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        spdlog::error("PlaylistManager: cannot write M3U: {}", path.toStdString());
        return false;
    }

    QTextStream out(&file);
    out << "#EXTM3U\n";

    auto tracks = getPlaylistTracks(playlistId);
    for (const auto& t : tracks) {
        out << "#EXTINF:" << t.durationMs / 1000 << "," << t.displayArtist() << " - " << t.displayTitle() << "\n";
        out << t.path << "\n";
    }

    return true;
}
