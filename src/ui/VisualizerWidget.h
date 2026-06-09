#pragma once

#include <QWidget>
#include <QTimer>
#include <QPushButton>
#include <vector>
#include <fftw3.h>
#include <cstdint>

class VisualizerWidget : public QWidget {
    Q_OBJECT
public:
    explicit VisualizerWidget(QWidget* parent = nullptr);
    ~VisualizerWidget() override;

    void pushAudio(const float* data, int frames, int channels);

    enum Mode { Spectrum, Oscilloscope, VUMeters };
    void setMode(Mode m) { mode_ = m; update(); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void processFft();
    void drainRingBuffer();

    static constexpr int kFftSize = 4096;
    static constexpr int kNumBars = 80;
    static constexpr int kRingBufferSize = 8192;

    Mode mode_ = Spectrum;

    double* fftIn_ = nullptr;
    fftw_complex* fftOut_ = nullptr;
    fftw_plan fftPlan_ = nullptr;

    std::vector<float> fftWindow_ = std::vector<float>(kFftSize);
    std::vector<float> fftMagnitudes_ = std::vector<float>(kFftSize / 2);
    std::vector<float> barEnergies_ = std::vector<float>(kNumBars, 0.0f);
    std::vector<float> barPeaks_ = std::vector<float>(kNumBars, 0.0f);

    std::vector<float> scopeBuffer_;

    alignas(64) std::atomic<int> ringWrite_{0};
    alignas(64) std::atomic<int> ringRead_{0};
    float ringBuffer_[kRingBufferSize];
};
