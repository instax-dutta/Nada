#include "CoverArtCache.h"
#include <QStandardPaths>
#include <QDir>
#include <QImage>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>

CoverArtCache::CoverArtCache() {
    QDir().mkpath(cacheDir());
}

QString CoverArtCache::cacheDir() const {
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/nada/covers/";
}

QString CoverArtCache::filePath(const QString& hash) const {
    return cacheDir() + hash + ".jpg";
}

QString CoverArtCache::thumbPath(const QString& hash) const {
    return cacheDir() + hash + "_thumb.jpg";
}

QString CoverArtCache::store(const QByteArray& rawBytes) {
    QByteArray hash = QCryptographicHash::hash(rawBytes, QCryptographicHash::Md5).toHex();
    QString hashStr = QString::fromLatin1(hash);

    QImage img;
    if (!img.loadFromData(rawBytes)) return hashStr;
    if (img.width() > 600 || img.height() > 600) {
        img = img.scaled(600, 600, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    img.save(filePath(hashStr), "JPEG", 90);

    QImage thumb = img.scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    thumb.save(thumbPath(hashStr), "JPEG", 85);

    return hashStr;
}

QPixmap CoverArtCache::get(const QString& hash) {
    if (hash.isEmpty()) return {};

    if (auto* cached = memoryCache_.object(hash)) {
        return *cached;
    }

    QString path = filePath(hash);
    if (!QFileInfo::exists(path)) return {};

    QPixmap pix(path);
    if (!pix.isNull()) {
        pix.setDevicePixelRatio(1.0);
        memoryCache_.insert(hash, new QPixmap(pix));
    }
    return pix;
}

QPixmap CoverArtCache::getThumbnail(const QString& hash) {
    if (hash.isEmpty()) return {};

    QString thumbKey = hash + "_thumb";
    if (auto* cached = memoryCache_.object(thumbKey)) {
        return *cached;
    }

    QString path = thumbPath(hash);
    if (!QFileInfo::exists(path)) {
        QPixmap full = get(hash);
        if (full.isNull()) return {};
        QImage thumb = full.toImage().scaled(120, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        thumb.save(path, "JPEG", 85);
    }

    QPixmap pix(path);
    if (!pix.isNull()) {
        memoryCache_.insert(thumbKey, new QPixmap(pix));
    }
    return pix;
}
