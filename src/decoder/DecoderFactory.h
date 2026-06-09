#pragma once

#include <memory>
#include <QString>
#include "AudioDecoder.h"

class DecoderFactory {
public:
    static std::unique_ptr<AudioDecoder> createForFile(const QString& path);
};
