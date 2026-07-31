#include "styles.h"

namespace Styles {

// ── Light mode ────────────────────────────────────────────────────────────────
QString getStyleSheet() {
    return R"(
QMainWindow {
    background-color: #f0faff;
}

QToolBar {
    background-color: #ffffff;
    border-bottom: 1px solid #caf0f8;
    spacing: 10px;
    padding: 5px;
}

QLineEdit {
    background-color: #ffffff;
    color: #333333;
    border: 1px solid #ade8f4;
    border-radius: 15px;
    padding: 5px 15px;
    font-size: 14px;
}

QLineEdit:focus {
    border: 1px solid #00b4d8;
    background-color: #ffffff;
}

QPushButton {
    background-color: #0077b6;
    color: white;
    border: none;
    border-radius: 4px;
    padding: 6px 12px;
    font-weight: bold;
}

QPushButton:hover {
    background-color: #0096c7;
}

QPushButton:pressed {
    background-color: #023e8a;
}

QTabWidget::pane {
    border-top: 1px solid #caf0f8;
}

QTabBar::tab {
    background-color: #e0f2fe;
    color: #023e8a;
    padding: 8px 15px;
    border-top-left-radius: 4px;
    border-top-right-radius: 4px;
    margin-right: 2px;
    border: 1px solid transparent;
}

QTabBar::tab:selected {
    background-color: #ffffff;
    color: #0077b6;
    border-bottom: 2px solid #0077b6;
    border-top: 1px solid #caf0f8;
    border-left: 1px solid #caf0f8;
    border-right: 1px solid #caf0f8;
}

QTabBar::tab:hover {
    background-color: #caf0f8;
}

QMenu {
    background-color: #ffffff;
    color: #333333;
    border: 1px solid #ade8f4;
}

QMenu::item:selected {
    background-color: #e0f2fe;
    color: #023e8a;
}

QDialog {
    background-color: #f0faff;
    color: #333333;
}

QLabel {
    color: #333333;
}

QTextEdit {
    background-color: #ffffff;
    color: #333333;
    border: 1px solid #ade8f4;
}

QProgressBar {
    border: 1px solid #ade8f4;
    border-radius: 5px;
    text-align: center;
    color: #333333;
}

QProgressBar::chunk {
    background-color: #0096c7;
    width: 20px;
}

QScrollArea {
    border: none;
    background-color: transparent;
}

QScrollBar:vertical {
    background-color: #f0faff;
    width: 12px;
    margin: 0px;
}

QScrollBar::handle:vertical {
    background-color: #caf0f8;
    min-height: 20px;
    border-radius: 6px;
}

QScrollBar::handle:vertical:hover {
    background-color: #90e0ef;
}

QListWidget {
    background-color: #ffffff;
    color: #333333;
    border: 1px solid #ade8f4;
}

QDialog#ToolDialog {
    border: 1px solid #0077b6;
    border-radius: 10px;
}

QDialog#ToolDialog QLabel#Title {
    font-size: 18px;
    font-weight: bold;
    color: #0077b6;
    margin-bottom: 5px;
    border-bottom: 2px solid #ade8f4;
    padding-bottom: 5px;
}

QDialog#ToolDialog QTextEdit#ResultBox {
    background-color: #f8f9fa;
    border: 1px solid #caf0f8;
    border-radius: 5px;
    font-size: 13px;
}
)";
}

