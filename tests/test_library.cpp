#include <catch2/catch_test_macros.hpp>
#include <QApplication>
#include "../src/library/TrackMetadata.h"
#include "../src/library/TagReader.h"
#include "../src/library/CoverArtCache.h"
#include "../src/library/LibraryManager.h"
#include "../src/library/PlaylistManager.h"
#include "../src/library/Schema.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QBuffer>

namespace {
    struct AppSetup {
        AppSetup() {
            static int argc = 1;
            static char* argv[] = { const_cast<char*>("test") };
            static QApplication app(argc, argv);
            (void)app;
        }
    } appSetup;
}

TEST_CASE("Schema creates tables", "[library][schema]") {
    QTemporaryDir dir;
    QString dbPath = dir.path() + "/test.db";
    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "schema_test");
        db.setDatabaseName(dbPath);
        REQUIRE(db.open());

        QSqlQuery q(db);
        REQUIRE(q.exec(Schema::CREATE_TRACKS));
        REQUIRE(q.exec(Schema::CREATE_PLAYLISTS));
        REQUIRE(q.exec(Schema::CREATE_PLAYLIST_TRACKS));
        REQUIRE(q.exec(Schema::CREATE_INDEX_ARTIST));
        REQUIRE(q.exec(Schema::CREATE_INDEX_ALBUM));
        REQUIRE(q.exec(Schema::CREATE_INDEX_ALBUM_ARTIST));
        REQUIRE(q.exec(Schema::CREATE_INDEX_GENRE));
        REQUIRE(q.exec(Schema::CREATE_INDEX_PATH));

        QSqlQuery check(db);
        check.exec("SELECT name FROM sqlite_master WHERE type='table' ORDER BY name");
        QStringList tables;
        while (check.next()) tables << check.value(0).toString();
        REQUIRE(tables.contains("tracks"));
        REQUIRE(tables.contains("playlists"));
        REQUIRE(tables.contains("playlist_tracks"));

        db.close();
    }
    QSqlDatabase::removeDatabase("schema_test");
}

TEST_CASE("TagReader parses WAV metadata", "[library][tagreader]") {
    QString testFile = "/tmp/test_sine.wav";
    REQUIRE(QFileInfo::exists(testFile));

    TrackMetadata meta = TagReader::readMetadata(testFile);
    REQUIRE(meta.isValid());
    REQUIRE(meta.format == "WAV");
    REQUIRE(meta.sampleRate == 44100);
    REQUIRE(meta.channels == 2);
    REQUIRE(meta.durationMs > 0);
}

TEST_CASE("CoverArtCache store and retrieve", "[library][coverart]") {
    CoverArtCache cache;
    QPixmap original(64, 64);
    original.fill(Qt::red);
    QByteArray rawBytes;
    QBuffer buffer(&rawBytes);
    buffer.open(QIODevice::WriteOnly);
    original.toImage().save(&buffer, "PNG");
    buffer.close();

    QString hash = cache.store(rawBytes);
    REQUIRE_FALSE(hash.isEmpty());

    QPixmap retrieved = cache.get(hash);
    REQUIRE_FALSE(retrieved.isNull());
    REQUIRE(retrieved.width() == 64);
    REQUIRE(retrieved.height() == 64);
}

TEST_CASE("LibraryManager CRUD operations", "[library][manager]") {
    QTemporaryDir dir;
    QString dbPath = dir.path() + "/library.db";

    LibraryManager mgr;
    REQUIRE(mgr.initialize(dbPath));

    auto allTracks = mgr.getAllTracks();
    REQUIRE(allTracks.isEmpty());

    auto searched = mgr.searchTracks("test");
    REQUIRE(searched.isEmpty());

    REQUIRE(mgr.databasePath() == dbPath);

    QSqlDatabase::removeDatabase("library_main");
}

TEST_CASE("PlaylistManager CRUD operations", "[library][playlist]") {
    QTemporaryDir dir;
    QString dbPath = dir.path() + "/library.db";

    {
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "library_main");
        db.setDatabaseName(dbPath);
        REQUIRE(db.open());
        QSqlQuery q(db);
        REQUIRE(q.exec(Schema::CREATE_TRACKS));
        REQUIRE(q.exec(Schema::CREATE_PLAYLISTS));
        REQUIRE(q.exec(Schema::CREATE_PLAYLIST_TRACKS));
        REQUIRE(q.exec(Schema::CREATE_INDEX_ARTIST));
        REQUIRE(q.exec(Schema::CREATE_INDEX_ALBUM));
        REQUIRE(q.exec(Schema::CREATE_INDEX_ALBUM_ARTIST));
        REQUIRE(q.exec(Schema::CREATE_INDEX_GENRE));
        REQUIRE(q.exec(Schema::CREATE_INDEX_PATH));

        QSqlQuery ins(db);
        ins.prepare("INSERT INTO tracks (path, title, artist, album, format, duration_ms, sample_rate, channels, bit_depth, file_size, date_added, date_modified) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
        ins.bindValue(0, "/tmp/test1.wav");
        ins.bindValue(1, "Test 1");
        ins.bindValue(2, "Artist 1");
        ins.bindValue(3, "Album 1");
        ins.bindValue(4, "wav");
        ins.bindValue(5, 1000);
        ins.bindValue(6, 44100);
        ins.bindValue(7, 2);
        ins.bindValue(8, 16);
        ins.bindValue(9, 100);
        ins.bindValue(10, 1000);
        ins.bindValue(11, 1000);
        REQUIRE(ins.exec());

        db.close();
    }

    PlaylistManager pm;
    int id = pm.createPlaylist("test_playlist");
    REQUIRE(id >= 0);

    pm.addTrack(id, 1);

    auto tracks = pm.getPlaylistTracks(id);
    REQUIRE(tracks.size() == 1);
    REQUIRE(tracks[0].title == "Test 1");

    pm.renamePlaylist(id, "renamed");

    pm.removeTrack(id, 1);
    tracks = pm.getPlaylistTracks(id);
    REQUIRE(tracks.isEmpty());

    pm.deletePlaylist(id);

    QSqlDatabase::removeDatabase("library_main");
}

TEST_CASE("TagReader readMetadata", "[library][tagreader]") {
    QString testFile = "/tmp/test_sine.wav";
    REQUIRE(QFileInfo::exists(testFile));

    TrackMetadata meta = TagReader::readMetadata(testFile);
    REQUIRE(meta.isValid());

    REQUIRE(meta.path == testFile);
    REQUIRE(meta.sampleRate == 44100);
    REQUIRE(meta.channels == 2);
    REQUIRE(meta.format == "WAV");
    REQUIRE(meta.fileSize > 0);
}
