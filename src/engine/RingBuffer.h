#pragma once

#include <atomic>
#include <cstddef>
#include <algorithm>

template<typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity)
        : capacity_(nextPowerOf2(capacity))
        , buffer_(new T[capacity_])
        , writeHead_(0)
        , readHead_(0)
    {
    }

    ~RingBuffer() {
        delete[] buffer_;
    }

    RingBuffer(const RingBuffer&) = delete;
    RingBuffer& operator=(const RingBuffer&) = delete;

    size_t write(const T* src, size_t count) {
        size_t available = availableWrite();
        size_t toWrite = std::min(count, available);
        size_t mask = capacity_ - 1;

        for (size_t i = 0; i < toWrite; ++i) {
            buffer_[(writeHead_ + i) & mask] = src[i];
        }
        std::atomic_thread_fence(std::memory_order_release);
        writeHead_.store(writeHead_.load(std::memory_order_relaxed) + toWrite, std::memory_order_relaxed);
        return toWrite;
    }

    size_t read(T* dst, size_t count) {
        size_t available = availableRead();
        size_t toRead = std::min(count, available);
        size_t mask = capacity_ - 1;

        for (size_t i = 0; i < toRead; ++i) {
            dst[i] = buffer_[(readHead_ + i) & mask];
        }
        std::atomic_thread_fence(std::memory_order_release);
        readHead_.store(readHead_.load(std::memory_order_relaxed) + toRead, std::memory_order_relaxed);
        return toRead;
    }

    size_t availableRead() const {
        size_t w = writeHead_.load(std::memory_order_acquire);
        size_t r = readHead_.load(std::memory_order_relaxed);
        return w - r;
    }

    size_t availableWrite() const {
        size_t r = readHead_.load(std::memory_order_acquire);
        size_t w = writeHead_.load(std::memory_order_relaxed);
        return capacity_ - (w - r);
    }

    void reset() {
        writeHead_.store(0, std::memory_order_relaxed);
        readHead_.store(0, std::memory_order_relaxed);
    }

    size_t capacity() const { return capacity_; }

private:
    static size_t nextPowerOf2(size_t n) {
        size_t p = 1;
        while (p < n) p <<= 1;
        return p;
    }

    const size_t capacity_;
    T* const buffer_;
    std::atomic<size_t> writeHead_;
    std::atomic<size_t> readHead_;
};
