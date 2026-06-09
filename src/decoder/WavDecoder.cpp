#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#include "WavDecoder.h"
#include <spdlog/spdlog.h>
#include <cstdlib>

static void* drwav_alloc(size_t sz, void*) { return std::malloc(sz); }
static void* drwav_realloc(void* p, size_t sz, void*) { return std::realloc(p, sz); }
static void drwav_free(void* p, void*) { std::free(p); }

bool WavDecoder::open(const QString& path) {
    close();
    auto* w = static_cast<drwav*>(std::malloc(sizeof(drwav)));
    drwav_allocation_callbacks alloc = {};
    alloc.pUserData = nullptr;
    alloc.onMalloc = drwav_alloc;
    alloc.onRealloc = drwav_realloc;
    alloc.onFree = drwav_free;
    if (!drwav_init_file(w, path.toUtf8().constData(), &alloc)) {
        spdlog::error("WavDecoder: failed to open {}", path.toStdString());
        std::free(w);
        return false;
    }
    totalFrames_ = static_cast<int64_t>(w->totalPCMFrameCount);
    sampleRate_ = static_cast<int>(w->sampleRate);
    channels_ = static_cast<int>(w->channels);
    bitDepth_ = static_cast<int>(w->bitsPerSample);
    wav_ = w;
    spdlog::info("WavDecoder: opened {} ({}ch, {}Hz, {}bit)", path.toStdString(), channels_, sampleRate_, bitDepth_);
    return true;
}

int64_t WavDecoder::decode(float* buf, int64_t frames) {
    if (!wav_) return 0;
    auto* w = static_cast<drwav*>(wav_);
    uint64_t read = drwav_read_pcm_frames_f32(w, static_cast<size_t>(frames), buf);
    return static_cast<int64_t>(read);
}

void WavDecoder::seek(int64_t frame) {
    if (!wav_) return;
    auto* w = static_cast<drwav*>(wav_);
    drwav_seek_to_pcm_frame(w, static_cast<uint64_t>(frame));
}

void WavDecoder::close() {
    if (wav_) {
        auto* w = static_cast<drwav*>(wav_);
        drwav_uninit(w);
        std::free(w);
        wav_ = nullptr;
    }
}
