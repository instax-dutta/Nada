#include "SlimSlider.h"
#include "Theme.h"
#include <QPainter>
#include <QMouseEvent>

SlimSlider::SlimSlider(Qt::Orientation, QWidget* parent) : QWidget(parent) {
    setCursor(Qt::PointingHandCursor);
    setMinimumHeight(20);
    setAttribute(Qt::WA_Hover, true);
}

void SlimSlider::setValue(int v) {
    v = qBound(min_, v, max_);
    if (v == value_) { update(); return; }
    value_ = v;
    update();
    emit valueChanged(value_);
}

void SlimSlider::updateFromPos(int x) {
    if (width() <= 0) return;
    double ratio = qBound(0.0, static_cast<double>(x) / width(), 1.0);
    setValue(min_ + static_cast<int>(ratio * (max_ - min_)));
}

void SlimSlider::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const int cy = height() / 2;
    const double ratio = (max_ > min_)
        ? static_cast<double>(value_ - min_) / (max_ - min_) : 0.0;
    const int fillX = static_cast<int>(ratio * width());

    p.setPen(Qt::NoPen);
    p.setBrush(Theme::CARD);
    p.drawRoundedRect(0, cy - 1.5, width(), 3, 1.5, 1.5);

    if (fillX > 0) {
        p.setBrush(Theme::ACCENT);
        p.drawRoundedRect(0, cy - 1.5, fillX, 3, 1.5, 1.5);
    }

    float thumbR = dragging_ ? 6.0f : 4.0f;
    if (dragging_) {
        p.setPen(QPen(Theme::BG, 2));
        p.setBrush(Theme::ACCENT);
        p.drawEllipse(QPointF(fillX, cy), thumbR, thumbR);
    } else {
        p.setBrush(Theme::ACCENT);
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(fillX, cy), thumbR, thumbR);
    }
}

void SlimSlider::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        dragging_ = true;
        updateFromPos(e->pos().x());
    }
}

void SlimSlider::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        dragging_ = false;
        update();
    }
}

void SlimSlider::mouseMoveEvent(QMouseEvent* e) {
    if (e->buttons() & Qt::LeftButton) updateFromPos(e->pos().x());
}
