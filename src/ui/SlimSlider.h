#pragma once

#include <QWidget>

class SlimSlider : public QWidget {
    Q_OBJECT
public:
    explicit SlimSlider(Qt::Orientation orient = Qt::Horizontal, QWidget* parent = nullptr);

    QSize sizeHint() const override { return {200, 20}; }
    QSize minimumSizeHint() const override { return {60, 20}; }

    int value() const { return value_; }
    void setValue(int v);
    void setRange(int min, int max) { min_ = min; max_ = max; setValue(value_); update(); }

signals:
    void valueChanged(int value);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private:
    void updateFromPos(int x);
    int value_ = 100;
    int min_ = 0;
    int max_ = 100;
    bool dragging_ = false;
};
