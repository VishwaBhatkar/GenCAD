#include "io/GencadFile.h"
#include "geometry/LineEntity.h"
#include "geometry/CircleEntity.h"
#include "geometry/RectangleEntity.h"
#include "geometry/ArcEntity.h"
#include "geometry/PolylineEntity.h"
#include "geometry/PolygonEntity.h"
#include "geometry/EllipseEntity.h"
#include "geometry/TextEntity.h"
#include "geometry/DimensionEntity.h"
#include "layers/LayerManager.h"
#include "document/PageSettings.h"
#include "document/UnitSystem.h"
#include "document/GridSettings.h"
#include "document/ConstraintEngine.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

QString GencadFile::toJson(const CadDocument* document) {
    if (!document) return QString();

    QJsonObject rootObj;
    rootObj["format"] = QStringLiteral("gencad");
    rootObj["version"] = QStringLiteral("2.0");
    rootObj["units"] = document->unitSettings().unitSuffix();

    // Document settings
    rootObj["pageSettings"] = document->pageSettings().toJson();
    rootObj["unitSettings"] = document->unitSettings().toJson();
    rootObj["gridSettings"] = document->gridSettings().toJson();
    if (document->constraintEngine()) {
        rootObj["constraints"] = document->constraintEngine()->toJson();
    }

    // Serialize layers
    if (document->layerManager()) {
        rootObj["layers"] = document->layerManager()->toJson();
        rootObj["activeLayer"] = document->layerManager()->activeLayerName();
    }

    // Serialize entities
    QJsonArray entitiesArray;
    for (const auto& entity : document->entities()) {
        if (entity) {
            entitiesArray.append(entity->toJson());
        }
    }
    rootObj["entities"] = entitiesArray;

    QJsonDocument jsonDoc(rootObj);
    return QString::fromUtf8(jsonDoc.toJson(QJsonDocument::Indented));
}

bool GencadFile::fromJson(const QString& jsonString, CadDocument* document, QString* errorMessage) {
    if (!document) {
        if (errorMessage) *errorMessage = QStringLiteral("Invalid document pointer");
        return false;
    }

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonString.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (errorMessage) *errorMessage = parseError.errorString();
        return false;
    }

    if (!jsonDoc.isObject()) {
        if (errorMessage) *errorMessage = QStringLiteral("Invalid GenCAD format: root is not a JSON object");
        return false;
    }

    QJsonObject rootObj = jsonDoc.object();
    if (!rootObj.contains("format") || rootObj["format"].toString() != QStringLiteral("gencad")) {
        if (errorMessage) *errorMessage = QStringLiteral("File is not a valid GenCAD drawing");
        return false;
    }

    document->clear();

    // Restore page settings (or default to A4 Landscape mm for older files)
    if (rootObj.contains("pageSettings") && rootObj["pageSettings"].isObject()) {
        PageSettings page;
        page.fromJson(rootObj["pageSettings"].toObject());
        document->setPageSettings(page);
    }

    // Restore unit settings (or default to mm, precision 2 for older files)
    if (rootObj.contains("unitSettings") && rootObj["unitSettings"].isObject()) {
        UnitSettings units;
        units.fromJson(rootObj["unitSettings"].toObject());
        document->setUnitSettings(units);
    }

    // Restore grid settings
    if (rootObj.contains("gridSettings") && rootObj["gridSettings"].isObject()) {
        GridSettings grid;
        grid.fromJson(rootObj["gridSettings"].toObject());
        document->setGridSettings(grid);
    }

    // Restore constraints
    if (rootObj.contains("constraints") && rootObj["constraints"].isArray() && document->constraintEngine()) {
        document->constraintEngine()->fromJson(rootObj["constraints"].toArray());
    }

    // Deserialize layers
    if (rootObj.contains("layers") && rootObj["layers"].isArray() && document->layerManager()) {
        document->layerManager()->fromJson(rootObj["layers"].toArray());
        if (rootObj.contains("activeLayer")) {
            document->layerManager()->setActiveLayer(rootObj["activeLayer"].toString());
        }
    }

    // Deserialize entities
    if (rootObj.contains("entities") && rootObj["entities"].isArray()) {
        QJsonArray entitiesArray = rootObj["entities"].toArray();
        for (const auto& val : entitiesArray) {
            if (!val.isObject()) continue;
            QJsonObject obj = val.toObject();
            QString typeStr = obj["type"].toString();
            bool ok = false;
            EntityType type = CadEntity::stringToEntityType(typeStr, &ok);
            if (!ok) continue;

            std::shared_ptr<CadEntity> entity;
            switch (type) {
                case EntityType::Line:
                    entity = std::make_shared<LineEntity>();
                    break;
                case EntityType::Circle:
                    entity = std::make_shared<CircleEntity>();
                    break;
                case EntityType::Rectangle:
                    entity = std::make_shared<RectangleEntity>();
                    break;
                case EntityType::Arc:
                    entity = std::make_shared<ArcEntity>();
                    break;
                case EntityType::Polyline:
                    entity = std::make_shared<PolylineEntity>();
                    break;
                case EntityType::Polygon:
                    entity = std::make_shared<PolygonEntity>();
                    break;
                case EntityType::Ellipse:
                    entity = std::make_shared<EllipseEntity>();
                    break;
                case EntityType::Text:
                    entity = std::make_shared<TextEntity>();
                    break;
                case EntityType::Dimension:
                    entity = std::make_shared<DimensionEntity>();
                    break;
            }

            if (entity && entity->fromJson(obj)) {
                document->addEntity(entity);
            }
        }
    }

    document->setModified(false);
    return true;
}

