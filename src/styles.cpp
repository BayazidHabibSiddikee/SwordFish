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
/* ═══════════════════════════════════════════════════════════════════
   SwordFish Dark Mode — SwordWM One Dark Palette
   BG: #282c34  BG2: #21252b  DIM: #3e4451  FG: #abb2bf
   CYAN: #61afef  GREEN: #98c379  AMBER: #e5c07b  RED: #e06c75
   ═══════════════════════════════════════════════════════════════════ */

/* ── Base ── */
QMainWindow {
    background-color: #282c34;
}

/* ── Toolbar ── */
QToolBar {
    background-color: #21252b;
    border-bottom: 1px solid #3e4451;
    spacing: 8px;
    padding: 4px 8px;
}

QToolBar QToolButton {
    background: transparent;
    color: #abb2bf;
    border: none;
    border-radius: 4px;
    padding: 4px 8px;
    font-size: 14px;
    font-weight: bold;
}
QToolBar QToolButton:hover {
    background: #3e4451;
    color: #abb2bf;
}
QToolBar QToolButton:pressed {
    background: #4b5263;
}

/* ── URL bar ── */
QLineEdit {
    background-color: #21252b;
    color: #abb2bf;
    border: 1px solid #3e4451;
    border-radius: 15px;
    padding: 5px 15px;
    font-size: 14px;
    selection-background-color: #3e4451;
    selection-color: #abb2bf;
}
QLineEdit:focus {
    border: 1px solid #61afef;
    background-color: #2c313c;
    color: #abb2bf;
}
QLineEdit::placeholder {
    color: #5c6370;
}

/* ── Buttons ── */
QPushButton {
    background-color: #3e4451;
    color: #abb2bf;
    border: 1px solid #4b5263;
    border-radius: 5px;
    padding: 6px 14px;
    font-weight: bold;
    font-size: 13px;
}
QPushButton:hover {
    background-color: #4b5263;
    border-color: #61afef;
    color: #abb2bf;
}
QPushButton:pressed {
    background-color: #2c313c;
}
QPushButton:disabled {
    background-color: #21252b;
    color: #5c6370;
    border-color: #3e4451;
}

/* ── Tabs ── */
QTabWidget::pane {
    border: none;
    background-color: #282c34;
}
QTabBar {
    background-color: #21252b;
}
QTabBar::tab {
    background-color: #21252b;
    color: #5c6370;
    padding: 7px 10px 7px 14px;
    border: none;
    border-bottom: 2px solid transparent;
    margin-right: 1px;
    font-size: 12px;
}
QTabBar::tab:selected {
    background-color: #282c34;
    color: #61afef;
    border-bottom: 2px solid #61afef;
}
QTabBar::tab:hover:!selected {
    background-color: #2c313c;
    color: #abb2bf;
}
QTabBar::close-button {
    image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='10' height='10' viewBox='0 0 10 10'%3E%3Cpath d='M1 1l8 8M9 1l-8 8' stroke='%235c6370' stroke-width='1.5' stroke-linecap='round'/%3E%3C/svg%3E");
    subcontrol-position: right;
    padding: 2px;
    margin: 2px;
}
QTabBar::close-button:hover {
    image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='10' height='10' viewBox='0 0 10 10'%3E%3Cpath d='M1 1l8 8M9 1l-8 8' stroke='%23e06c75' stroke-width='1.5' stroke-linecap='round'/%3E%3C/svg%3E");
    background-color: rgba(224, 108, 117, 0.2);
    border-radius: 3px;
}

/* ── Menus ── */
QMenu {
    background-color: #21252b;
    color: #abb2bf;
    border: 1px solid #3e4451;
    border-radius: 6px;
    padding: 4px;
    font-size: 13px;
}
QMenu::item {
    padding: 6px 20px 6px 14px;
    border-radius: 4px;
}
QMenu::item:selected {
    background-color: #3e4451;
    color: #abb2bf;
}
QMenu::separator {
    height: 1px;
    background-color: #3e4451;
    margin: 3px 8px;
}

/* ── Dialogs ── */
QDialog {
    background-color: #282c34;
    color: #abb2bf;
}

