#include "FreqResponseGraph.h"
#include "Theme.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QMenu>
#include <QAction>
#include <QLinearGradient>
#include <cmath>
#include <algorithm>

namespace {
    constexpr int kMargin = 40;
}

FreqResponseGraph::FreqResponseGraph(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(400, 200);
    setMouseTracking(true);
}

void FreqResponseGraph::refresh() {
    if (!eq_) return;
    for (int i = 0; i < kPoints; ++i) {
        freqs_[i] = static_cast<float>(kMinFreq * std::pow(kMaxFreq / kMinFreq, i / double(kPoints - 1)));
    }
    eq_->getFrequencyResponse(kPoints, freqs_.data(), magDb_.data());
    update();
}

void FreqResponseGraph::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();
    int plotW = w - 2 * kMargin;
    int plotH = h - 2 * kMargin;
    QRect plotRect(kMargin, kMargin, plotW, plotH);

    p.fillRect(rect(), Theme::SURFACE);

    auto xToScreen = [&](double freq) -> double {
        return kMargin + std::log(freq / kMinFreq) / std::log(kMaxFreq / kMinFreq) * plotW;
    };
    auto yToScreen = [&](double db) -> double {
        return kMargin + plotH / 2.0 - (db / 40.0) * plotH;
    };

    p.setPen(QPen(Theme::BORDER, 0.5));
    for (double db : {-12.0, -6.0, 6.0, 12.0}) {
        double y = yToScreen(db);
        p.drawLine(QPointF(kMargin, y), QPointF(w - kMargin, y));
    }

    p.setPen(QPen(Theme::CARD_HOVER, 1.0));
    p.drawLine(QPointF(kMargin, yToScreen(0)), QPointF(w - kMargin, yToScreen(0)));

    p.setPen(QPen(Theme::BORDER, 0.5));
    for (double freq : {20.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0}) {
        double x = xToScreen(freq);
        p.drawLine(QPointF(x, kMargin), QPointF(x, h - kMargin));
    }

    if (!magDb_.empty()) {
        QPainterPath path;
        for (int i = 0; i < kPoints; ++i) {
            double x = xToScreen(freqs_[i]);
            double y = yToScreen(magDb_[i]);
            if (i == 0) path.moveTo(x, y);
            else path.lineTo(x, y);
        }

        double bottomY = yToScreen(-30);
        QPainterPath fillPath = path;
        fillPath.lineTo(xToScreen(kMaxFreq), bottomY);
        fillPath.lineTo(xToScreen(kMinFreq), bottomY);
        fillPath.closeSubpath();

        QLinearGradient grad(0, kMargin, 0, h - kMargin);
        grad.setColorAt(0.0, QColor(0x22, 0x1C, 0x12, 160));
        grad.setColorAt(1.0, QColor(0x0A, 0x0A, 0x0A, 0));
        p.setBrush(grad);
        p.setPen(Qt::NoPen);
        p.drawPath(fillPath);

        p.setPen(QPen(Theme::ACCENT, 2));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);
    }

    if (eq_) {
        for (int i = 0; i < eq_->bandCount(); ++i) {
            auto band = eq_->band(i);
            if (!band.enabled) continue;
            QPointF pt = bandToScreen(i);
            QColor handleColor = (dragBand_ == i) ? Qt::white : Theme::ACCENT;
            p.setPen(Qt::NoPen);
            p.setBrush(handleColor);
            p.drawEllipse(pt, 5, 5);
            p.setBrush(Qt::NoBrush);
            p.setPen(QPen(Theme::SURFACE, 2));
            p.drawEllipse(pt, 5, 5);

            if (dragBand_ == i) {
                p.setPen(QPen(handleColor, 1, Qt::DotLine));
                double xLine = xToScreen(band.freq);
                p.drawLine(QPointF(xLine, kMargin), QPointF(xLine, h - kMargin));
            }
        }
    }
}

QPointF FreqResponseGraph::bandToScreen(int index) const {
    auto band = eq_->band(index);
    double x = kMargin + std::log(band.freq / kMinFreq) / std::log(kMaxFreq / kMinFreq) * (width() - 2 * kMargin);
    double y = kMargin + (height() - 2 * kMargin) / 2.0 - (band.gainDb / 40.0) * (height() - 2 * kMargin);
    return QPointF(x, y);
}

int FreqResponseGraph::bandAtPoint(const QPointF& pt) const {
    for (int i = 0; i < eq_->bandCount(); ++i) {
        auto pos = bandToScreen(i);
        if ((pt - pos).manhattanLength() < 10) return i;
    }
    return -1;
}

void FreqResponseGraph::mousePressEvent(QMouseEvent* event) {
    if (event->button() != Qt::LeftButton || !eq_) return;
    QPointF pt = event->position();
    dragBand_ = bandAtPoint(pt);
    if (dragBand_ >= 0) {
        dragStart_ = pt;
        auto b = eq_->band(dragBand_);
        dragStartFreq_ = b.freq;
        dragStartGain_ = b.gainDb;
        shiftHeld_ = event->modifiers() & Qt::ShiftModifier;
        update();
    }
}

void FreqResponseGraph::mouseMoveEvent(QMouseEvent* event) {
    if (dragBand_ < 0 || !eq_) return;
    QPointF pt = event->position();
    QPointF delta = pt - dragStart_;

    auto band = eq_->band(dragBand_);
    double snap = shiftHeld_ ? 0.1 : 0.5;
    double gainDelta = -(delta.y() / (height() - 2 * kMargin)) * 40.0;
    band.gainDb = std::round((dragStartGain_ + gainDelta) / snap) * snap;
    band.gainDb = std::clamp(band.gainDb, -20.0f, 20.0f);

    double logRatio = delta.x() / (width() - 2 * kMargin);
    double freqMult = std::pow(20000.0 / 20.0, logRatio);
    band.freq = static_cast<float>(std::clamp(dragStartFreq_ * freqMult, 20.0, 20000.0));

    eq_->setBand(dragBand_, band);
    refresh();
}

void FreqResponseGraph::mouseReleaseEvent(QMouseEvent*) {
    if (dragBand_ >= 0) {
        emit bandChanged(dragBand_);
        dragBand_ = -1;
        update();
    }
}

void FreqResponseGraph::wheelEvent(QWheelEvent* event) {
    if (!eq_) return;
    int idx = bandAtPoint(event->position());
    if (idx < 0) return;
    auto band = eq_->band(idx);
    band.Q += static_cast<float>(event->angleDelta().y()) / 120.0f * 0.5f;
    band.Q = std::clamp(band.Q, 0.1f, 10.0f);
    eq_->setBand(idx, band);
    emit bandChanged(idx);
    refresh();
}

void FreqResponseGraph::mouseDoubleClickEvent(QMouseEvent* event) {
    if (!eq_) return;
    int idx = bandAtPoint(event->position());
    if (idx < 0) return;
    auto band = eq_->band(idx);
    band.gainDb = 0;
    eq_->setBand(idx, band);
    emit bandChanged(idx);
    refresh();
}
