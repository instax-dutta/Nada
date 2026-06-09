#pragma once

#include <QString>
#include <cstdint>

class AudioDecoder {
public:
    virtual ~AudioDecoder() = default;
    virtual bool     open(const QString& path)              = 0;
    virtual int64_t  totalFrames()                    const = 0;
    virtual int      sampleRate()                     const = 0;
    virtual int      channels()                       const = 0;
    virtual int      bitDepth()                       const = 0;
    virtual int64_t  decode(float* buf, int64_t frames)     = 0;
    virtual void     seek(int64_t frame)                    = 0;
    virtual void     close()                                = 0;
};