bool GencadFile::save(const QString& filePath, const CadDocument* document, QString* errorMessage) {
    if (!document) {
        if (errorMessage) *errorMessage = QStringLiteral("Invalid document pointer");
        return false;
    }

    if (filePath.trimmed().isEmpty()) {
        if (errorMessage) *errorMessage = QStringLiteral("File path cannot be empty");
        return false;
    }

    // Never save JSON content with a .dxf or .dwg extension
    QFileInfo fileInfo(filePath);
    QString ext = fileInfo.suffix().toLower();
    if (ext == QStringLiteral("dxf") || ext == QStringLiteral("dwg")) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Cannot save GenCAD project with extension .%1. GenCAD native format is .gencad. Use File -> Export Drawing to export standard CAD files.").arg(ext);
        }
        return false;
    }

    QString jsonStr = toJson(document);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        if (errorMessage) *errorMessage = file.errorString();
        return false;
    }

    qint64 bytesWritten = file.write(jsonStr.toUtf8());
    file.close();

    if (bytesWritten == -1) {
        if (errorMessage) *errorMessage = QStringLiteral("Failed to write data to file");
        return false;
    }

    const_cast<CadDocument*>(document)->setFilePath(filePath);
    const_cast<CadDocument*>(document)->setModified(false);
    return true;
}

bool GencadFile::load(const QString& filePath, CadDocument* document, QString* errorMessage) {
    if (!document) {
        if (errorMessage) *errorMessage = QStringLiteral("Invalid document pointer");
        return false;
    }

    if (filePath.trimmed().isEmpty()) {
        if (errorMessage) *errorMessage = QStringLiteral("File path cannot be empty");
        return false;
    }

    // Never load .dxf or .dwg directly as JSON
    QFileInfo fileInfo(filePath);
    QString ext = fileInfo.suffix().toLower();
    if (ext == QStringLiteral("dxf") || ext == QStringLiteral("dwg")) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Cannot open .%1 directly as a GenCAD project. Use Import DXF or File -> Open with DXF filter.").arg(ext);
        }
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) *errorMessage = file.errorString();
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    bool ok = fromJson(QString::fromUtf8(data), document, errorMessage);
    if (ok) {
        document->setFilePath(filePath);
        document->setModified(false);
    }
    return ok;
}
