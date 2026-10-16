#include "ui/CadTheme.h"

QString CadTheme::getStyleSheet(ThemeMode mode) {
    if (mode == ThemeMode::Dark) {
        return QStringLiteral(
            // Global & Windows
            "QMainWindow { background-color: #21252b; color: #d7dae0; font-family: 'Segoe UI', 'Roboto', sans-serif; }"
            "QWidget { font-family: 'Segoe UI', 'Roboto', sans-serif; font-size: 11px; color: #d7dae0; }"
            
            // Menu Bar & Menus
            "QMenuBar { background-color: #1e2227; color: #abb2bf; border-bottom: 1px solid #181a1f; padding: 2px 4px; font-size: 12px; }"
            "QMenuBar::item { background: transparent; padding: 4px 8px; border-radius: 3px; }"
            "QMenuBar::item:selected { background-color: #2c313a; color: #ffffff; }"
            "QMenuBar::item:pressed { background-color: #007acc; color: #ffffff; }"
            "QMenu { background-color: #21252b; color: #d7dae0; border: 1px solid #181a1f; border-radius: 4px; padding: 4px; }"
            "QMenu::item { padding: 5px 24px 5px 20px; border-radius: 3px; }"
            "QMenu::item:selected { background-color: #007acc; color: #ffffff; }"
            "QMenu::separator { height: 1px; background-color: #2c313a; margin: 4px 6px; }"
            "QMenu::icon { padding-left: 6px; }"

            // Toolbars
            "QToolBar { background-color: #21252b; border: none; spacing: 3px; padding: 3px 6px; }"
            "QToolBar::separator { width: 1px; background-color: #313640; margin: 4px 6px; }"

            // ToolButtons
            "QToolButton { background-color: transparent; border: 1px solid transparent; border-radius: 4px; padding: 4px; color: #d7dae0; }"
            "QToolButton:hover { background-color: #2c313a; border: 1px solid #3e4451; color: #ffffff; }"
            "QToolButton:pressed { background-color: #094771; border: 1px solid #007acc; color: #ffffff; }"
            "QToolButton:checked { background-color: #0e639c; border: 1px solid #0098ff; color: #ffffff; }"
            "QToolButton:disabled { color: #5c6370; background-color: transparent; }"

            // Dock Widgets
            "QDockWidget { color: #ffffff; font-weight: bold; border: 1px solid #181a1f; titlebar-close-icon: url(:/icons/delete.svg); }"
            "QDockWidget::title { background-color: #282c34; text-align: left; padding: 6px 8px; border-bottom: 1px solid #181a1f; border-top: 1px solid #313640; font-size: 11px; letter-spacing: 0.5px; }"

            // Tabs
            "QTabWidget::pane { border: 1px solid #181a1f; background-color: #21252b; top: -1px; }"
            "QTabBar::tab { background-color: #1e2227; color: #828997; padding: 6px 14px; border: 1px solid #181a1f; border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; min-width: 60px; font-weight: 500; }"
            "QTabBar::tab:hover { background-color: #282c34; color: #abb2bf; }"
            "QTabBar::tab:selected { background-color: #21252b; color: #00e5ff; border-top: 2px solid #007acc; font-weight: bold; }"

            // Buttons
            "QPushButton { background-color: #2c313a; color: #d7dae0; border: 1px solid #3e4451; border-radius: 3px; padding: 5px 12px; font-weight: 500; }"
            "QPushButton:hover { background-color: #353b45; border-color: #007acc; color: #ffffff; }"
            "QPushButton:pressed { background-color: #0e639c; border-color: #0098ff; }"
            "QPushButton:checked { background-color: #0e639c; border-color: #0098ff; color: #ffffff; }"
            "QPushButton:disabled { background-color: #21252b; color: #5c6370; border-color: #2c313a; }"

            // LineEdits & SpinBoxes
            "QLineEdit { background-color: #1b1d23; color: #ffffff; border: 1px solid #3e4451; border-radius: 3px; padding: 3px 6px; selection-background-color: #007acc; }"
            "QLineEdit:focus { border: 1px solid #007acc; background-color: #1e2227; }"
            "QLineEdit:disabled { background-color: #21252b; color: #5c6370; border-color: #2c313a; }"
            "QSpinBox, QDoubleSpinBox { background-color: #1b1d23; color: #ffffff; border: 1px solid #3e4451; border-radius: 3px; padding: 3px 6px; }"
            "QSpinBox:focus, QDoubleSpinBox:focus { border: 1px solid #007acc; }"

            // ComboBox
            "QComboBox { background-color: #2c313a; color: #d7dae0; border: 1px solid #3e4451; border-radius: 3px; padding: 3px 8px; min-height: 20px; }"
            "QComboBox:hover { border-color: #007acc; }"
            "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 18px; border-left: 1px solid #3e4451; }"
            "QComboBox QAbstractItemView { background-color: #21252b; color: #d7dae0; selection-background-color: #007acc; border: 1px solid #181a1f; }"

            // Tables
            "QTableWidget, QTableView { background-color: #1b1d23; color: #d7dae0; gridline-color: #282c34; border: 1px solid #282c34; selection-background-color: #0e639c; selection-color: #ffffff; alternate-background-color: #1e2227; }"
            "QHeaderView::section { background-color: #282c34; color: #abb2bf; padding: 5px; border: none; border-right: 1px solid #1e2227; border-bottom: 1px solid #1e2227; font-weight: bold; font-size: 11px; }"
            "QTableCornerButton::section { background-color: #282c34; border: none; }"

            // Lists & Trees
            "QListWidget, QTreeWidget { background-color: #1b1d23; color: #d7dae0; border: 1px solid #282c34; selection-background-color: #0e639c; selection-color: #ffffff; }"
            "QListWidget::item:hover, QTreeWidget::item:hover { background-color: #282c34; }"
            "QListWidget::item:selected, QTreeWidget::item:selected { background-color: #0e639c; color: #ffffff; }"

            // Status Bar
            "QStatusBar { background-color: #1e2227; color: #abb2bf; border-top: 1px solid #181a1f; min-height: 24px; font-family: 'Segoe UI', monospace; font-size: 11px; }"
            "QStatusBar::item { border: none; }"

            // ScrollBars
            "QScrollBar:vertical { background: #1e2227; width: 10px; margin: 0px; border: none; }"
            "QScrollBar::handle:vertical { background: #3e4451; min-height: 20px; border-radius: 4px; margin: 2px; }"
            "QScrollBar::handle:vertical:hover { background: #5c6370; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
            "QScrollBar:horizontal { background: #1e2227; height: 10px; margin: 0px; border: none; }"
            "QScrollBar::handle:horizontal { background: #3e4451; min-width: 20px; border-radius: 4px; margin: 2px; }"
            "QScrollBar::handle:horizontal:hover { background: #5c6370; }"
            "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }"

            // Tooltips
            "QToolTip { background-color: #1b1d23; color: #ffffff; border: 1px solid #007acc; border-radius: 3px; padding: 4px 8px; font-size: 11px; }"

            // GroupBoxes
            "QGroupBox { border: 1px solid #313640; border-radius: 4px; margin-top: 10px; padding-top: 8px; font-weight: bold; color: #abb2bf; }"
            "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 4px; }"
        );
    } else {
        // Light Engineering Theme
        return QStringLiteral(
            // Global & Windows
            "QMainWindow { background-color: #f3f3f5; color: #24292f; font-family: 'Segoe UI', 'Roboto', sans-serif; }"
            "QWidget { font-family: 'Segoe UI', 'Roboto', sans-serif; font-size: 11px; color: #24292f; }"

            // Menu Bar & Menus
            "QMenuBar { background-color: #ffffff; color: #24292f; border-bottom: 1px solid #d0d7de; padding: 2px 4px; font-size: 12px; }"
            "QMenuBar::item { background: transparent; padding: 4px 8px; border-radius: 3px; }"
            "QMenuBar::item:selected { background-color: #e1e4e8; color: #0969da; }"
            "QMenuBar::item:pressed { background-color: #0969da; color: #ffffff; }"
            "QMenu { background-color: #ffffff; color: #24292f; border: 1px solid #d0d7de; border-radius: 4px; padding: 4px; }"
            "QMenu::item { padding: 5px 24px 5px 20px; border-radius: 3px; }"
            "QMenu::item:selected { background-color: #0969da; color: #ffffff; }"
            "QMenu::separator { height: 1px; background-color: #d0d7de; margin: 4px 6px; }"
            "QMenu::icon { padding-left: 6px; }"

            // Toolbars
            "QToolBar { background-color: #f6f8fa; border: none; spacing: 3px; padding: 3px 6px; }"
            "QToolBar::separator { width: 1px; background-color: #d0d7de; margin: 4px 6px; }"

            // ToolButtons
            "QToolButton { background-color: transparent; border: 1px solid transparent; border-radius: 4px; padding: 4px; color: #24292f; }"
            "QToolButton:hover { background-color: #e1e4e8; border: 1px solid #c0c6cc; color: #0969da; }"
            "QToolButton:pressed { background-color: #0969da; border: 1px solid #0550ae; color: #ffffff; }"
            "QToolButton:checked { background-color: #ddf4ff; border: 1px solid #0969da; color: #0969da; }"
            "QToolButton:disabled { color: #8c959f; background-color: transparent; }"

            // Dock Widgets
            "QDockWidget { color: #24292f; font-weight: bold; border: 1px solid #d0d7de; titlebar-close-icon: url(:/icons/delete.svg); }"
            "QDockWidget::title { background-color: #f6f8fa; text-align: left; padding: 6px 8px; border-bottom: 1px solid #d0d7de; font-size: 11px; letter-spacing: 0.5px; }"

            // Tabs
            "QTabWidget::pane { border: 1px solid #d0d7de; background-color: #ffffff; top: -1px; }"
            "QTabBar::tab { background-color: #f6f8fa; color: #57606a; padding: 6px 14px; border: 1px solid #d0d7de; border-bottom: none; border-top-left-radius: 4px; border-top-right-radius: 4px; min-width: 60px; font-weight: 500; }"
            "QTabBar::tab:hover { background-color: #ffffff; color: #24292f; }"
            "QTabBar::tab:selected { background-color: #ffffff; color: #0969da; border-top: 2px solid #0969da; font-weight: bold; }"

            // Buttons
            "QPushButton { background-color: #f6f8fa; color: #24292f; border: 1px solid #d0d7de; border-radius: 3px; padding: 5px 12px; font-weight: 500; }"
            "QPushButton:hover { background-color: #f3f4f6; border-color: #0969da; color: #0969da; }"
            "QPushButton:pressed { background-color: #ddf4ff; border-color: #0969da; }"
            "QPushButton:checked { background-color: #ddf4ff; border-color: #0969da; color: #0969da; }"
            "QPushButton:disabled { background-color: #f6f8fa; color: #8c959f; border-color: #e1e4e8; }"

            // LineEdits & SpinBoxes
            "QLineEdit { background-color: #ffffff; color: #24292f; border: 1px solid #d0d7de; border-radius: 3px; padding: 3px 6px; selection-background-color: #0969da; }"
            "QLineEdit:focus { border: 1px solid #0969da; background-color: #ffffff; }"
            "QLineEdit:disabled { background-color: #f6f8fa; color: #8c959f; border-color: #d0d7de; }"
            "QSpinBox, QDoubleSpinBox { background-color: #ffffff; color: #24292f; border: 1px solid #d0d7de; border-radius: 3px; padding: 3px 6px; }"
            "QSpinBox:focus, QDoubleSpinBox:focus { border: 1px solid #0969da; }"

            // ComboBox
            "QComboBox { background-color: #ffffff; color: #24292f; border: 1px solid #d0d7de; border-radius: 3px; padding: 3px 8px; min-height: 20px; }"
            "QComboBox:hover { border-color: #0969da; }"
            "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 18px; border-left: 1px solid #d0d7de; }"
            "QComboBox QAbstractItemView { background-color: #ffffff; color: #24292f; selection-background-color: #0969da; selection-color: #ffffff; border: 1px solid #d0d7de; }"

            // Tables
            "QTableWidget, QTableView { background-color: #ffffff; color: #24292f; gridline-color: #eaeef2; border: 1px solid #d0d7de; selection-background-color: #ddf4ff; selection-color: #0969da; alternate-background-color: #f8fafc; }"
            "QHeaderView::section { background-color: #f6f8fa; color: #57606a; padding: 5px; border: none; border-right: 1px solid #d0d7de; border-bottom: 1px solid #d0d7de; font-weight: bold; font-size: 11px; }"
            "QTableCornerButton::section { background-color: #f6f8fa; border: none; }"

            // Lists & Trees
            "QListWidget, QTreeWidget { background-color: #ffffff; color: #24292f; border: 1px solid #d0d7de; selection-background-color: #ddf4ff; selection-color: #0969da; }"
            "QListWidget::item:hover, QTreeWidget::item:hover { background-color: #f6f8fa; }"
            "QListWidget::item:selected, QTreeWidget::item:selected { background-color: #ddf4ff; color: #0969da; }"

            // Status Bar
            "QStatusBar { background-color: #f6f8fa; color: #57606a; border-top: 1px solid #d0d7de; min-height: 24px; font-family: 'Segoe UI', monospace; font-size: 11px; }"
            "QStatusBar::item { border: none; }"

            // ScrollBars
            "QScrollBar:vertical { background: #f6f8fa; width: 10px; margin: 0px; border: none; }"
            "QScrollBar::handle:vertical { background: #d0d7de; min-height: 20px; border-radius: 4px; margin: 2px; }"
            "QScrollBar::handle:vertical:hover { background: #8c959f; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
            "QScrollBar:horizontal { background: #f6f8fa; height: 10px; margin: 0px; border: none; }"
            "QScrollBar::handle:horizontal { background: #d0d7de; min-width: 20px; border-radius: 4px; margin: 2px; }"
            "QScrollBar::handle:horizontal:hover { background: #8c959f; }"
            "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; }"

            // Tooltips
            "QToolTip { background-color: #ffffff; color: #24292f; border: 1px solid #0969da; border-radius: 3px; padding: 4px 8px; font-size: 11px; }"

            // GroupBoxes
            "QGroupBox { border: 1px solid #d0d7de; border-radius: 4px; margin-top: 10px; padding-top: 8px; font-weight: bold; color: #57606a; }"
            "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; padding: 0 4px; }"
        );
    }
}