// ── Dark mode — bluish cyan glassmorphism ─────────────────────────────────────
QString getDarkStyleSheet() {
    return R"(
/* ── Base ── */
QMainWindow {
    background-color: #050a14;
}

/* ── Toolbar — glass panel ── */
QToolBar {
    background-color: rgba(10, 30, 60, 0.82);
    border-bottom: 1px solid rgba(0, 210, 255, 0.25);
    spacing: 8px;
    padding: 4px 8px;
}

QToolBar QToolButton {
    background: transparent;
    color: #00d2ff;
    border: none;
    border-radius: 4px;
    padding: 4px 8px;
    font-size: 14px;
    font-weight: bold;
}
QToolBar QToolButton:hover {
    background: rgba(0, 210, 255, 0.15);
    color: #ffffff;
}
QToolBar QToolButton:pressed {
    background: rgba(0, 210, 255, 0.30);
}

/* ── URL bar ── */
QLineEdit {
    background-color: rgba(0, 20, 45, 0.75);
    color: #e8f8ff;
    border: 1px solid rgba(0, 210, 255, 0.35);
    border-radius: 15px;
    padding: 5px 15px;
    font-size: 14px;
    selection-background-color: #0096c7;
    selection-color: #ffffff;
}
QLineEdit:focus {
    border: 1px solid rgba(0, 210, 255, 0.80);
    background-color: rgba(0, 20, 45, 0.90);
    color: #ffffff;
}
QLineEdit::placeholder {
    color: #4a7a99;
}

/* ── Buttons ── */
QPushButton {
    background-color: rgba(0, 100, 160, 0.70);
    color: #e8f8ff;
    border: 1px solid rgba(0, 210, 255, 0.30);
    border-radius: 5px;
    padding: 6px 14px;
    font-weight: bold;
    font-size: 13px;
}
QPushButton:hover {
    background-color: rgba(0, 160, 220, 0.75);
    border-color: rgba(0, 210, 255, 0.65);
    color: #ffffff;
}
QPushButton:pressed {
    background-color: rgba(0, 80, 140, 0.90);
}
QPushButton:disabled {
    background-color: rgba(30, 40, 55, 0.60);
    color: #3a5a6a;
    border-color: rgba(0, 100, 140, 0.20);
}

/* ── Tabs ── */
QTabWidget::pane {
    border: none;
    background-color: #050a14;
}
QTabBar::tab {
    background-color: rgba(0, 20, 50, 0.70);
    color: #7ac8e8;
    padding: 7px 14px;
    border-top-left-radius: 5px;
    border-top-right-radius: 5px;
    margin-right: 2px;
    border: 1px solid rgba(0, 150, 200, 0.20);
    font-size: 12px;
}
QTabBar::tab:selected {
    background-color: rgba(0, 60, 110, 0.85);
    color: #00d2ff;
    border-bottom: 2px solid #00d2ff;
    border-top: 1px solid rgba(0, 210, 255, 0.40);
    border-left: 1px solid rgba(0, 210, 255, 0.25);
    border-right: 1px solid rgba(0, 210, 255, 0.25);
}
QTabBar::tab:hover:!selected {
    background-color: rgba(0, 40, 80, 0.80);
    color: #a0e0f8;
}
QTabBar::close-button {
    image: none;
    subcontrol-position: right;
}

/* ── Menus ── */
QMenu {
    background-color: rgba(5, 18, 38, 0.95);
    color: #d0eeff;
    border: 1px solid rgba(0, 180, 220, 0.30);
    border-radius: 6px;
    padding: 4px;
    font-size: 13px;
}
QMenu::item {
    padding: 6px 20px 6px 14px;
    border-radius: 4px;
}
QMenu::item:selected {
    background-color: rgba(0, 140, 200, 0.45);
    color: #ffffff;
}
QMenu::separator {
    height: 1px;
    background-color: rgba(0, 180, 220, 0.20);
    margin: 3px 8px;
}

/* ── Dialogs ── */
QDialog {
    background-color: #070d1c;
    color: #d0eeff;
}

/* ── Labels — always readable ── */
QLabel {
    color: #d0eeff;
    font-size: 13px;
}

/* ── Text edits ── */
QTextEdit, QPlainTextEdit {
    background-color: rgba(0, 15, 35, 0.85);
    color: #e0f4ff;
    border: 1px solid rgba(0, 180, 220, 0.30);
    border-radius: 4px;
    selection-background-color: #0077aa;
}

/* ── Progress bar ── */
QProgressBar {
    background-color: rgba(0, 20, 45, 0.70);
    border: 1px solid rgba(0, 180, 220, 0.30);
    border-radius: 5px;
    text-align: center;
    color: #d0eeff;
    font-size: 12px;
}
QProgressBar::chunk {
    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 #006494, stop:1 #00d2ff);
    border-radius: 4px;
}

/* ── Scrollbars ── */
QScrollBar:vertical {
    background-color: rgba(0, 15, 35, 0.50);
    width: 10px;
    margin: 0;
    border-radius: 5px;
}
QScrollBar::handle:vertical {
    background-color: rgba(0, 150, 200, 0.45);
    min-height: 24px;
    border-radius: 5px;
}
QScrollBar::handle:vertical:hover {
    background-color: rgba(0, 210, 255, 0.60);
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal {
    background-color: rgba(0, 15, 35, 0.50);
    height: 10px;
    border-radius: 5px;
}
QScrollBar::handle:horizontal {
    background-color: rgba(0, 150, 200, 0.45);
    min-width: 24px;
    border-radius: 5px;
}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

/* ── List/Tree widgets ── */
QListWidget, QTreeWidget {
    background-color: rgba(0, 12, 30, 0.80);
    color: #d0eeff;
    border: 1px solid rgba(0, 150, 200, 0.25);
    border-radius: 4px;
    alternate-background-color: rgba(0, 20, 45, 0.60);
    font-size: 13px;
}
QListWidget::item, QTreeWidget::item {
    padding: 4px 6px;
    border-radius: 3px;
    color: #d0eeff;
}
QListWidget::item:selected, QTreeWidget::item:selected {
    background-color: rgba(0, 140, 200, 0.50);
    color: #ffffff;
}
QListWidget::item:hover:!selected, QTreeWidget::item:hover:!selected {
    background-color: rgba(0, 80, 130, 0.35);
}

/* ── Header view (tree columns) ── */
QHeaderView::section {
    background-color: rgba(0, 25, 55, 0.85);
    color: #7ac8e8;
    border: none;
    border-bottom: 1px solid rgba(0, 180, 220, 0.25);
    padding: 5px 6px;
    font-size: 12px;
}

/* ── ComboBox ── */
QComboBox {
    background-color: rgba(0, 20, 45, 0.75);
    color: #d0eeff;
    border: 1px solid rgba(0, 180, 220, 0.30);
    border-radius: 4px;
    padding: 4px 10px;
    font-size: 13px;
}
QComboBox:hover { border-color: rgba(0, 210, 255, 0.55); }
QComboBox::drop-down { border: none; width: 18px; }
QComboBox QAbstractItemView {
    background-color: rgba(5, 18, 40, 0.97);
    color: #d0eeff;
    selection-background-color: rgba(0, 140, 200, 0.50);
    border: 1px solid rgba(0, 180, 220, 0.30);
}

/* ── SpinBox ── */
QSpinBox, QDoubleSpinBox {
    background-color: rgba(0, 20, 45, 0.75);
    color: #d0eeff;
    border: 1px solid rgba(0, 180, 220, 0.30);
    border-radius: 4px;
    padding: 4px 6px;
}

/* ── CheckBox / GroupBox ── */
QCheckBox { color: #d0eeff; }
QGroupBox {
    color: #7ac8e8;
    border: 1px solid rgba(0, 180, 220, 0.25);
    border-radius: 5px;
    margin-top: 10px;
    padding-top: 6px;
    font-size: 13px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    padding: 0 6px;
    color: #00d2ff;
}

/* ── Splitter ── */
QSplitter::handle { background-color: rgba(0, 150, 200, 0.20); }

/* ── Scroll area ── */
QScrollArea { border: none; background: transparent; }

/* ── Tab content area ── */
QTabWidget QWidget { background-color: #050a14; }

/* ── Tool dialogs ── */
QDialog#ToolDialog {
    border: 1px solid rgba(0, 210, 255, 0.30);
    border-radius: 10px;
    background-color: #070d1c;
}
QDialog#ToolDialog QLabel#Title {
    font-size: 17px;
    font-weight: bold;
    color: #00d2ff;
    margin-bottom: 5px;
    border-bottom: 1px solid rgba(0, 210, 255, 0.25);
    padding-bottom: 5px;
}
QDialog#ToolDialog QTextEdit#ResultBox {
    background-color: rgba(0, 15, 35, 0.85);
    border: 1px solid rgba(0, 180, 220, 0.25);
    border-radius: 5px;
    font-size: 13px;
    color: #e0f4ff;
}
)";
}

} // namespace Styles
