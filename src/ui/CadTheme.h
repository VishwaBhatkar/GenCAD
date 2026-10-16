#pragma once

#include <QString>
#include <QColor>

class CadTheme {
public:
    enum class ThemeMode {
        Dark,
        Light
    };

    static QString getStyleSheet(ThemeMode mode);

    // Canvas visual colors
    static QColor canvasBackground(ThemeMode mode);
    static QColor paperBackground(ThemeMode mode);
    static QColor paperBorder(ThemeMode mode);
    static QColor paperLimits(ThemeMode mode);
    static QColor minorGridColor(ThemeMode mode);
    static QColor majorGridColor(ThemeMode mode);
    static QColor xAxisColor(ThemeMode mode);
    static QColor yAxisColor(ThemeMode mode);
    static QColor crosshairColor(ThemeMode mode);
    static QColor ucsOriginColor(ThemeMode mode);
    static QColor selectionWindowFill();
    static QColor selectionWindowBorder();
    static QColor selectionCrossingFill();
    static QColor selectionCrossingBorder();
    static QColor textColor(ThemeMode mode);
    static QColor secondaryTextColor(ThemeMode mode);
};
