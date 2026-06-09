#include "DecoderFactory.h"
#include "WavDecoder.h"
#include "FlacDecoder.h"
#include "Mp3Decoder.h"
#include <QFileInfo>
#include <spdlog/spdlog.h>

std::unique_ptr<AudioDecoder> DecoderFactory::createForFile(const QString& path) {
    QString ext = QFileInfo(path).suffix().toLower();

    if (ext == "wav") {
        return std::make_unique<WavDecoder>();
    } else if (ext == "flac") {
        return std::make_unique<FlacDecoder>();
    } else if (ext == "mp3") {
        return std::make_unique<Mp3Decoder>();
    }

    spdlog::warn("DecoderFactory: unsupported extension '{}' for {}", ext.toStdString(), path.toStdString());
    return nullptr;
}
