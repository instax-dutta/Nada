#pragma once

#include <cmath>
#include <cstddef>

struct BiquadCoeffs {
    double b0 = 1.0, b1 = 0.0, b2 = 0.0;
    double a1 = 0.0, a2 = 0.0;
};

class BiquadFilter {
public:
    void setCoeffs(const BiquadCoeffs& c) {
        if (crossfade_ > 0) {
            cur_ = computeInterpolated();
        }
        old_ = cur_;
        target_ = c;
        crossfade_ = 64;
        fadePos_ = 0;
    }

    float process(float in) {
        if (crossfade_ > 0) {
            double frac = static_cast<double>(fadePos_) / crossfade_;
            cur_.b0 = old_.b0 + (target_.b0 - old_.b0) * frac;
            cur_.b1 = old_.b1 + (target_.b1 - old_.b1) * frac;
            cur_.b2 = old_.b2 + (target_.b2 - old_.b2) * frac;
            cur_.a1 = old_.a1 + (target_.a1 - old_.a1) * frac;
            cur_.a2 = old_.a2 + (target_.a2 - old_.a2) * frac;
            ++fadePos_;
            if (fadePos_ >= crossfade_) {
                cur_ = target_;
                crossfade_ = 0;
            }
        }
        out_ = cur_.b0 * in + cur_.b1 * x1_ + cur_.b2 * x2_ - cur_.a1 * y1_ - cur_.a2 * y2_;
        x2_ = x1_; x1_ = in; y2_ = y1_; y1_ = out_;
        return static_cast<float>(out_);
    }

    void reset() {
        x1_ = x2_ = y1_ = y2_ = out_ = 0.0;
        crossfade_ = 0;
    }

    static BiquadCoeffs makePeak(double freq, double sr, double gainDb, double Q) {
        double A = std::pow(10.0, gainDb / 40.0);
        double w0 = 2.0 * M_PI * freq / sr;
        double alpha = std::sin(w0) / (2.0 * Q);
        BiquadCoeffs c;
        c.b0 = 1.0 + alpha * A;
        c.b1 = -2.0 * std::cos(w0);
        c.b2 = 1.0 - alpha * A;
        c.a1 = c.b1;
        c.a2 = 1.0 - alpha / A;
        double a0 = 1.0 + alpha / A;
        c.b0 /= a0; c.b1 /= a0; c.b2 /= a0;
        c.a1 /= a0; c.a2 /= a0;
        return c;
    }

    static BiquadCoeffs makeLowShelf(double freq, double sr, double gainDb, double S) {
        double A = std::pow(10.0, gainDb / 40.0);
        double w0 = 2.0 * M_PI * freq / sr;
        double alpha = std::sin(w0) / 2.0 * std::sqrt((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0);
        double cosW0 = std::cos(w0);
        BiquadCoeffs c;
        c.b0 = A * ((A + 1.0) - (A - 1.0) * cosW0 + 2.0 * std::sqrt(A) * alpha);
        c.b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * cosW0);
        c.b2 = A * ((A + 1.0) - (A - 1.0) * cosW0 - 2.0 * std::sqrt(A) * alpha);
        c.a1 = -2.0 * ((A - 1.0) + (A + 1.0) * cosW0);
        c.a2 = (A + 1.0) + (A - 1.0) * cosW0 - 2.0 * std::sqrt(A) * alpha;
        double a0 = (A + 1.0) + (A - 1.0) * cosW0 + 2.0 * std::sqrt(A) * alpha;
        c.b0 /= a0; c.b1 /= a0; c.b2 /= a0;
        c.a1 /= a0; c.a2 /= a0;
        return c;
    }

