#pragma once

#include <QWidget>
#include "../dsp/ParametricEQ.h"

class FreqResponseGraph : public QWidget {
    Q_OBJECT
public:
    explicit FreqResponseGraph(QWidget* parent = nullptr);

    void setEq(ParametricEQ* eq) { eq_ = eq; }
    void refresh();

signals:
    void bandChanged(int index);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    int bandAtPoint(const QPointF& pt) const;
    QPointF bandToScreen(int index) const;
    void screenToBand(const QPointF& pt, int index);

    static constexpr int kPoints = 512;
    static constexpr double kMinFreq = 20.0;
    static constexpr double kMaxFreq = 20000.0;

    ParametricEQ* eq_ = nullptr;
    std::vector<float> freqs_ = std::vector<float>(kPoints);
    std::vector<float> magDb_ = std::vector<float>(kPoints);

    int dragBand_ = -1;
    QPointF dragStart_;
    double dragStartFreq_ = 0;
    double dragStartGain_ = 0;
    bool shiftHeld_ = false;
};