/* ── Labels ── */
QLabel {
    color: #abb2bf;
    font-size: 13px;
}

/* ── Text edits ── */
QTextEdit, QPlainTextEdit {
    background-color: #21252b;
    color: #abb2bf;
    border: 1px solid #3e4451;
    border-radius: 4px;
    selection-background-color: #3e4451;
}

/* ── Progress bar ── */
QProgressBar {
    background-color: #21252b;
    border: 1px solid #3e4451;
    border-radius: 5px;
    text-align: center;
    color: #abb2bf;
    font-size: 12px;
}
QProgressBar::chunk {
    background-color: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 #61afef, stop:1 #56b6c2);
    border-radius: 4px;
}

/* ── Scrollbars ── */
QScrollBar:vertical {
    background-color: #282c34;
    width: 10px;
    margin: 0;
    border-radius: 5px;
}
QScrollBar::handle:vertical {
    background-color: #3e4451;
    min-height: 24px;
    border-radius: 5px;
}
QScrollBar::handle:vertical:hover {
    background-color: #4b5263;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal {
    background-color: #282c34;
    height: 10px;
    border-radius: 5px;
}
QScrollBar::handle:horizontal {
    background-color: #3e4451;
    min-width: 24px;
    border-radius: 5px;
}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

/* ── List/Tree widgets ── */
QListWidget, QTreeWidget {
    background-color: transparent;
    color: #abb2bf;
    border: none;
    border-radius: 4px;
    alternate-background-color: transparent;
    font-size: 13px;
}
QListWidget::item, QTreeWidget::item {
    padding: 4px 6px;
    border-radius: 3px;
    color: #abb2bf;
}
QListWidget::item:selected, QTreeWidget::item:selected {
    background-color: #3e4451;
    color: #abb2bf;
}
QListWidget::item:hover:!selected, QTreeWidget::item:hover:!selected {
    background-color: #2c313c;
}

/* ── Header view (tree columns) ── */
QHeaderView::section {
    background-color: #21252b;
    color: #61afef;
    border: none;
    border-bottom: 1px solid #3e4451;
    padding: 5px 6px;
    font-size: 12px;
}

/* ── ComboBox ── */
QComboBox {
    background-color: #21252b;
    color: #abb2bf;
    border: none;
    border-bottom: 1px solid #3e4451;
    border-radius: 0;
    padding: 4px 10px;
    font-size: 13px;
}
QComboBox:hover { border-bottom-color: #61afef; }
QComboBox::drop-down { border: none; width: 18px; }
QComboBox QAbstractItemView {
    background-color: #21252b;
    color: #abb2bf;
    selection-background-color: #3e4451;
    border: 1px solid #3e4451;
}

/* ── SpinBox ── */
QSpinBox, QDoubleSpinBox {
    background-color: #21252b;
    color: #abb2bf;
    border: 1px solid #3e4451;
    border-radius: 4px;
    padding: 4px 6px;
}

/* ── CheckBox / GroupBox ── */
QCheckBox { color: #abb2bf; }
QGroupBox {
    color: #61afef;
    border: 1px solid #3e4451;
    border-radius: 5px;
    margin-top: 10px;
    padding-top: 6px;
    font-size: 13px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    padding: 0 6px;
    color: #61afef;
}

/* ── Splitter ── */
QSplitter::handle { background-color: #3e4451; }

/* ── Scroll area ── */
QScrollArea { border: none; background: transparent; }

/* ── Tab content area ── */
QTabWidget QWidget {
    background-color: transparent;
}

/* ── Tool dialogs ── */
QDialog#ToolDialog {
    border: 1px solid #3e4451;
    border-radius: 10px;
    background-color: #282c34;
}
QDialog#ToolDialog QLabel#Title {
    font-size: 17px;
    font-weight: bold;
    color: #61afef;
    margin-bottom: 5px;
    border-bottom: 1px solid #3e4451;
    padding-bottom: 5px;
}
QDialog#ToolDialog QTextEdit#ResultBox {
    background-color: #21252b;
    border: 1px solid #3e4451;
    border-radius: 5px;
    font-size: 13px;
    color: #abb2bf;
}
)";
}

} // namespace Styles
