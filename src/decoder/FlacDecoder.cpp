#define DR_FLAC_IMPLEMENTATION
#include "dr_flac.h"
#include "FlacDecoder.h"
#include <spdlog/spdlog.h>

bool FlacDecoder::open(const QString& path) {
    close();
    auto* f = drflac_open_file(path.toUtf8().constData(), nullptr);
    if (!f) {
        spdlog::error("FlacDecoder: failed to open {}", path.toStdString());
        return false;
    }
    totalFrames_ = static_cast<int64_t>(f->totalPCMFrameCount);
    sampleRate_ = static_cast<int>(f->sampleRate);
    channels_ = static_cast<int>(f->channels);
    bitDepth_ = static_cast<int>(f->bitsPerSample);
    flac_ = f;
    spdlog::info("FlacDecoder: opened {} ({}ch, {}Hz, {}bit)", path.toStdString(), channels_, sampleRate_, bitDepth_);
    return true;
}

int64_t FlacDecoder::decode(float* buf, int64_t frames) {
    if (!flac_) return 0;
    auto* f = static_cast<drflac*>(flac_);
    uint64_t read = drflac_read_pcm_frames_f32(f, static_cast<size_t>(frames), buf);
    return static_cast<int64_t>(read);
}

void FlacDecoder::seek(int64_t frame) {
    if (!flac_) return;
    auto* f = static_cast<drflac*>(flac_);
    drflac_seek_to_pcm_frame(f, static_cast<uint64_t>(frame));
}

void FlacDecoder::close() {
    if (flac_) {
        auto* f = static_cast<drflac*>(flac_);
        drflac_close(f);
        flac_ = nullptr;
    }
}
