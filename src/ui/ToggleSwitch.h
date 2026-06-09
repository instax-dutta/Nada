#pragma once

#include <QAbstractButton>
#include <QPropertyAnimation>

class ToggleSwitch : public QAbstractButton {
    Q_OBJECT
    Q_PROPERTY(float thumbX READ thumbX WRITE setThumbX)
public:
    explicit ToggleSwitch(QWidget* parent = nullptr);

    QSize sizeHint() const override { return {44, 24}; }
    QSize minimumSizeHint() const override { return {44, 24}; }

    float thumbX() const { return thumbX_; }
    void setThumbX(float x) { thumbX_ = x; update(); }

protected:
    void paintEvent(QPaintEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private:
    float thumbX_ = 3.0f;
    QPropertyAnimation* anim_ = nullptr;
};
