#pragma once

#include <QWidget>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include "../dsp/ParametricEQ.h"

class FreqResponseGraph;
class DspChain;
class ToggleSwitch;
class SlimSlider;

class EQView : public QWidget {
    Q_OBJECT
public:
    explicit EQView(DspChain* dsp, QWidget* parent = nullptr);

signals:
    void eqChanged();

private slots:
    void onBandChanged(int index);
    void onPresetChanged(int index);
    void savePreset();
    void deletePreset();
    void resetAll();

private:
    void setupUi();
    void setupPresets();
    void syncFromDsp();
    void syncBandToDsp(int index);

    DspChain* dsp_;
    FreqResponseGraph* graph_;

    struct BandControl {
        QLabel* freqLabel;
        QDoubleSpinBox* gainSpin;
        QDoubleSpinBox* qSpin;
        QComboBox* typeCombo;
        ToggleSwitch* enableToggle;
    };
    std::vector<BandControl> bands_;

    SlimSlider* preampSlider_;
    QLabel* preampLabel_;
    ToggleSwitch* eqEnable_;
    QComboBox* presetCombo_;
    QPushButton* saveBtn_;
    QPushButton* deleteBtn_;
    QPushButton* resetBtn_;

    QComboBox* crossfeedLevel_;
    ToggleSwitch* crossfeedEnable_;
    QComboBox* rgMode_;
    SlimSlider* rgPreampSlider_;
    ToggleSwitch* softClipEnable_;

    QStringList builtinPresets_;
};
