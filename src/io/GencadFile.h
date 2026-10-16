#pragma once

#include <QString>
#include "document/CadDocument.h"

class GencadFile {
public:
    static bool save(const QString& filePath, const CadDocument* document, QString* errorMessage = nullptr);
    static bool load(const QString& filePath, CadDocument* document, QString* errorMessage = nullptr);

    static QString toJson(const CadDocument* document);
    static bool fromJson(const QString& jsonString, CadDocument* document, QString* errorMessage = nullptr);

    static QString fileExtension() { return QStringLiteral("gencad"); }
    static QString fileFilter() { return QStringLiteral("GenCAD Project (*.gencad)"); }
};