    static BiquadCoeffs makeHighShelf(double freq, double sr, double gainDb, double S) {
        double A = std::pow(10.0, gainDb / 40.0);
        double w0 = 2.0 * M_PI * freq / sr;
        double alpha = std::sin(w0) / 2.0 * std::sqrt((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0);
        double cosW0 = std::cos(w0);
        BiquadCoeffs c;
        c.b0 = A * ((A + 1.0) + (A - 1.0) * cosW0 + 2.0 * std::sqrt(A) * alpha);
        c.b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosW0);
        c.b2 = A * ((A + 1.0) + (A - 1.0) * cosW0 - 2.0 * std::sqrt(A) * alpha);
        c.a1 = 2.0 * ((A - 1.0) - (A + 1.0) * cosW0);
        c.a2 = (A + 1.0) - (A - 1.0) * cosW0 - 2.0 * std::sqrt(A) * alpha;
        double a0 = (A + 1.0) - (A - 1.0) * cosW0 + 2.0 * std::sqrt(A) * alpha;
        c.b0 /= a0; c.b1 /= a0; c.b2 /= a0;
        c.a1 /= a0; c.a2 /= a0;
        return c;
    }

    static BiquadCoeffs makeLowPass(double freq, double sr, double Q) {
        double w0 = 2.0 * M_PI * freq / sr;
        double alpha = std::sin(w0) / (2.0 * Q);
        double cosW0 = std::cos(w0);
        BiquadCoeffs c;
        c.b0 = (1.0 - cosW0) / 2.0;
        c.b1 = 1.0 - cosW0;
        c.b2 = (1.0 - cosW0) / 2.0;
        c.a1 = -2.0 * cosW0;
        c.a2 = 1.0 - alpha;
        double a0 = 1.0 + alpha;
        c.b0 /= a0; c.b1 /= a0; c.b2 /= a0;
        c.a1 /= a0; c.a2 /= a0;
        return c;
    }

    static BiquadCoeffs makeHighPass(double freq, double sr, double Q) {
        double w0 = 2.0 * M_PI * freq / sr;
        double alpha = std::sin(w0) / (2.0 * Q);
        double cosW0 = std::cos(w0);
        BiquadCoeffs c;
        c.b0 = (1.0 + cosW0) / 2.0;
        c.b1 = -(1.0 + cosW0);
        c.b2 = (1.0 + cosW0) / 2.0;
        c.a1 = -2.0 * cosW0;
        c.a2 = 1.0 - alpha;
        double a0 = 1.0 + alpha;
        c.b0 /= a0; c.b1 /= a0; c.b2 /= a0;
        c.a1 /= a0; c.a2 /= a0;
        return c;
    }

    static BiquadCoeffs makeNotch(double freq, double sr, double Q) {
        double w0 = 2.0 * M_PI * freq / sr;
        double alpha = std::sin(w0) / (2.0 * Q);
        double cosW0 = std::cos(w0);
        BiquadCoeffs c;
        c.b0 = 1.0;
        c.b1 = -2.0 * cosW0;
        c.b2 = 1.0;
        c.a1 = -2.0 * cosW0;
        c.a2 = 1.0 - alpha;
        double a0 = 1.0 + alpha;
        c.b0 /= a0; c.b1 /= a0; c.b2 /= a0;
        c.a1 /= a0; c.a2 /= a0;
        return c;
    }

private:
    BiquadCoeffs computeInterpolated() const {
        double frac = static_cast<double>(fadePos_) / crossfade_;
        BiquadCoeffs c;
        c.b0 = old_.b0 + (target_.b0 - old_.b0) * frac;
        c.b1 = old_.b1 + (target_.b1 - old_.b1) * frac;
        c.b2 = old_.b2 + (target_.b2 - old_.b2) * frac;
        c.a1 = old_.a1 + (target_.a1 - old_.a1) * frac;
        c.a2 = old_.a2 + (target_.a2 - old_.a2) * frac;
        return c;
    }

    BiquadCoeffs cur_;
    BiquadCoeffs old_;
    BiquadCoeffs target_;
    double x1_ = 0.0, x2_ = 0.0, y1_ = 0.0, y2_ = 0.0, out_ = 0.0;
    int crossfade_ = 0;
    int fadePos_ = 0;
};
