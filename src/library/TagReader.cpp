#include "TagReader.h"
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/audioproperties.h>
#include <taglib/tpropertymap.h>
#include <taglib/flacfile.h>
#include <taglib/mpegfile.h>
#include <taglib/wavfile.h>
#include <taglib/mp4file.h>
#include <taglib/vorbisfile.h>
#include <taglib/opusfile.h>
#include <taglib/apefile.h>
#include <taglib/wavpackfile.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/textidentificationframe.h>
#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <spdlog/spdlog.h>

static float parseGain(const QString& str) {
    if (str.isEmpty()) return 0.f;
    QString s = str;
    s.remove(QChar(' '));
    s.remove(QChar('d')).remove(QChar('B'));
    return s.toFloat();
}

static float parsePeak(const QString& str) {
    if (str.isEmpty()) return 1.f;
    return str.toFloat();
}

static TagLib::String qs(const QString& s) {
    return TagLib::String(s.toUtf8().constData(), TagLib::String::UTF8);
}

static QString fromTS(const TagLib::String& s) {
    return QString::fromUtf8(s.toCString(true));
}

static QString detectFormat(const QString& path) {
    QString ext = QFileInfo(path).suffix().toLower();
    if (ext == "mp3") return "MP3";
    if (ext == "flac") return "FLAC";
    if (ext == "wav") return "WAV";
    if (ext == "aiff" || ext == "aif") return "AIFF";
    if (ext == "m4a" || ext == "mp4") return "M4A";
    if (ext == "ogg") return "OGG";
    if (ext == "opus") return "Opus";
    if (ext == "ape") return "APE";
    if (ext == "wv") return "WV";
    return ext.toUpper();
}

