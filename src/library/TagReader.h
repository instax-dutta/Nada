#pragma once

#include "TrackMetadata.h"
#include <QByteArray>

class TagReader {
public:
    static TrackMetadata readMetadata(const QString& path);
    static QByteArray readEmbeddedCover(const QString& path);
};
