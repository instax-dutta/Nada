#pragma once

#include <QWidget>
#include <QPropertyAnimation>

class PlayingIndicator : public QWidget {
    Q_OBJECT
    Q_PROPERTY(float h0 READ h0 WRITE setH0)
    Q_PROPERTY(float h1 READ h1 WRITE setH1)
    Q_PROPERTY(float h2 READ h2 WRITE setH2)
public:
    explicit PlayingIndicator(QWidget* parent = nullptr);

    QSize sizeHint() const override { return {14, 20}; }
    QSize minimumSizeHint() const override { return {14, 20}; }

    void startAnimation();
    void stopAnimation();

    float h0() const { return h_[0]; } void setH0(float v) { h_[0] = v; update(); }
    float h1() const { return h_[1]; } void setH1(float v) { h_[1] = v; update(); }
    float h2() const { return h_[2]; } void setH2(float v) { h_[2] = v; update(); }

protected:
    void paintEvent(QPaintEvent*) override;

private:
    float h_[3] = {8.0f, 14.0f, 6.0f};
    QPropertyAnimation* anims_[3] = {nullptr, nullptr, nullptr};
};
