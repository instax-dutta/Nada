#include "LibraryScanner.h"
#include "TagReader.h"
#include <QDirIterator>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QDateTime>
#include <QSqlError>
#include <QThread>
#include <QFileInfo>
#include <QDateTime>
#include <spdlog/spdlog.h>
#include <set>

static const QStringList kAudioExts = {
    ".flac", ".mp3", ".wav", ".aiff", ".aif", ".m4a", ".mp4",
    ".ogg", ".opus", ".ape", ".wv"
};

static bool isAudioFile(const QString& path) {
    QString ext = QFileInfo(path).suffix().toLower();
    return kAudioExts.contains("." + ext);
}

LibraryScanner::LibraryScanner(QObject* parent)
    : QObject(parent)
{
}

void LibraryScanner::cancelScan() {
    cancelled_.store(true);
}

void LibraryScanner::scanFolders(const QStringList& roots) {
    emit scanStarted();
    cancelled_.store(false);

    int added = 0, updated = 0, removed = 0;
    QString dbName = "scanner_" + QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()));
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", dbName);
        db.setDatabaseName(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/nada/library.db");

        if (!db.open()) {
            spdlog::error("LibraryScanner: failed to open database");
            return;
        }

        QStringList allScannedPaths;
        int totalFiles = 0;
        int processed = 0;

        for (const QString& root : roots) {
            QDirIterator it(root, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                QString path = it.next();
                if (isAudioFile(path)) {
                    allScannedPaths.append(path);
                    totalFiles++;
                }
            }
        }

        QSqlQuery checkQuery(db);
        checkQuery.prepare("SELECT date_modified FROM tracks WHERE path = ?");

        QSqlQuery upsertQuery(db);
        upsertQuery.prepare(R"(
            INSERT INTO tracks (path, title, artist, album, album_artist, composer, genre,
                year, track_number, track_total, disc_number, disc_total,
                duration_ms, sample_rate, bit_depth, channels, bitrate_kbps,
                format, file_size, replay_gain_track_db, replay_gain_track_peak,
                replay_gain_album_db, replay_gain_album_peak,
                cover_art_hash, date_added, date_modified, play_count, rating)
            VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,0,0)
            ON CONFLICT(path) DO UPDATE SET
                title=excluded.title, artist=excluded.artist, album=excluded.album,
                album_artist=excluded.album_artist, composer=excluded.composer,
                genre=excluded.genre, year=excluded.year,
                track_number=excluded.track_number, track_total=excluded.track_total,
                disc_number=excluded.disc_number, disc_total=excluded.disc_total,
                duration_ms=excluded.duration_ms, sample_rate=excluded.sample_rate,
                bit_depth=excluded.bit_depth, channels=excluded.channels,
                bitrate_kbps=excluded.bitrate_kbps, format=excluded.format,
                file_size=excluded.file_size,
                replay_gain_track_db=excluded.replay_gain_track_db,
                replay_gain_track_peak=excluded.replay_gain_track_peak,
                replay_gain_album_db=excluded.replay_gain_album_db,
                replay_gain_album_peak=excluded.replay_gain_album_peak,
                cover_art_hash=excluded.cover_art_hash,
                date_modified=excluded.date_modified
        )");

        db.transaction();
        int batchCount = 0;

        for (const QString& path : allScannedPaths) {
            if (cancelled_.load(std::memory_order_relaxed)) {
                db.rollback();
                spdlog::info("LibraryScanner: scan cancelled by user");
                emit scanFinished(0, 0, 0);
                db.close();
                QSqlDatabase::removeDatabase(dbName);
                return;
            }

            QFileInfo fi(path);
            int64_t modTime = fi.lastModified().toSecsSinceEpoch();

            checkQuery.bindValue(0, path);
            checkQuery.exec();
            bool needsUpdate = true;
            if (checkQuery.next()) {
                int64_t dbModTime = checkQuery.value(0).toLongLong();
                if (dbModTime == modTime) {
                    needsUpdate = false;
                }
            }

            if (needsUpdate) {
                TrackMetadata meta = TagReader::readMetadata(path);
                if (!meta.isValid()) {
                    processed++;
                    emit progress(processed, totalFiles);
                    continue;
                }

                upsertQuery.bindValue(0, meta.path);
                upsertQuery.bindValue(1, meta.title);
                upsertQuery.bindValue(2, meta.artist);
                upsertQuery.bindValue(3, meta.album);
                upsertQuery.bindValue(4, meta.albumArtist);
                upsertQuery.bindValue(5, meta.composer);
                upsertQuery.bindValue(6, meta.genre);
                upsertQuery.bindValue(7, meta.year);
                upsertQuery.bindValue(8, meta.trackNumber);
                upsertQuery.bindValue(9, meta.trackTotal);
                upsertQuery.bindValue(10, meta.discNumber);
                upsertQuery.bindValue(11, meta.discTotal);
                upsertQuery.bindValue(12, meta.durationMs);
                upsertQuery.bindValue(13, meta.sampleRate);
                upsertQuery.bindValue(14, meta.bitDepth);
                upsertQuery.bindValue(15, meta.channels);
                upsertQuery.bindValue(16, meta.bitrateKbps);
                upsertQuery.bindValue(17, meta.format);
                upsertQuery.bindValue(18, meta.fileSize);
                upsertQuery.bindValue(19, meta.replayGainTrackDb);
                upsertQuery.bindValue(20, meta.replayGainTrackPeak);
                upsertQuery.bindValue(21, meta.replayGainAlbumDb);
                upsertQuery.bindValue(22, meta.replayGainAlbumPeak);
                upsertQuery.bindValue(23, meta.coverArtHash);
                upsertQuery.bindValue(24, meta.dateAdded);
                upsertQuery.bindValue(25, meta.dateModified);

                if (upsertQuery.exec()) {
                    if (checkQuery.next()) updated++;
                    else added++;
                } else {
                    spdlog::error("LibraryScanner: upsert failed: {}", upsertQuery.lastError().text().toStdString());
                }

                batchCount++;
                if (batchCount % 100 == 0) {
                    db.commit();
                    db.transaction();
                }
            }

            processed++;
            emit progress(processed, totalFiles);
        }

        db.commit();

        QSqlQuery deleteQuery(db);
        if (!allScannedPaths.isEmpty()) {
            QStringList placeholders;
            for (int i = 0; i < allScannedPaths.size(); ++i) placeholders << "?";
            QString sql = QString("DELETE FROM tracks WHERE path NOT IN (%1)").arg(placeholders.join(","));
            deleteQuery.prepare(sql);
            for (int i = 0; i < allScannedPaths.size(); ++i) {
                deleteQuery.bindValue(i, allScannedPaths[i]);
            }
            if (deleteQuery.exec()) {
                removed = deleteQuery.numRowsAffected();
            }
        }

        db.close();
    }
    QSqlDatabase::removeDatabase(dbName);

    emit scanFinished(added, updated, removed);
}