QColor CadTheme::canvasBackground(ThemeMode mode) {
    return (mode == ThemeMode::Dark) ? QColor(24, 26, 32) : QColor(248, 250, 252);
}

QColor CadTheme::paperBackground(ThemeMode mode) {
    return (mode == ThemeMode::Dark) ? QColor(32, 35, 43) : QColor(255, 255, 255);
}

QColor CadTheme::paperBorder(ThemeMode mode) {
    return (mode == ThemeMode::Dark) ? QColor(90, 96, 112) : QColor(180, 186, 198);
}

QColor CadTheme::paperLimits(ThemeMode mode) {
    return (mode == ThemeMode::Dark) ? QColor(0, 160, 220, 120) : QColor(9, 105, 218, 100);
}

QColor CadTheme::minorGridColor(ThemeMode mode) {
    return (mode == ThemeMode::Dark) ? QColor(42, 46, 56) : QColor(230, 234, 240);
}

QColor CadTheme::majorGridColor(ThemeMode mode) {
    return (mode == ThemeMode::Dark) ? QColor(62, 68, 84) : QColor(200, 206, 216);
}

QColor CadTheme::xAxisColor(ThemeMode mode) {
    Q_UNUSED(mode);
    return QColor(220, 53, 69, 200);
}

QColor CadTheme::yAxisColor(ThemeMode mode) {
    Q_UNUSED(mode);
    return QColor(40, 167, 69, 200);
}

QColor CadTheme::crosshairColor(ThemeMode mode) {
    return (mode == ThemeMode::Dark) ? QColor(220, 225, 235, 180) : QColor(40, 45, 55, 180);
}

QColor CadTheme::ucsOriginColor(ThemeMode mode) {
    return (mode == ThemeMode::Dark) ? QColor(220, 225, 235) : QColor(40, 45, 55);
}

QColor CadTheme::selectionWindowFill() {
    return QColor(0, 120, 215, 50);
}

QColor CadTheme::selectionWindowBorder() {
    return QColor(0, 120, 215, 220);
}

QColor CadTheme::selectionCrossingFill() {
    return QColor(46, 204, 113, 50);
}

QColor CadTheme::selectionCrossingBorder() {
    return QColor(46, 204, 113, 220);
}

QColor CadTheme::textColor(ThemeMode mode) {
    return (mode == ThemeMode::Dark) ? QColor(220, 225, 235) : QColor(36, 41, 47);
}

QColor CadTheme::secondaryTextColor(ThemeMode mode) {
    return (mode == ThemeMode::Dark) ? QColor(140, 148, 160) : QColor(101, 109, 118);
}
