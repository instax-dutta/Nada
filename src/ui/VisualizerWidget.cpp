#include "VisualizerWidget.h"
#include "Theme.h"
#include <QPainter>
#include <QPainterPath>
#include <cmath>
#include <algorithm>

VisualizerWidget::VisualizerWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(120);
    setMaximumHeight(200);

    for (int i = 0; i < kFftSize; ++i) {
        fftWindow_[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * i / (kFftSize - 1)));
    }

    fftIn_ = fftw_alloc_real(kFftSize);
    fftOut_ = fftw_alloc_complex(kFftSize / 2 + 1);
    fftPlan_ = fftw_plan_dft_r2c_1d(kFftSize, fftIn_, fftOut_, FFTW_ESTIMATE);

    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        drainRingBuffer();
        if (mode_ == Spectrum) processFft();
        update();
    });
    timer->start(16);
}

VisualizerWidget::~VisualizerWidget() {
    if (fftPlan_) fftw_destroy_plan(fftPlan_);
    if (fftIn_) fftw_free(fftIn_);
    if (fftOut_) fftw_free(fftOut_);
}

void VisualizerWidget::drainRingBuffer() {
    int rd = ringRead_.load(std::memory_order_acquire);
    int wr = ringWrite_.load(std::memory_order_acquire);
    while (rd != wr) {
        scopeBuffer_.push_back(ringBuffer_[rd]);
        rd = (rd + 1) & (kRingBufferSize - 1);
    }
    ringRead_.store(rd, std::memory_order_release);

    if (scopeBuffer_.size() > kFftSize * 2) {
        scopeBuffer_.erase(scopeBuffer_.begin(), scopeBuffer_.begin() + (scopeBuffer_.size() - kFftSize));
    }
}

void VisualizerWidget::pushAudio(const float* data, int frames, int channels) {
    for (int i = 0; i < frames; ++i) {
        int wr = ringWrite_.load(std::memory_order_relaxed);
        int next = (wr + 1) & (kRingBufferSize - 1);
        if (next == ringRead_.load(std::memory_order_acquire))
            break;
        ringBuffer_[wr] = data[i * channels];
        ringWrite_.store(next, std::memory_order_release);
    }
}

void VisualizerWidget::processFft() {
    if (scopeBuffer_.size() < kFftSize) return;

    for (int i = 0; i < kFftSize; ++i) {
        fftIn_[i] = scopeBuffer_[i] * fftWindow_[i];
    }
    scopeBuffer_.erase(scopeBuffer_.begin(), scopeBuffer_.begin() + kFftSize / 2);

    fftw_execute(fftPlan_);

    for (int i = 0; i < kFftSize / 2; ++i) {
        fftMagnitudes_[i] = std::sqrt(fftOut_[i][0] * fftOut_[i][0] + fftOut_[i][1] * fftOut_[i][1]);
    }

    float maxMag = *std::max_element(fftMagnitudes_.begin(), fftMagnitudes_.end());
    if (maxMag < 0.001f) maxMag = 0.001f;

    double minFreq = 20.0;
    double maxFreq = 20000.0;
    int maxBin = std::min(kFftSize / 2, (int)(maxFreq * kFftSize / 48000.0));
    int minBin = std::max(1, (int)(minFreq * kFftSize / 48000.0));

    for (int i = 0; i < kNumBars; ++i) {
        double lowFreq = minFreq * std::pow(maxFreq / minFreq, i / (double)kNumBars);
        double highFreq = minFreq * std::pow(maxFreq / minFreq, (i + 1) / (double)kNumBars);
        int lowBin = std::max(minBin, (int)(lowFreq * kFftSize / 48000.0));
        int highBin = std::min(maxBin, (int)(highFreq * kFftSize / 48000.0));
        if (highBin <= lowBin) highBin = lowBin + 1;

        double energy = 0;
        for (int b = lowBin; b < highBin; ++b) {
            energy += fftMagnitudes_[b];
        }
        energy /= (highBin - lowBin);
        barEnergies_[i] = static_cast<float>(energy / maxMag);
        barEnergies_[i] = std::clamp(barEnergies_[i], 0.0f, 1.0f);

        if (barEnergies_[i] > barPeaks_[i]) {
            barPeaks_[i] = barEnergies_[i];
        } else {
            barPeaks_[i] -= 0.02f;
            if (barPeaks_[i] < 0) barPeaks_[i] = 0;
        }
    }
}

void VisualizerWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Theme::SURFACE);

    int w = width();
    int h = height();

    if (mode_ == Spectrum) {
        int barW = w / kNumBars;
        for (int i = 0; i < kNumBars; ++i) {
            float val = barEnergies_[i];
            int barH = static_cast<int>(val * (h - 4));
            int x = i * barW;
            int y = h - 2 - barH;

            float t = std::clamp(val, 0.0f, 1.0f);
            QColor c;
            c.setHsvF(0.1f - t * 0.1f, 0.7f, 0.3f + t * 0.5f);
            p.fillRect(x, y, barW - 1, barH, c);

            int peakY = h - 2 - static_cast<int>(barPeaks_[i] * (h - 4));
            p.fillRect(x, peakY, barW - 1, 2, Theme::ACCENT);
        }
    } else if (mode_ == Oscilloscope) {
        if (scopeBuffer_.size() < 2) return;
        QPainterPath path;
        int midY = h / 2;
        for (size_t i = 0; i < scopeBuffer_.size(); ++i) {
            float x = (float)i / scopeBuffer_.size() * w;
            float y = midY + scopeBuffer_[i] * (h / 2 - 4);
            if (i == 0) path.moveTo(x, y);
            else path.lineTo(x, y);
        }
        p.setPen(QPen(Theme::ACCENT, 1));
        p.drawPath(path);
    } else {
        if (scopeBuffer_.size() < 2) return;
        float l = 0, r = 0;
        for (size_t i = 0; i < scopeBuffer_.size(); i += 2) {
            l = std::max(l, std::abs(scopeBuffer_[i]));
        }
        for (size_t i = 1; i < scopeBuffer_.size(); i += 2) {
            r = std::max(r, std::abs(scopeBuffer_[i]));
        }

        int barW = w / 4;
        int cv = h - 2 - static_cast<int>(l * (h - 4));
        p.fillRect(w / 2 - barW, cv, barW, h - 2 - cv, Theme::ACCENT);
        int cvR = h - 2 - static_cast<int>(r * (h - 4));
        p.fillRect(w / 2, cvR, barW, h - 2 - cvR, Theme::ACCENT);
    }
}
