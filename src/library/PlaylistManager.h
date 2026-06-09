#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include "TrackMetadata.h"

class PlaylistManager : public QObject {
    Q_OBJECT
public:
    explicit PlaylistManager(const QString& dbConnectionName = "library_main", QObject* parent = nullptr);

    int     createPlaylist(const QString& name);
    void    deletePlaylist(int id);
    void    renamePlaylist(int id, const QString& name);
    void    addTrack(int playlistId, int trackId, int position = -1);
    void    removeTrack(int playlistId, int trackId);
    void    reorderTrack(int playlistId, int trackId, int newPosition);
    QList<TrackMetadata> getPlaylistTracks(int playlistId);
    int     importM3U(const QString& path);
    bool    exportM3U(int playlistId, const QString& path);

private:
    struct PlaylistInfo {
        int id;
        QString name;
    };
    QList<PlaylistInfo> getPlaylists();

    QString dbConnName_;
};
