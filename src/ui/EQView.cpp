#include "EQView.h"
#include "FreqResponseGraph.h"
#include "ToggleSwitch.h"
#include "SlimSlider.h"
#include "Theme.h"
#include "../dsp/DspChain.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QFormLayout>
#include <QScrollArea>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <cmath>

EQView::EQView(DspChain* dsp, QWidget* parent)
    : QWidget(parent)
    , dsp_(dsp)
{
    setupUi();
    setupPresets();
    syncFromDsp();
}

void EQView::setupUi() {
    auto* mainLay = new QHBoxLayout(this);

    auto* leftPanel = new QWidget(this);
    auto* leftLay = new QVBoxLayout(leftPanel);

    graph_ = new FreqResponseGraph(this);
    graph_->setEq(&dsp_->eq());
    graph_->setMinimumHeight(200);

    auto* graphTop = new QHBoxLayout();
    eqEnable_ = new ToggleSwitch(this);
    auto* eqLabel = new QLabel("EQ", this);
    eqLabel->setStyleSheet("color: #F5F5F7;");
    graphTop->addStretch();
    graphTop->addWidget(eqLabel);
    graphTop->addWidget(eqEnable_);
    connect(eqEnable_, &ToggleSwitch::clicked, this, [this](bool v) {
        dsp_->setEqEnabled(v);
    });

    leftLay->addLayout(graphTop);
    leftLay->addWidget(graph_, 1);

    auto* bandArea = new QWidget(this);
    auto* bandLay = new QVBoxLayout(bandArea);
    for (int i = 0; i < 10; ++i) {
        auto* row = new QHBoxLayout();
        BandControl bc;

        bc.freqLabel = new QLabel(this);
        bc.freqLabel->setFixedWidth(70);
        bc.freqLabel->setFont(Theme::monoFont());
        row->addWidget(bc.freqLabel);

        bc.gainSpin = new QDoubleSpinBox(this);
        bc.gainSpin->setRange(-20, 20);
        bc.gainSpin->setSingleStep(0.5);
        bc.gainSpin->setFixedWidth(80);
        row->addWidget(bc.gainSpin);

        bc.qSpin = new QDoubleSpinBox(this);
        bc.qSpin->setRange(0.1, 10);
        bc.qSpin->setSingleStep(0.1);
        bc.qSpin->setValue(1.41);
        bc.qSpin->setFixedWidth(70);
        row->addWidget(bc.qSpin);

        bc.typeCombo = new QComboBox(this);
        bc.typeCombo->addItems({"Peak", "LowShelf", "HighShelf", "LowPass", "HighPass", "Notch"});
        bc.typeCombo->setFixedWidth(100);
        row->addWidget(bc.typeCombo);

        bc.enableToggle = new ToggleSwitch(this);
        bc.enableToggle->setChecked(true);
        row->addWidget(bc.enableToggle);

        int idx = i;
        connect(bc.gainSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, idx]() {
            syncBandToDsp(idx);
        });
        connect(bc.qSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this, idx]() {
            syncBandToDsp(idx);
        });
        connect(bc.typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, idx]() {
            syncBandToDsp(idx);
        });
        connect(bc.enableToggle, &ToggleSwitch::clicked, this, [this, idx]() {
            syncBandToDsp(idx);
        });

        bands_.push_back(bc);
        bandLay->addLayout(row);
    }

    auto* preampRow = new QHBoxLayout();
    preampRow->addWidget(new QLabel("Preamp", this));
    preampSlider_ = new SlimSlider(Qt::Horizontal, this);
    preampSlider_->setRange(-200, 200);
    preampSlider_->setValue(0);
    preampSlider_->setFixedWidth(200);
    preampLabel_ = new QLabel("0.0 dB", this);
    preampRow->addWidget(preampSlider_);
    preampRow->addWidget(preampLabel_);
    connect(preampSlider_, &SlimSlider::valueChanged, this, [this](int v) {
        dsp_->setEqPreamp(v / 10.0f);
        preampLabel_->setText(QString("%1 dB").arg(v / 10.0, 0, 'f', 1));
    });
    bandLay->addLayout(preampRow);

    auto* presetRow = new QHBoxLayout();
    presetCombo_ = new QComboBox(this);
    presetCombo_->setMinimumWidth(200);
    presetRow->addWidget(presetCombo_);

    saveBtn_ = new QPushButton("Save", this);
    deleteBtn_ = new QPushButton("Delete", this);
    resetBtn_ = new QPushButton("Reset All", this);
    presetRow->addWidget(saveBtn_);
    presetRow->addWidget(deleteBtn_);
    presetRow->addWidget(resetBtn_);
    bandLay->addLayout(presetRow);

    connect(presetCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &EQView::onPresetChanged);
    connect(saveBtn_, &QPushButton::clicked, this, &EQView::savePreset);
    connect(deleteBtn_, &QPushButton::clicked, this, &EQView::deletePreset);
    connect(resetBtn_, &QPushButton::clicked, this, &EQView::resetAll);

    leftLay->addWidget(bandArea, 1);

    auto* leftScroll = new QScrollArea(this);
    leftScroll->setWidget(leftPanel);
    leftScroll->setWidgetResizable(true);
    leftScroll->setFrameShape(QFrame::NoFrame);
    leftScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    leftScroll->setStyleSheet(
        "QScrollArea { background: transparent; }"
        "QScrollBar:vertical { width: 6px; background: #0A0A0A; }"
        "QScrollBar::handle:vertical { background: #1E1E1E; border-radius: 3px; min-height: 20px; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }");
    mainLay->addWidget(leftScroll, 1);

    auto* rightPanel = new QWidget(this);
    rightPanel->setFixedWidth(200);
    auto* rightLay = new QVBoxLayout(rightPanel);

    auto* cfGroup = new QGroupBox("Crossfeed", this);
    auto* cfLay = new QVBoxLayout(cfGroup);
    crossfeedEnable_ = new ToggleSwitch(this);
    crossfeedLevel_ = new QComboBox(this);
    crossfeedLevel_->addItems({"Off", "Low", "Mid", "High"});
    cfLay->addWidget(crossfeedEnable_);
    cfLay->addWidget(crossfeedLevel_);
    rightLay->addWidget(cfGroup);

    connect(crossfeedEnable_, &ToggleSwitch::clicked, this, [this](bool v) {
        dsp_->setCrossfeedEnabled(v);
    });
    connect(crossfeedLevel_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int i) {
        dsp_->setCrossfeedLevel(i);
    });

    auto* rgGroup = new QGroupBox("ReplayGain", this);
    auto* rgLay = new QVBoxLayout(rgGroup);
    rgMode_ = new QComboBox(this);
    rgMode_->addItems({"Off", "Track", "Album"});
    rgPreampSlider_ = new SlimSlider(Qt::Horizontal, this);
    rgPreampSlider_->setRange(-100, 100);
    rgPreampSlider_->setValue(0);
    rgLay->addWidget(new QLabel("Mode:", this));
    rgLay->addWidget(rgMode_);
    rgLay->addWidget(new QLabel("Preamp:", this));
    rgLay->addWidget(rgPreampSlider_);
    rightLay->addWidget(rgGroup);

    connect(rgMode_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int i) {
        dsp_->setReplayGainMode(i);
    });

    rightPanel->setStyleSheet(
        "background-color: #111111;"
        "QGroupBox { color: #F5F5F7; font-size: 12px; border: 0.5px solid rgba(30,30,30,0.5); border-radius: 2px; margin-top: 12px; padding-top: 16px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }");
    auto* scGroup = new QGroupBox("Soft Clipper", this);
    auto* scLay = new QVBoxLayout(scGroup);
    softClipEnable_ = new ToggleSwitch(this);
    scLay->addWidget(softClipEnable_);
    rightLay->addWidget(scGroup);

    connect(softClipEnable_, &ToggleSwitch::clicked, this, [this](bool v) {
        dsp_->setSoftClipEnabled(v);
    });

    rightLay->addStretch();
    mainLay->addWidget(rightPanel);

    connect(graph_, &FreqResponseGraph::bandChanged, this, &EQView::onBandChanged);
}

