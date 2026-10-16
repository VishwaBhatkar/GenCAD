#pragma once

#include <QString>
#include <QPointF>
#include <QRectF>
#include <QUuid>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <vector>
#include <memory>
#include "ai/AiDsl.h"
#include "geometry/CadEntity.h"

enum class FloorPlanWallStyle {
    SingleLine,
    DoubleLine
};

enum class FloorPlanRoomType {
    LivingRoom,
    Kitchen,
    DiningRoom,
    MasterBedroom,
    Bedroom,
    Toilet,
    Staircase,
    Corridor,
    Balcony,
    Utility,
    Other
};

struct FloorPlanRoomSpec {
    QString id;
    QString name;
    FloorPlanRoomType type{FloorPlanRoomType::Other};
    QRectF rect;
    double areaSqM{0.0};
    bool hasWindow{true};
    bool hasDoor{true};

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["name"] = name;
        obj["type"] = static_cast<int>(type);
        obj["x"] = rect.x();
        obj["y"] = rect.y();
        obj["width"] = rect.width();
        obj["height"] = rect.height();
        obj["area_m2"] = areaSqM;
        obj["has_window"] = hasWindow;
        obj["has_door"] = hasDoor;
        return obj;
    }

    static FloorPlanRoomSpec fromJson(const QJsonObject& obj) {
        FloorPlanRoomSpec r;
        r.id = obj["id"].toString();
        r.name = obj["name"].toString();
        r.type = static_cast<FloorPlanRoomType>(obj["type"].toInt());
        r.rect = QRectF(obj["x"].toDouble(), obj["y"].toDouble(), obj["width"].toDouble(), obj["height"].toDouble());
        r.areaSqM = obj["area_m2"].toDouble();
        r.hasWindow = obj["has_window"].toBool(true);
        r.hasDoor = obj["has_door"].toBool(true);
        return r;
    }
};

struct FloorPlanWallSegment {
    QPointF p1;
    QPointF p2;
    double thickness{200.0};
    bool isExterior{false};
    QString layer{QStringLiteral("A-WALL")};

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["x1"] = p1.x();
        obj["y1"] = p1.y();
        obj["x2"] = p2.x();
        obj["y2"] = p2.y();
        obj["thickness"] = thickness;
        obj["is_exterior"] = isExterior;
        obj["layer"] = layer;
        return obj;
    }

    static FloorPlanWallSegment fromJson(const QJsonObject& obj) {
        FloorPlanWallSegment w;
        w.p1 = QPointF(obj["x1"].toDouble(), obj["y1"].toDouble());
        w.p2 = QPointF(obj["x2"].toDouble(), obj["y2"].toDouble());
        w.thickness = obj["thickness"].toDouble(200.0);
        w.isExterior = obj["is_exterior"].toBool();
        w.layer = obj["layer"].toString(QStringLiteral("A-WALL"));
        return w;
    }
};

struct FloorPlanDoorSpec {
    QString id;
    QPointF hinge;
    QPointF strike;
    double width{900.0};
    QPointF swingCenter;
    double swingRadius{900.0};
    double startAngleDeg{0.0};
    double sweepAngleDeg{90.0};
    QString targetRoom;
    QString layer{QStringLiteral("A-DOOR")};

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["hinge_x"] = hinge.x();
        obj["hinge_y"] = hinge.y();
        obj["strike_x"] = strike.x();
        obj["strike_y"] = strike.y();
        obj["width"] = width;
        obj["swing_cx"] = swingCenter.x();
        obj["swing_cy"] = swingCenter.y();
        obj["swing_radius"] = swingRadius;
        obj["start_angle"] = startAngleDeg;
        obj["sweep_angle"] = sweepAngleDeg;
        obj["target_room"] = targetRoom;
        obj["layer"] = layer;
        return obj;
    }

    static FloorPlanDoorSpec fromJson(const QJsonObject& obj) {
        FloorPlanDoorSpec d;
        d.id = obj["id"].toString();
        d.hinge = QPointF(obj["hinge_x"].toDouble(), obj["hinge_y"].toDouble());
        d.strike = QPointF(obj["strike_x"].toDouble(), obj["strike_y"].toDouble());
        d.width = obj["width"].toDouble(900.0);
        d.swingCenter = QPointF(obj["swing_cx"].toDouble(), obj["swing_cy"].toDouble());
        d.swingRadius = obj["swing_radius"].toDouble(d.width);
        d.startAngleDeg = obj["start_angle"].toDouble(0.0);
        d.sweepAngleDeg = obj["sweep_angle"].toDouble(90.0);
        d.targetRoom = obj["target_room"].toString();
        d.layer = obj["layer"].toString(QStringLiteral("A-DOOR"));
        return d;
    }
};

