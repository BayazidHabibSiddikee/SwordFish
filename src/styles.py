
STYLE_SHEET = """
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

/* Compact Tool Dialog Styles */
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
"""

def apply_theme(app):
    app.setStyleSheet(STYLE_SHEET)
