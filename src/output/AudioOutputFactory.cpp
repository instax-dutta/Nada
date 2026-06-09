#include "AudioOutput.h"
#if defined(__APPLE__)
#include "CoreAudioOutput.h"
#elif defined(_WIN32)
#include "WasapiOutput.h"
#else
#include "AlsaOutput.h"
#endif

std::unique_ptr<AudioOutput> AudioOutput::create() {
#if defined(__APPLE__)
    return std::make_unique<CoreAudioOutput>();
#elif defined(_WIN32)
    return std::make_unique<WasapiOutput>();
#else
    return std::make_unique<AlsaOutput>();
#endif
}
