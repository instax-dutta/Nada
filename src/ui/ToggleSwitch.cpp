#include "ToggleSwitch.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>

ToggleSwitch::ToggleSwitch(QWidget* parent) : QAbstractButton(parent) {
    setCheckable(true);
    setCursor(Qt::PointingHandCursor);
    setFixedSize(44, 24);
    anim_ = new QPropertyAnimation(this, "thumbX", this);
    anim_->setDuration(120);
    anim_->setEasingCurve(QEasingCurve::OutCubic);
}

void ToggleSwitch::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    // Track — ACCENT #c8a96e when on, CARD #1a1a1a when off
    QPainterPath track;
    track.addRoundedRect(0, 0, 44, 24, 12, 12);
    p.fillPath(track, isChecked() ? QColor(0xc8, 0xa9, 0x6e) : QColor(0x1a, 0x1a, 0x1a));
    // Thumb 18px — BG #111111 when on, grey #555 when off
    QColor thumbColor = isChecked() ? QColor(0x11, 0x11, 0x11) : QColor(0x55, 0x55, 0x55);
    p.setBrush(thumbColor);
    p.setPen(Qt::NoPen);
    p.drawEllipse(QRectF(thumbX_, 3, 18, 18));
}

void ToggleSwitch::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton && rect().contains(e->pos())) {
        setChecked(!isChecked());
        anim_->stop();
        anim_->setStartValue(thumbX_);
        anim_->setEndValue(isChecked() ? 23.0f : 3.0f);
        anim_->start();
        emit clicked(isChecked());
    }
}
