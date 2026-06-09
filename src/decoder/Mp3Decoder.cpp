#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"
#include "Mp3Decoder.h"
#include <spdlog/spdlog.h>

bool Mp3Decoder::open(const QString& path) {
    close();
    auto* m = new drmp3();
    if (!drmp3_init_file(m, path.toUtf8().constData(), nullptr)) {
        spdlog::error("Mp3Decoder: failed to open {}", path.toStdString());
        delete m;
        return false;
    }
    totalFrames_ = static_cast<int64_t>(drmp3_get_pcm_frame_count(m));
    sampleRate_ = static_cast<int>(m->sampleRate);
    channels_ = static_cast<int>(m->channels);
    bitDepth_ = 16;
    mp3_ = m;
    spdlog::info("Mp3Decoder: opened {} ({}ch, {}Hz)", path.toStdString(), channels_, sampleRate_);
    return true;
}

int64_t Mp3Decoder::decode(float* buf, int64_t frames) {
    if (!mp3_) return 0;
    auto* m = static_cast<drmp3*>(mp3_);
    uint64_t read = drmp3_read_pcm_frames_f32(m, static_cast<size_t>(frames), buf);
    return static_cast<int64_t>(read);
}

void Mp3Decoder::seek(int64_t frame) {
    if (!mp3_) return;
    auto* m = static_cast<drmp3*>(mp3_);
    drmp3_seek_to_pcm_frame(m, static_cast<uint64_t>(frame));
}

void Mp3Decoder::close() {
    if (mp3_) {
        auto* m = static_cast<drmp3*>(mp3_);
        drmp3_uninit(m);
        delete m;
        mp3_ = nullptr;
    }
}