void EQView::setupPresets() {
    builtinPresets_ = {
        "Flat", "Bass Boost", "Treble Boost", "V-Shape",
        "Classical", "Rock", "Vocal", "Loudness"
    };

    presetCombo_->clear();
    presetCombo_->addItem("-- Select Preset --");
    for (const auto& name : builtinPresets_) {
        presetCombo_->addItem(name);
    }

    QSettings settings;
    int count = settings.beginReadArray("EQ/Presets");
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        presetCombo_->addItem(settings.value("name").toString());
    }
    settings.endArray();
}

void EQView::syncFromDsp() {
    for (int i = 0; i < 10; ++i) {
        auto band = dsp_->eq().band(i);
        QString freqStr = QString::number(static_cast<int>(band.freq));
        if (band.freq >= 1000) freqStr = QString::number(band.freq / 1000.0, 'f', 1) + "k";
        bands_[i].freqLabel->setText(freqStr + " Hz");
        bands_[i].gainSpin->setValue(band.gainDb);
        bands_[i].qSpin->setValue(band.Q);
        bands_[i].typeCombo->setCurrentIndex(static_cast<int>(band.type));
        bands_[i].enableToggle->setChecked(band.enabled);
    }
    graph_->refresh();
}

void EQView::syncBandToDsp(int index) {
    auto& bc = bands_[index];
    auto band = dsp_->eq().band(index);
    band.gainDb = static_cast<float>(bc.gainSpin->value());
    band.Q = static_cast<float>(bc.qSpin->value());
    band.type = static_cast<EQBand::Type>(bc.typeCombo->currentIndex());
    band.enabled = bc.enableToggle->isChecked();
    dsp_->setEqBand(index, band);
    graph_->refresh();
    emit eqChanged();
}

void EQView::onBandChanged(int index) {
    auto band = dsp_->eq().band(index);
    bands_[index].gainSpin->setValue(band.gainDb);
    bands_[index].qSpin->setValue(band.Q);
    bands_[index].typeCombo->setCurrentIndex(static_cast<int>(band.type));
}