struct FloorPlanWindowSpec {
    QString id;
    QPointF p1;
    QPointF p2;
    double width{1200.0};
    QString wallOrientation; // "north", "south", "east", "west"
    QString layer{QStringLiteral("A-GLAZ")};

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["x1"] = p1.x();
        obj["y1"] = p1.y();
        obj["x2"] = p2.x();
        obj["y2"] = p2.y();
        obj["width"] = width;
        obj["wall"] = wallOrientation;
        obj["layer"] = layer;
        return obj;
    }

    static FloorPlanWindowSpec fromJson(const QJsonObject& obj) {
        FloorPlanWindowSpec w;
        w.id = obj["id"].toString();
        w.p1 = QPointF(obj["x1"].toDouble(), obj["y1"].toDouble());
        w.p2 = QPointF(obj["x2"].toDouble(), obj["y2"].toDouble());
        w.width = obj["width"].toDouble(1200.0);
        w.wallOrientation = obj["wall"].toString();
        w.layer = obj["layer"].toString(QStringLiteral("A-GLAZ"));
        return w;
    }
};

struct FloorPlanStairSpec {
    QString id;
    QRectF bounds;
    int stepCount{14};
    QString direction{QStringLiteral("UP")};
    QString layer{QStringLiteral("A-FLOR-STRS")};

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = id;
        obj["x"] = bounds.x();
        obj["y"] = bounds.y();
        obj["width"] = bounds.width();
        obj["height"] = bounds.height();
        obj["step_count"] = stepCount;
        obj["direction"] = direction;
        obj["layer"] = layer;
        return obj;
    }

    static FloorPlanStairSpec fromJson(const QJsonObject& obj) {
        FloorPlanStairSpec s;
        s.id = obj["id"].toString();
        s.bounds = QRectF(obj["x"].toDouble(), obj["y"].toDouble(), obj["width"].toDouble(), obj["height"].toDouble());
        s.stepCount = obj["step_count"].toInt(14);
        s.direction = obj["direction"].toString(QStringLiteral("UP"));
        s.layer = obj["layer"].toString(QStringLiteral("A-FLOR-STRS"));
        return s;
    }
};

struct FloorPlanDimensionSpec {
    QPointF p1;
    QPointF p2;
    QPointF dimPoint;
    QString label;
    QString layer{QStringLiteral("A-ANNO-DIMS")};

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["x1"] = p1.x();
        obj["y1"] = p1.y();
        obj["x2"] = p2.x();
        obj["y2"] = p2.y();
        obj["dim_x"] = dimPoint.x();
        obj["dim_y"] = dimPoint.y();
        obj["label"] = label;
        obj["layer"] = layer;
        return obj;
    }

    static FloorPlanDimensionSpec fromJson(const QJsonObject& obj) {
        FloorPlanDimensionSpec d;
        d.p1 = QPointF(obj["x1"].toDouble(), obj["y1"].toDouble());
        d.p2 = QPointF(obj["x2"].toDouble(), obj["y2"].toDouble());
        d.dimPoint = QPointF(obj["dim_x"].toDouble(), obj["dim_y"].toDouble());
        d.label = obj["label"].toString();
        d.layer = obj["layer"].toString(QStringLiteral("A-ANNO-DIMS"));
        return d;
    }
};