TrackMetadata TagReader::readMetadata(const QString& path) {
    TrackMetadata meta;
    meta.path = path;
    meta.format = detectFormat(path);

    TagLib::FileRef f(path.toUtf8().constData());
    if (f.isNull() || !f.audioProperties()) {
        spdlog::warn("TagReader: cannot open {}", path.toStdString());
        return meta;
    }

    TagLib::Tag* tag = f.tag();
    if (tag) {
        meta.title = fromTS(tag->title());
        meta.artist = fromTS(tag->artist());
        meta.album = fromTS(tag->album());
        meta.comment = fromTS(tag->comment());
        meta.genre = fromTS(tag->genre());
        meta.year = tag->year();
        meta.trackNumber = tag->track();
    }

    TagLib::AudioProperties* ap = f.audioProperties();
    if (ap) {
        meta.durationMs = ap->lengthInMilliseconds();
        meta.sampleRate = ap->sampleRate();
        meta.channels = ap->channels();
        meta.bitrateKbps = ap->bitrate();
        meta.bitDepth = 16;
    }

    QFileInfo fi(path);
    meta.fileSize = fi.size();
    meta.dateAdded = QDateTime::currentSecsSinceEpoch();
    meta.dateModified = fi.lastModified().toSecsSinceEpoch();

    auto props = f.file()->properties();
    meta.albumArtist = fromTS(props["ALBUMARTIST"].toString());
    if (meta.albumArtist.isEmpty())
        meta.albumArtist = fromTS(props["ALBUM ARTIST"].toString());
    meta.composer = fromTS(props["COMPOSER"].toString());
    QString trackTotal = fromTS(props["TRACKNUMBER"].toString());
    if (!trackTotal.isEmpty()) {
        int slash = trackTotal.indexOf('/');
        if (slash > 0) {
            meta.trackTotal = trackTotal.mid(slash + 1).toInt();
        }
    }
    meta.discNumber = fromTS(props["DISCNUMBER"].toString()).toInt();
    QString discTotal = fromTS(props["DISCNUMBER"].toString());
    if (!discTotal.isEmpty()) {
        int slash = discTotal.indexOf('/');
        if (slash > 0) {
            meta.discTotal = discTotal.mid(slash + 1).toInt();
        }
    }

    meta.replayGainTrackDb = parseGain(fromTS(props["REPLAYGAIN_TRACK_GAIN"].toString()));
    meta.replayGainTrackPeak = parsePeak(fromTS(props["REPLAYGAIN_TRACK_PEAK"].toString()));
    meta.replayGainAlbumDb = parseGain(fromTS(props["REPLAYGAIN_ALBUM_GAIN"].toString()));
    meta.replayGainAlbumPeak = parsePeak(fromTS(props["REPLAYGAIN_ALBUM_PEAK"].toString()));

    QString ext = QFileInfo(path).suffix().toLower();
    if (ext == "mp3" && meta.replayGainTrackDb == 0.f) {
        auto* mp3 = dynamic_cast<TagLib::MPEG::File*>(f.file());
        if (mp3 && mp3->ID3v2Tag()) {
            auto list = mp3->ID3v2Tag()->frameListMap()["TXXX"];
            for (size_t i = 0; i < list.size(); ++i) {
                auto* frame = dynamic_cast<TagLib::ID3v2::UserTextIdentificationFrame*>(list[i]);
                if (frame && frame->description() == "replaygain_track_gain") {
                    meta.replayGainTrackDb = parseGain(fromTS(frame->fieldList().size() > 1 ? frame->fieldList()[1] : TagLib::String()));
                }
                if (frame && frame->description() == "replaygain_track_peak") {
                    meta.replayGainTrackPeak = parsePeak(fromTS(frame->fieldList().size() > 1 ? frame->fieldList()[1] : TagLib::String()));
                }
                if (frame && frame->description() == "replaygain_album_gain") {
                    meta.replayGainAlbumDb = parseGain(fromTS(frame->fieldList().size() > 1 ? frame->fieldList()[1] : TagLib::String()));
                }
                if (frame && frame->description() == "replaygain_album_peak") {
                    meta.replayGainAlbumPeak = parsePeak(fromTS(frame->fieldList().size() > 1 ? frame->fieldList()[1] : TagLib::String()));
                }
            }
        }
    }

    if (ext == "m4a" && meta.replayGainTrackDb == 0.f) {
        auto* mp4 = dynamic_cast<TagLib::MP4::File*>(f.file());
        if (mp4) {
            auto& items = mp4->tag()->itemMap();
            auto processMP4RG = [&](const char* key, float& outDb, float& outPeak, bool isGain) {
                auto it = items.find(qs(key));
                if (it != items.end()) {
                    QString val = fromTS(it->second.toStringList().toString());
                    if (isGain) outDb = parseGain(val);
                    else outPeak = parsePeak(val);
                }
            };
            processMP4RG("----:com.apple.iTunes:REPLAYGAIN_TRACK_GAIN", meta.replayGainTrackDb, meta.replayGainTrackPeak, true);
            processMP4RG("----:com.apple.iTunes:REPLAYGAIN_TRACK_PEAK", meta.replayGainTrackDb, meta.replayGainTrackPeak, false);
            processMP4RG("----:com.apple.iTunes:REPLAYGAIN_ALBUM_GAIN", meta.replayGainAlbumDb, meta.replayGainAlbumPeak, true);
            processMP4RG("----:com.apple.iTunes:REPLAYGAIN_ALBUM_PEAK", meta.replayGainAlbumDb, meta.replayGainAlbumPeak, false);
        }
    }

    QByteArray coverData = readEmbeddedCover(path);
    if (!coverData.isEmpty()) {
        QByteArray hash = QCryptographicHash::hash(coverData, QCryptographicHash::Md5).toHex();
        meta.coverArtHash = QString::fromLatin1(hash);
        QString cachePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                          + QStringLiteral("/nada/covers/");
        QDir().mkpath(cachePath);
        QString coverFile = cachePath + meta.coverArtHash + QStringLiteral(".jpg");
        if (!QFile::exists(coverFile)) {
            QImage img;
            if (img.loadFromData(coverData)) {
                if (img.width() > 600 || img.height() > 600)
                    img = img.scaled(600, 600, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                img.save(coverFile, "JPEG", 90);
            }
        }
    }

    return meta;
}

QByteArray TagReader::readEmbeddedCover(const QString& path) {
    TagLib::FileRef f(path.toUtf8().constData());
    if (f.isNull() || !f.file()) return {};

    QString ext = QFileInfo(path).suffix().toLower();

    if (ext == "flac") {
        auto* flac = dynamic_cast<TagLib::FLAC::File*>(f.file());
        if (flac) {
            auto pics = flac->pictureList();
            for (auto& pic : pics) {
                if (pic->type() == TagLib::FLAC::Picture::FrontCover) {
                    auto data = pic->data();
                    return QByteArray(data.data(), static_cast<int>(data.size()));
                }
            }
        }
    }

    if (ext == "mp3") {
        auto* mp3 = dynamic_cast<TagLib::MPEG::File*>(f.file());
        if (mp3 && mp3->ID3v2Tag()) {
            auto frames = mp3->ID3v2Tag()->frameListMap()["APIC"];
            if (!frames.isEmpty()) {
                auto* apic = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(frames.front());
                if (apic) {
                    auto data = apic->picture();
                    return QByteArray(data.data(), static_cast<int>(data.size()));
                }
            }
        }
    }

    if (ext == "m4a" || ext == "mp4") {
        auto* mp4 = dynamic_cast<TagLib::MP4::File*>(f.file());
        if (mp4) {
            auto& items = mp4->tag()->itemMap();
            auto it = items.find(qs("covr"));
            if (it != items.end()) {
                auto coverList = it->second.toCoverArtList();
                if (!coverList.isEmpty()) {
                    auto data = coverList.front().data();
                    return QByteArray(data.data(), static_cast<int>(data.size()));
                }
            }
        }
    }

    return {};
}
