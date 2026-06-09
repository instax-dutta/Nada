#pragma once

#include <QString>
#include <QPixmap>
#include <QCache>
#include <QByteArray>

class CoverArtCache {
public:
    CoverArtCache();

    QString store(const QByteArray& rawBytes);
    QPixmap get(const QString& hash);
    QPixmap getThumbnail(const QString& hash);

private:
    QString cacheDir() const;
    QString filePath(const QString& hash) const;
    QString thumbPath(const QString& hash) const;

    QCache<QString, QPixmap> memoryCache_{100};
};