struct FloorPlanLabelSpec {
    QPointF position;
    QString text;
    double height{180.0};
    QString layer{QStringLiteral("A-ANNO-TEXT")};

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["x"] = position.x();
        obj["y"] = position.y();
        obj["text"] = text;
        obj["height"] = height;
        obj["layer"] = layer;
        return obj;
    }

    static FloorPlanLabelSpec fromJson(const QJsonObject& obj) {
        FloorPlanLabelSpec l;
        l.position = QPointF(obj["x"].toDouble(), obj["y"].toDouble());
        l.text = obj["text"].toString();
        l.height = obj["height"].toDouble(180.0);
        l.layer = obj["layer"].toString(QStringLiteral("A-ANNO-TEXT"));
        return l;
    }
};

struct FloorPlanConfig {
    QString name{QStringLiteral("Residential Ground Floor")};
    double width{10000.0};  // in mm
    double depth{12000.0};  // in mm
    FloorPlanWallStyle wallStyle{FloorPlanWallStyle::DoubleLine};
    double exteriorWallThickness{200.0}; // in mm
    double interiorWallThickness{100.0}; // in mm
    QString units{QStringLiteral("mm")};
    int bedroomCount{3};
    int toiletCount{2};
    bool hasLivingRoom{true};
    bool hasKitchen{true};
    bool hasDining{true};
    bool hasStairs{true};
    bool hasDoors{true};
    bool hasWindows{true};
    bool hasDimensions{true};
    bool hasLabels{true};
    double doorWidth{900.0};
    double windowWidth{1200.0};

    QJsonObject toJson() const {
        QJsonObject obj;
        obj["name"] = name;
        obj["width"] = width;
        obj["depth"] = depth;
        obj["wall_style"] = (wallStyle == FloorPlanWallStyle::DoubleLine ? QStringLiteral("double_line") : QStringLiteral("single_line"));
        obj["exterior_wall_thickness"] = exteriorWallThickness;
        obj["interior_wall_thickness"] = interiorWallThickness;
        obj["units"] = units;
        obj["bedroom_count"] = bedroomCount;
        obj["toilet_count"] = toiletCount;
        obj["has_living_room"] = hasLivingRoom;
        obj["has_kitchen"] = hasKitchen;
        obj["has_dining"] = hasDining;
        obj["has_stairs"] = hasStairs;
        obj["has_doors"] = hasDoors;
        obj["has_windows"] = hasWindows;
        obj["has_dimensions"] = hasDimensions;
        obj["has_labels"] = hasLabels;
        obj["door_width"] = doorWidth;
        obj["window_width"] = windowWidth;
        return obj;
    }

    static FloorPlanConfig fromJson(const QJsonObject& obj) {
        FloorPlanConfig c;
        c.name = obj["name"].toString(QStringLiteral("Residential Ground Floor"));
        c.width = obj["width"].toDouble(10000.0);
        c.depth = obj["depth"].toDouble(12000.0);
        c.wallStyle = (obj["wall_style"].toString() == QStringLiteral("single_line") ? FloorPlanWallStyle::SingleLine : FloorPlanWallStyle::DoubleLine);
        c.exteriorWallThickness = obj["exterior_wall_thickness"].toDouble(200.0);
        c.interiorWallThickness = obj["interior_wall_thickness"].toDouble(100.0);
        c.units = obj["units"].toString(QStringLiteral("mm"));
        c.bedroomCount = obj["bedroom_count"].toInt(3);
        c.toiletCount = obj["toilet_count"].toInt(2);
        c.hasLivingRoom = obj["has_living_room"].toBool(true);
        c.hasKitchen = obj["has_kitchen"].toBool(true);
        c.hasDining = obj["has_dining"].toBool(true);
        c.hasStairs = obj["has_stairs"].toBool(true);
        c.hasDoors = obj["has_doors"].toBool(true);
        c.hasWindows = obj["has_windows"].toBool(true);
        c.hasDimensions = obj["has_dimensions"].toBool(true);
        c.hasLabels = obj["has_labels"].toBool(true);
        c.doorWidth = obj["door_width"].toDouble(900.0);
        c.windowWidth = obj["window_width"].toDouble(1200.0);
        return c;
    }
};

