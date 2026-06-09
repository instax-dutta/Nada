#pragma once

#include <QString>
#include <QFileInfo>
#include <QDateTime>

struct TrackMetadata {
    int     id = 0;
    QString path, title, artist, album, albumArtist;
    QString composer, genre, comment, format;
    int     year=0, trackNumber=0, trackTotal=0;
    int     discNumber=0, discTotal=0;
    int64_t durationMs=0;
    int     sampleRate=0, bitDepth=0, channels=0, bitrateKbps=0;
    int64_t fileSize=0;
    float   replayGainTrackDb=0, replayGainTrackPeak=1.f;
    float   replayGainAlbumDb=0, replayGainAlbumPeak=1.f;
    QString coverArtHash;
    int64_t dateAdded=0, dateModified=0;
    int     playCount=0, rating=0;

    bool    isValid()        const { return !path.isEmpty(); }
    QString displayTitle()   const { return title.isEmpty() ? QFileInfo(path).baseName() : title; }
    QString displayArtist()  const {
        if (!artist.isEmpty()) return artist;
        if (!albumArtist.isEmpty()) return albumArtist;
        return QStringLiteral("Unknown Artist");
    }
    QString formattedDuration() const {
        int seconds = static_cast<int>(durationMs / 1000);
        int mins = seconds / 60;
        int secs = seconds % 60;
        return QStringLiteral("%1:%2").arg(mins).arg(secs, 2, 10, QLatin1Char('0'));
    }
};