void EQView::onPresetChanged(int index) {
    if (index <= 0) return;
    QString name = presetCombo_->currentText();

    auto applyPreset = [this](const std::vector<EQBand>& preset) {
        for (size_t i = 0; i < preset.size() && i < 10; ++i) {
            dsp_->setEqBand(static_cast<int>(i), preset[i]);
        }
        syncFromDsp();
    };

    if (name == "Flat") {
        std::vector<EQBand> flat(10);
        applyPreset(flat);
    } else if (name == "Bass Boost") {
        std::vector<EQBand> p(10);
        p[0].gainDb = 4; p[0].freq = 60;
        p[1].gainDb = 3; p[1].freq = 120;
        p[2].gainDb = 1; p[2].freq = 250;
        applyPreset(p);
    } else if (name == "Treble Boost") {
        std::vector<EQBand> p(10);
        p[6].gainDb = 2; p[6].freq = 4000;
        p[7].gainDb = 4; p[7].freq = 8000;
        p[8].gainDb = 3; p[8].freq = 16000;
        applyPreset(p);
    } else if (name == "V-Shape") {
        std::vector<EQBand> p(10);
        p[0].gainDb = 4; p[0].freq = 60;
        p[1].gainDb = 2; p[1].freq = 120;
        p[6].gainDb = 2; p[6].freq = 4000;
        p[7].gainDb = 3; p[7].freq = 8000;
        p[8].gainDb = 2; p[8].freq = 16000;
        applyPreset(p);
    } else if (name == "Classical") {
        std::vector<EQBand> p(10);
        p[0].gainDb = -2; p[0].freq = 32;
        p[1].gainDb = -2; p[1].freq = 64;
        p[8].gainDb = 3; p[8].freq = 16000;
        applyPreset(p);
    } else if (name == "Rock") {
        std::vector<EQBand> p(10);
        p[0].gainDb = 3; p[0].freq = 60;
        p[2].gainDb = 1; p[2].freq = 250;
        p[4].gainDb = -1; p[4].freq = 1000;
        p[6].gainDb = 2; p[6].freq = 4000;
        applyPreset(p);
    } else if (name == "Vocal") {
        std::vector<EQBand> p(10);
        p[3].gainDb = -1; p[3].freq = 500;
        p[4].gainDb = 2; p[4].freq = 1000;
        p[5].gainDb = 3; p[5].freq = 2000;
        p[6].gainDb = 1; p[6].freq = 4000;
        applyPreset(p);
    } else if (name == "Loudness") {
        std::vector<EQBand> p(10);
        p[0].gainDb = 4; p[0].freq = 60;
        p[8].gainDb = 3; p[8].freq = 16000;
        applyPreset(p);
    } else {
        QSettings settings;
        int count = settings.beginReadArray("EQ/Presets");
        for (int i = 0; i < count; ++i) {
            settings.setArrayIndex(i);
            if (settings.value("name").toString() == name) {
                dsp_->fromJson(settings.value("data").toJsonObject());
                syncFromDsp();
                settings.endArray();
                return;
            }
        }
        settings.endArray();
    }
}

void EQView::savePreset() {
    QString name = presetCombo_->currentText();
    if (name == "-- Select Preset --" || name.isEmpty()) return;

    QSettings settings;
    int count = settings.beginReadArray("EQ/Presets");
    settings.endArray();

    settings.beginWriteArray("EQ/Presets");
    settings.setArrayIndex(count);
    settings.setValue("name", name);
    settings.setValue("data", dsp_->toJson());
    settings.endArray();

    if (presetCombo_->findText(name) == -1) {
        presetCombo_->addItem(name);
    }
}

void EQView::deletePreset() {
    QString name = presetCombo_->currentText();
    if (builtinPresets_.contains(name)) return;
    if (name == "-- Select Preset --" || name.isEmpty()) return;

    QSettings settings;
    int count = settings.beginReadArray("EQ/Presets");
    QJsonArray remaining;
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        if (settings.value("name").toString() != name) {
            QJsonObject obj;
            obj["name"] = settings.value("name").toString();
            obj["data"] = QJsonDocument::fromVariant(settings.value("data")).object();
            remaining.append(obj);
        }
    }
    settings.endArray();

    settings.beginWriteArray("EQ/Presets");
    for (int i = 0; i < remaining.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("name", remaining[i][QStringLiteral("name")].toString());
        settings.setValue("data", remaining[i][QStringLiteral("data")].toObject());
    }
    settings.endArray();

    int idx = presetCombo_->findText(name);
    if (idx >= 0) presetCombo_->removeItem(idx);
}

void EQView::resetAll() {
    for (int i = 0; i < 10; ++i) {
        auto band = dsp_->eq().band(i);
        band.gainDb = 0;
        band.Q = 1.41f;
        band.type = EQBand::Peak;
        band.enabled = true;
        dsp_->setEqBand(i, band);
    }
    syncFromDsp();
    emit eqChanged();
}
