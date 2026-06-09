#pragma once

#include <QColor>
#include <QPalette>
#include <QFont>
#include <QString>

namespace Theme {

constexpr QColor BG        { 0x0A, 0x0A, 0x0A };
constexpr QColor SURFACE   { 0x11, 0x11, 0x11 };
constexpr QColor CARD      { 0x16, 0x16, 0x16 };
constexpr QColor CARD_HOVER{ 0x1E, 0x1E, 0x1E };
constexpr QColor BORDER    { 30, 30, 30, 128 };
constexpr QColor ACCENT    { 0xD4, 0xB9, 0x82 };
constexpr QColor ACCENT_DIM{ 0x22, 0x1C, 0x12 };
constexpr QColor TEXT_PRI  { 0xF5, 0xF5, 0xF7 };
constexpr QColor TEXT_SEC  { 0x86, 0x86, 0x8B };
constexpr QColor TEXT_TER  { 0x51, 0x51, 0x54 };

constexpr QColor ELEVATED = SURFACE;

constexpr int FONT_UI     = 13;
constexpr int FONT_TITLE  = 16;
constexpr int FONT_MONO   = 11;

inline QPalette darkPalette() {
    QPalette p;
    p.setColor(QPalette::Window,          BG);
    p.setColor(QPalette::WindowText,      TEXT_PRI);
    p.setColor(QPalette::Base,            CARD);
    p.setColor(QPalette::AlternateBase,   SURFACE);
    p.setColor(QPalette::ToolTipBase,     SURFACE);
    p.setColor(QPalette::ToolTipText,     TEXT_PRI);
    p.setColor(QPalette::Text,            TEXT_PRI);
    p.setColor(QPalette::Button,          SURFACE);
    p.setColor(QPalette::ButtonText,      TEXT_PRI);
    p.setColor(QPalette::BrightText,      Qt::red);
    p.setColor(QPalette::Link,            ACCENT);
    p.setColor(QPalette::Highlight,       ACCENT);
    p.setColor(QPalette::HighlightedText, BG);
    return p;
}

inline QFont uiFont() {
    QFont f(".AppleSystemUIFont, .SF NS Text, SF Pro Text, -apple-system, Helvetica Neue, sans-serif", FONT_UI);
    f.setWeight(QFont::Medium);
    return f;
}

inline QFont titleFont() {
    QFont f = uiFont();
    f.setPointSize(FONT_TITLE);
    f.setWeight(QFont::Bold);
    return f;
}

inline QFont monoFont() {
    QFont f("SF Mono, .AppleSystemUIFontMono, Menlo, monospace", FONT_MONO);
    f.setWeight(QFont::DemiBold);
    return f;
}

inline QString globalQss() {
    return QStringLiteral(
        "QWidget { background-color: #0A0A0A; color: #F5F5F7; font-size: 13px; }"
        "QFrame, QScrollArea { border: none; background: transparent; }"
        "QGroupBox { border: 0.5px solid rgba(30,30,30,0.5); border-radius: 2px; margin-top: 12px; padding-top: 16px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; color: #F5F5F7; }"
        "QComboBox { background-color: #161616; border: 0.5px solid rgba(30,30,30,0.5); border-radius: 2px; padding: 4px 8px; color: #F5F5F7; }"
        "QComboBox:hover { border-color: #D4B982; }"
        "QComboBox::drop-down { border: none; width: 20px; }"
        "QComboBox::down-arrow { image: none; }"
        "QComboBox QAbstractItemView { background-color: #111111; border: 0.5px solid rgba(30,30,30,0.5); selection-background-color: #221C12; color: #F5F5F7; }"
        "QLineEdit { background-color: #161616; border: 0.5px solid rgba(30,30,30,0.5); border-radius: 2px; padding: 4px 8px; color: #F5F5F7; }"
        "QLineEdit:focus { border-color: #D4B982; }"
        "QMenu { background-color: #111111; border: 0.5px solid rgba(30,30,30,0.5); padding: 4px; }"
        "QMenu::item { padding: 6px 24px; color: #F5F5F7; }"
        "QMenu::item:selected { background-color: #221C12; color: #F5F5F7; }"
        "QMenu::separator { height: 0.5px; background-color: rgba(30,30,30,0.5); margin: 4px 8px; }"
        "QScrollBar:vertical { width: 6px; background: #0A0A0A; }"
        "QScrollBar::handle:vertical { background: #1E1E1E; border-radius: 3px; min-height: 20px; }"
        "QScrollBar::handle:vertical:hover { background: #515154; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar:horizontal { height: 6px; background: #0A0A0A; }"
        "QScrollBar::handle:horizontal { background: #1E1E1E; border-radius: 3px; min-width: 20px; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"
        "QCheckBox { color: #F5F5F7; spacing: 6px; }"
        "QCheckBox::indicator { width: 16px; height: 16px; border: 0.5px solid rgba(30,30,30,0.5); border-radius: 2px; background-color: #161616; }"
        "QCheckBox::indicator:checked { background-color: #D4B982; border-color: #D4B982; }"
        "QRadioButton { color: #F5F5F7; spacing: 6px; }"
        "QRadioButton::indicator { width: 14px; height: 14px; border: 0.5px solid rgba(30,30,30,0.5); border-radius: 7px; background-color: #161616; }"
        "QRadioButton::indicator:checked { background-color: #D4B982; border-color: #D4B982; }"
        "QToolTip { background-color: #111111; border: 0.5px solid rgba(30,30,30,0.5); color: #F5F5F7; padding: 4px; }"
        "QSplitter::handle { background-color: rgba(30,30,30,0.5); }"
        "QHeaderView::section { background-color: #111111; color: #86868B; border: none; border-bottom: 0.5px solid rgba(30,30,30,0.5); padding: 4px 8px; }"
        "QTreeView { background-color: #0A0A0A; alternate-background-color: #111111; border: none; outline: none; }"
        "QTreeView::item { color: #F5F5F7; padding: 4px 0; border-bottom: 0.5px solid rgba(30,30,30,0.5); }"
        "QTreeView::item:selected { background-color: #221C12; color: #F5F5F7; }"
        "QTreeView::item:hover { background-color: #161616; }"
        "QListView { background-color: #0A0A0A; border: none; outline: none; }"
        "QProgressBar { background-color: #161616; border: none; border-radius: 2px; height: 4px; text-align: center; color: transparent; }"
        "QProgressBar::chunk { background-color: #D4B982; border-radius: 2px; }"
        "QPushButton { background-color: #161616; border: 0.5px solid rgba(30,30,30,0.5); border-radius: 2px; padding: 6px 16px; color: #F5F5F7; }"
        "QPushButton:hover { background-color: #1E1E1E; border-color: #D4B982; }"
        "QPushButton:pressed { background-color: #221C12; }"
    );
}

inline QString accentStylesheet() {
    return QStringLiteral(
        "QPushButton { background-color: #D4B982; color: #0A0A0A; "
        "border: none; padding: 6px 16px; font-size: 13px; border-radius: 2px; }"
        "QPushButton:hover { background-color: #DCC396; }"
        "QPushButton:pressed { background-color: #C4A66E; }"
    );
}

}
