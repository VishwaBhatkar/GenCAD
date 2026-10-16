#pragma once

#include <QString>
#include <QStringList>

class CadDocument;

class DxfFile {
public:
    static bool save(const QString& filePath, const CadDocument* document, QString* errorMessage = nullptr, QStringList* warnings = nullptr);
    static bool load(const QString& filePath, CadDocument* document, QString* errorMessage = nullptr, QStringList* warnings = nullptr);

    static int rgbToAci(const class QColor& color);
    static class QColor aciToRgb(int aci);
};
