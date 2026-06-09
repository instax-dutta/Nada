#pragma once

#include <QString>

namespace Schema {

constexpr auto CREATE_TRACKS = R"(
CREATE TABLE IF NOT EXISTS tracks (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    path TEXT UNIQUE NOT NULL,
    title TEXT, artist TEXT, album TEXT, album_artist TEXT,
    composer TEXT, genre TEXT, comment TEXT,
    year INTEGER, track_number INTEGER, track_total INTEGER,
    disc_number INTEGER, disc_total INTEGER,
    duration_ms INTEGER, sample_rate INTEGER,
    bit_depth INTEGER, channels INTEGER, bitrate_kbps INTEGER,
    format TEXT, file_size INTEGER,
    replay_gain_track_db REAL, replay_gain_track_peak REAL,
    replay_gain_album_db REAL, replay_gain_album_peak REAL,
    cover_art_hash TEXT,
    date_added INTEGER, date_modified INTEGER,
    play_count INTEGER DEFAULT 0,
    last_played INTEGER, rating INTEGER DEFAULT 0
);
)";

constexpr auto CREATE_PLAYLISTS = R"(
CREATE TABLE IF NOT EXISTS playlists (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT UNIQUE NOT NULL,
    created INTEGER
);
)";

constexpr auto CREATE_PLAYLIST_TRACKS = R"(
CREATE TABLE IF NOT EXISTS playlist_tracks (
    playlist_id INTEGER REFERENCES playlists(id) ON DELETE CASCADE,
    track_id    INTEGER REFERENCES tracks(id) ON DELETE CASCADE,
    position INTEGER NOT NULL,
    PRIMARY KEY(playlist_id, position)
);
)";

constexpr auto CREATE_INDEX_ARTIST = "CREATE INDEX IF NOT EXISTS idx_artist ON tracks(artist);";
constexpr auto CREATE_INDEX_ALBUM = "CREATE INDEX IF NOT EXISTS idx_album ON tracks(album);";
constexpr auto CREATE_INDEX_ALBUM_ARTIST = "CREATE INDEX IF NOT EXISTS idx_album_artist ON tracks(album_artist);";
constexpr auto CREATE_INDEX_GENRE = "CREATE INDEX IF NOT EXISTS idx_genre ON tracks(genre);";
constexpr auto CREATE_INDEX_PATH = "CREATE INDEX IF NOT EXISTS idx_path ON tracks(path);";

inline QString allStatements() {
    return QString(CREATE_TRACKS) + CREATE_PLAYLISTS + CREATE_PLAYLIST_TRACKS
        + CREATE_INDEX_ARTIST + CREATE_INDEX_ALBUM
        + CREATE_INDEX_ALBUM_ARTIST + CREATE_INDEX_GENRE + CREATE_INDEX_PATH;
}

}