struct FloorPlanLayout {
    FloorPlanConfig config;
    std::vector<FloorPlanRoomSpec> rooms;
    std::vector<FloorPlanWallSegment> walls;
    std::vector<FloorPlanDoorSpec> doors;
    std::vector<FloorPlanWindowSpec> windows;
    std::vector<FloorPlanStairSpec> stairs;
    std::vector<FloorPlanDimensionSpec> dimensions;
    std::vector<FloorPlanLabelSpec> labels;
    double totalAreaSqM{0.0};

    QJsonObject toJson() const {
        QJsonObject root;
        root["$schema"] = QStringLiteral("https://gencad.org/schemas/v1/floorplan.json");
        root["version"] = QStringLiteral("1.0");
        root["config"] = config.toJson();
        root["total_area_m2"] = totalAreaSqM;

        QJsonArray roomsArr;
        for (const auto& r : rooms) roomsArr.append(r.toJson());
        root["rooms"] = roomsArr;

        QJsonArray wallsArr;
        for (const auto& w : walls) wallsArr.append(w.toJson());
        root["walls"] = wallsArr;

        QJsonArray doorsArr;
        for (const auto& d : doors) doorsArr.append(d.toJson());
        root["doors"] = doorsArr;

        QJsonArray winArr;
        for (const auto& w : windows) winArr.append(w.toJson());
        root["windows"] = winArr;

        QJsonArray stairsArr;
        for (const auto& s : stairs) stairsArr.append(s.toJson());
        root["stairs"] = stairsArr;

        QJsonArray dimsArr;
        for (const auto& dm : dimensions) dimsArr.append(dm.toJson());
        root["dimensions"] = dimsArr;

        QJsonArray labelsArr;
        for (const auto& l : labels) labelsArr.append(l.toJson());
        root["labels"] = labelsArr;

        return root;
    }

    static bool fromJson(const QJsonObject& json, FloorPlanLayout* outLayout, QString* errorMessage = nullptr) {
        if (!outLayout) {
            if (errorMessage) *errorMessage = QStringLiteral("Null output pointer");
            return false;
        }

        if (json.contains("config") && json["config"].isObject()) {
            outLayout->config = FloorPlanConfig::fromJson(json["config"].toObject());
        }
        outLayout->totalAreaSqM = json["total_area_m2"].toDouble();

        outLayout->rooms.clear();
        for (const auto& val : json["rooms"].toArray()) {
            outLayout->rooms.push_back(FloorPlanRoomSpec::fromJson(val.toObject()));
        }

        outLayout->walls.clear();
        for (const auto& val : json["walls"].toArray()) {
            outLayout->walls.push_back(FloorPlanWallSegment::fromJson(val.toObject()));
        }

        outLayout->doors.clear();
        for (const auto& val : json["doors"].toArray()) {
            outLayout->doors.push_back(FloorPlanDoorSpec::fromJson(val.toObject()));
        }

        outLayout->windows.clear();
        for (const auto& val : json["windows"].toArray()) {
            outLayout->windows.push_back(FloorPlanWindowSpec::fromJson(val.toObject()));
        }

        outLayout->stairs.clear();
        for (const auto& val : json["stairs"].toArray()) {
            outLayout->stairs.push_back(FloorPlanStairSpec::fromJson(val.toObject()));
        }

        outLayout->dimensions.clear();
        for (const auto& val : json["dimensions"].toArray()) {
            outLayout->dimensions.push_back(FloorPlanDimensionSpec::fromJson(val.toObject()));
        }

        outLayout->labels.clear();
        for (const auto& val : json["labels"].toArray()) {
            outLayout->labels.push_back(FloorPlanLabelSpec::fromJson(val.toObject()));
        }

        return true;
    }

    static bool fromJsonString(const QString& jsonString, FloorPlanLayout* outLayout, QString* errorMessage = nullptr) {
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8(), &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            if (errorMessage) *errorMessage = err.errorString();
            return false;
        }
        return fromJson(doc.object(), outLayout, errorMessage);
    }

    AiProposal toAiProposal() const;
    std::vector<std::shared_ptr<CadEntity>> instantiateEntities() const;
};
