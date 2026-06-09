#include "PlayingIndicator.h"
#include "Theme.h"
#include <QPainter>
#include <QTimer>

PlayingIndicator::PlayingIndicator(QWidget* parent) : QWidget(parent) {
    setFixedSize(14, 20);

    const char* props[] = {"h0", "h1", "h2"};
    const int delays[] = {0, 80, 160};
    for (int i = 0; i < 3; ++i) {
        anims_[i] = new QPropertyAnimation(this, props[i], this);
        anims_[i]->setDuration(600);
        anims_[i]->setStartValue(4.0f);
        anims_[i]->setKeyValueAt(0.5, 20.0f);
        anims_[i]->setEndValue(4.0f);
        anims_[i]->setEasingCurve(QEasingCurve::InOutSine);
        anims_[i]->setLoopCount(-1);
        QTimer::singleShot(delays[i], this, [this, i]() { anims_[i]->start(); });
    }
}

void PlayingIndicator::startAnimation() {
    for (auto* a : anims_)
        if (a && a->state() != QAbstractAnimation::Running) a->start();
}

void PlayingIndicator::stopAnimation() {
    for (auto* a : anims_) if (a) a->stop();
}

void PlayingIndicator::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(Theme::ACCENT);
    p.setPen(Qt::NoPen);
    int x = 0;
    for (int i = 0; i < 3; ++i) {
        float h = qBound(2.0f, h_[i], 20.0f);
        p.drawRoundedRect(QRectF(x, 20.0f - h, 3.0, h), 1.0, 1.0);
        x += 5;
    }
}
