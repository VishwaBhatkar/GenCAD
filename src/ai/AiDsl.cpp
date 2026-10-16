#include "ai/AiDsl.h"
#include <QJsonDocument>

static QJsonObject pointToJson(const QPointF& pt) {
    QJsonObject obj;
    obj["x"] = pt.x();
    obj["y"] = pt.y();
    return obj;
}

static QPointF pointFromJson(const QJsonValue& val, bool* ok = nullptr) {
    if (ok) *ok = false;
    if (val.isArray()) {
        QJsonArray arr = val.toArray();
        if (arr.size() >= 2) {
            if (ok) *ok = true;
            return QPointF(arr[0].toDouble(), arr[1].toDouble());
        }
    } else if (val.isObject()) {
        QJsonObject obj = val.toObject();
        if (obj.contains("x") && obj.contains("y")) {
            if (ok) *ok = true;
            return QPointF(obj["x"].toDouble(), obj["y"].toDouble());
        }
    }
    return QPointF(0.0, 0.0);
}

QString AiOperation::operationTypeToString(AiOperationType type) {
    switch (type) {
        case AiOperationType::CreateLine: return QStringLiteral("create_line");
        case AiOperationType::CreateCircle: return QStringLiteral("create_circle");
        case AiOperationType::CreateRectangle: return QStringLiteral("create_rectangle");
        case AiOperationType::CreateArc: return QStringLiteral("create_arc");
        case AiOperationType::CreatePolyline: return QStringLiteral("create_polyline");
        case AiOperationType::CreateText: return QStringLiteral("create_text");
        case AiOperationType::CreateDimension: return QStringLiteral("create_dimension");
        case AiOperationType::ModifyMove: return QStringLiteral("modify_move");
        case AiOperationType::ModifyRotate: return QStringLiteral("modify_rotate");
        case AiOperationType::ModifyMirror: return QStringLiteral("modify_mirror");
        case AiOperationType::DeleteEntity: return QStringLiteral("delete_entity");
        case AiOperationType::AddLayer: return QStringLiteral("add_layer");
        case AiOperationType::SetEntityLayer: return QStringLiteral("set_entity_layer");
        default: return QStringLiteral("unknown");
    }
}

AiOperationType AiOperation::stringToOperationType(const QString& str) {
    QString s = str.toLower().trimmed();
    if (s == QStringLiteral("create_line") || s == QStringLiteral("line")) return AiOperationType::CreateLine;
    if (s == QStringLiteral("create_circle") || s == QStringLiteral("circle")) return AiOperationType::CreateCircle;
    if (s == QStringLiteral("create_rectangle") || s == QStringLiteral("rectangle") || s == QStringLiteral("rect")) return AiOperationType::CreateRectangle;
    if (s == QStringLiteral("create_arc") || s == QStringLiteral("arc")) return AiOperationType::CreateArc;
    if (s == QStringLiteral("create_polyline") || s == QStringLiteral("polyline") || s == QStringLiteral("pline")) return AiOperationType::CreatePolyline;
    if (s == QStringLiteral("create_text") || s == QStringLiteral("text")) return AiOperationType::CreateText;
    if (s == QStringLiteral("create_dimension") || s == QStringLiteral("dimension") || s == QStringLiteral("dim")) return AiOperationType::CreateDimension;
    if (s == QStringLiteral("modify_move") || s == QStringLiteral("move")) return AiOperationType::ModifyMove;
    if (s == QStringLiteral("modify_rotate") || s == QStringLiteral("rotate")) return AiOperationType::ModifyRotate;
    if (s == QStringLiteral("modify_mirror") || s == QStringLiteral("mirror")) return AiOperationType::ModifyMirror;
    if (s == QStringLiteral("delete_entity") || s == QStringLiteral("delete") || s == QStringLiteral("remove")) return AiOperationType::DeleteEntity;
    if (s == QStringLiteral("add_layer") || s == QStringLiteral("layer")) return AiOperationType::AddLayer;
    if (s == QStringLiteral("set_entity_layer")) return AiOperationType::SetEntityLayer;
    return AiOperationType::Unknown;
}

QJsonObject AiOperation::toJson() const {
    QJsonObject obj;
    obj["type"] = operationTypeToString(type);

    if (!targetId.isNull()) {
        obj["target_id"] = targetId.toString(QUuid::WithoutBraces);
    }
    if (!layer.isEmpty()) {
        obj["layer"] = layer;
    }
    if (hasCustomColor && color.isValid()) {
        obj["color"] = color.name();
    }

    switch (type) {
        case AiOperationType::CreateLine:
            obj["start"] = pointToJson(p1);
            obj["end"] = pointToJson(p2);
            break;

        case AiOperationType::CreateCircle:
            obj["center"] = pointToJson(p1);
            obj["radius"] = radius;
            break;

        case AiOperationType::CreateRectangle:
            obj["top_left"] = pointToJson(p1);
            obj["width"] = width;
            obj["height"] = height;
            break;

        case AiOperationType::CreateArc:
            obj["center"] = pointToJson(p1);
            obj["radius"] = radius;
            obj["start_angle"] = startAngle;
            obj["sweep_angle"] = sweepAngle;
            break;

        case AiOperationType::CreatePolyline: {
            QJsonArray ptsArr;
            for (const auto& pt : points) {
                ptsArr.append(pointToJson(pt));
            }
            obj["points"] = ptsArr;
            obj["closed"] = closed;
            break;
        }

        case AiOperationType::CreateText:
            obj["position"] = pointToJson(p1);
            obj["text"] = text;
            obj["height"] = height > 0.0 ? height : 10.0;
            obj["rotation"] = rotation;
            break;

        case AiOperationType::CreateDimension:
            obj["p1"] = pointToJson(p1);
            obj["p2"] = pointToJson(p2);
            obj["dim_point"] = pointToJson(p3);
            break;

        case AiOperationType::ModifyMove:
            obj["delta"] = pointToJson(delta);
            break;

        case AiOperationType::ModifyRotate:
            obj["origin"] = pointToJson(origin);
            obj["angle_deg"] = angleDeg;
            break;

        case AiOperationType::ModifyMirror:
            obj["mirror_p1"] = pointToJson(p1);
            obj["mirror_p2"] = pointToJson(p2);
            break;

        case AiOperationType::DeleteEntity:
            break;

        case AiOperationType::AddLayer:
            obj["layer_name"] = layerName;
            obj["layer_color"] = layerColor.name();
            break;

        case AiOperationType::SetEntityLayer:
            obj["layer"] = layer;
            break;

        default:
            break;
    }

    return obj;
}

bool AiOperation::fromJson(const QJsonObject& json, AiOperation* outOp, QString* errorMessage) {
    if (!outOp) return false;
    *outOp = AiOperation();

    QString typeStr = json["type"].toString();
    outOp->type = stringToOperationType(typeStr);
    if (outOp->type == AiOperationType::Unknown) {
        if (errorMessage) *errorMessage = QStringLiteral("Unknown operation type: '%1'").arg(typeStr);
        return false;
    }

    if (json.contains("target_id")) {
        outOp->targetId = QUuid::fromString(json["target_id"].toString());
    } else if (json.contains("id")) {
        outOp->targetId = QUuid::fromString(json["id"].toString());
    }

    if (json.contains("layer")) {
        outOp->layer = json["layer"].toString();
    }
    if (json.contains("color")) {
        outOp->color = QColor(json["color"].toString());
        outOp->hasCustomColor = outOp->color.isValid();
    }

    switch (outOp->type) {
        case AiOperationType::CreateLine: {
            bool ok1 = false, ok2 = false;
            if (json.contains("start")) outOp->p1 = pointFromJson(json["start"], &ok1);
            else if (json.contains("p1")) outOp->p1 = pointFromJson(json["p1"], &ok1);

            if (json.contains("end")) outOp->p2 = pointFromJson(json["end"], &ok2);
            else if (json.contains("p2")) outOp->p2 = pointFromJson(json["p2"], &ok2);

            if (!ok1 || !ok2) {
                if (errorMessage) *errorMessage = QStringLiteral("Line requires start and end points");
                return false;
            }
            break;
        }

        case AiOperationType::CreateCircle: {
            bool ok = false;
            if (json.contains("center")) outOp->p1 = pointFromJson(json["center"], &ok);
            else if (json.contains("p1")) outOp->p1 = pointFromJson(json["p1"], &ok);

            if (json.contains("radius")) outOp->radius = json["radius"].toDouble();
            else if (json.contains("r")) outOp->radius = json["r"].toDouble();
            else if (json.contains("diameter")) outOp->radius = json["diameter"].toDouble() / 2.0;

            if (!ok) {
                if (errorMessage) *errorMessage = QStringLiteral("Circle requires center point");
                return false;
            }
            break;
        }

        case AiOperationType::CreateRectangle: {
            bool ok = false;
            if (json.contains("top_left")) outOp->p1 = pointFromJson(json["top_left"], &ok);
            else if (json.contains("corner")) outOp->p1 = pointFromJson(json["corner"], &ok);
            else if (json.contains("p1")) outOp->p1 = pointFromJson(json["p1"], &ok);
            else if (json.contains("center")) {
                QPointF center = pointFromJson(json["center"], &ok);
                double w = json["width"].toDouble();
                double h = json["height"].toDouble();
                outOp->p1 = QPointF(center.x() - w / 2.0, center.y() - h / 2.0);
            }

            outOp->width = json["width"].toDouble();
            outOp->height = json["height"].toDouble();
            if (!ok) {
                if (errorMessage) *errorMessage = QStringLiteral("Rectangle requires origin corner or center");
                return false;
            }
            break;
        }

        case AiOperationType::CreateArc: {
            bool ok = false;
            if (json.contains("center")) outOp->p1 = pointFromJson(json["center"], &ok);
            else if (json.contains("p1")) outOp->p1 = pointFromJson(json["p1"], &ok);

            outOp->radius = json["radius"].toDouble();
            outOp->startAngle = json["start_angle"].toDouble();
            if (json.contains("sweep_angle")) outOp->sweepAngle = json["sweep_angle"].toDouble();
            else if (json.contains("span_angle")) outOp->sweepAngle = json["span_angle"].toDouble();
            else if (json.contains("end_angle")) outOp->sweepAngle = json["end_angle"].toDouble() - outOp->startAngle;

            if (!ok) {
                if (errorMessage) *errorMessage = QStringLiteral("Arc requires center point");
                return false;
            }
            break;
        }

        case AiOperationType::CreatePolyline: {
            if (!json.contains("points") || !json["points"].isArray()) {
                if (errorMessage) *errorMessage = QStringLiteral("Polyline requires points array");
                return false;
            }
            QJsonArray ptsArr = json["points"].toArray();
            for (const auto& item : ptsArr) {
                bool ok = false;
                QPointF pt = pointFromJson(item, &ok);
                if (ok) outOp->points.push_back(pt);
            }
            outOp->closed = json["closed"].toBool(false);
            break;
        }

        case AiOperationType::CreateText: {
            bool ok = false;
            if (json.contains("position")) outOp->p1 = pointFromJson(json["position"], &ok);
            else if (json.contains("pos")) outOp->p1 = pointFromJson(json["pos"], &ok);
            else if (json.contains("p1")) outOp->p1 = pointFromJson(json["p1"], &ok);

            outOp->text = json["text"].toString();
            outOp->height = json.contains("height") ? json["height"].toDouble() : 10.0;
            outOp->rotation = json.contains("rotation") ? json["rotation"].toDouble() : 0.0;

            if (!ok) {
                if (errorMessage) *errorMessage = QStringLiteral("Text requires position");
                return false;
            }
            break;
        }

        case AiOperationType::CreateDimension: {
            bool ok1 = false, ok2 = false, ok3 = false;
            if (json.contains("p1")) outOp->p1 = pointFromJson(json["p1"], &ok1);
            if (json.contains("p2")) outOp->p2 = pointFromJson(json["p2"], &ok2);
            if (json.contains("dim_point")) outOp->p3 = pointFromJson(json["dim_point"], &ok3);
            else if (json.contains("p3")) outOp->p3 = pointFromJson(json["p3"], &ok3);
            else {
                // Default offset midpoint
                outOp->p3 = QPointF((outOp->p1.x() + outOp->p2.x()) / 2.0,
                                    (outOp->p1.y() + outOp->p2.y()) / 2.0 + 15.0);
                ok3 = true;
            }

            if (!ok1 || !ok2) {
                if (errorMessage) *errorMessage = QStringLiteral("Dimension requires p1 and p2");
                return false;
            }
            break;
        }

        case AiOperationType::ModifyMove: {
            bool ok = false;
            if (json.contains("delta")) outOp->delta = pointFromJson(json["delta"], &ok);
            else if (json.contains("dx") && json.contains("dy")) {
                outOp->delta = QPointF(json["dx"].toDouble(), json["dy"].toDouble());
                ok = true;
            }
            if (!ok) {
                if (errorMessage) *errorMessage = QStringLiteral("Move requires delta [dx, dy]");
                return false;
            }
            break;
        }

        case AiOperationType::ModifyRotate: {
            bool ok = false;
            if (json.contains("origin")) outOp->origin = pointFromJson(json["origin"], &ok);
            else {
                outOp->origin = QPointF(0, 0);
                ok = true;
            }
            outOp->angleDeg = json["angle_deg"].toDouble();
            break;
        }

        case AiOperationType::ModifyMirror: {
            bool ok1 = false, ok2 = false;
            if (json.contains("mirror_p1")) outOp->p1 = pointFromJson(json["mirror_p1"], &ok1);
            else if (json.contains("p1")) outOp->p1 = pointFromJson(json["p1"], &ok1);

            if (json.contains("mirror_p2")) outOp->p2 = pointFromJson(json["mirror_p2"], &ok2);
            else if (json.contains("p2")) outOp->p2 = pointFromJson(json["p2"], &ok2);

            if (!ok1 || !ok2) {
                if (errorMessage) *errorMessage = QStringLiteral("Mirror requires mirror line points p1 and p2");
                return false;
            }
            break;
        }

        case AiOperationType::DeleteEntity:
            if (outOp->targetId.isNull()) {
                if (errorMessage) *errorMessage = QStringLiteral("Delete requires valid target_id");
                return false;
            }
            break;

        case AiOperationType::AddLayer:
            outOp->layerName = json["layer_name"].toString();
            if (json.contains("layer_color")) {
                outOp->layerColor = QColor(json["layer_color"].toString());
            }
            if (outOp->layerName.isEmpty()) {
                if (errorMessage) *errorMessage = QStringLiteral("AddLayer requires layer_name");
                return false;
            }
            break;

        case AiOperationType::SetEntityLayer:
            if (outOp->targetId.isNull() || outOp->layer.isEmpty()) {
                if (errorMessage) *errorMessage = QStringLiteral("SetEntityLayer requires target_id and layer");
                return false;
            }
            break;

        default:
            break;
    }

    return true;
}

int AiProposal::createdCount() const {
    int count = 0;
    for (const auto& op : operations) {
        if (op.type == AiOperationType::CreateLine ||
            op.type == AiOperationType::CreateCircle ||
            op.type == AiOperationType::CreateRectangle ||
            op.type == AiOperationType::CreateArc ||
            op.type == AiOperationType::CreatePolyline ||
            op.type == AiOperationType::CreateText ||
            op.type == AiOperationType::CreateDimension) {
            count++;
        }
    }
    return count;
}

int AiProposal::modifiedCount() const {
    int count = 0;
    for (const auto& op : operations) {
        if (op.type == AiOperationType::ModifyMove ||
            op.type == AiOperationType::ModifyRotate ||
            op.type == AiOperationType::ModifyMirror ||
            op.type == AiOperationType::SetEntityLayer) {
            count++;
        }
    }
    return count;
}

int AiProposal::deletedCount() const {
    int count = 0;
    for (const auto& op : operations) {
        if (op.type == AiOperationType::DeleteEntity) {
            count++;
        }
    }
    return count;
}

int AiProposal::dimensionCount() const {
    int count = 0;
    for (const auto& op : operations) {
        if (op.type == AiOperationType::CreateDimension) {
            count++;
        }
    }
    return count;
}

QString AiProposal::summaryText() const {
    int lines = 0, circles = 0, rects = 0, arcs = 0, plines = 0, texts = 0, dims = 0;
    int mods = 0, dels = 0;

    for (const auto& op : operations) {
        switch (op.type) {
            case AiOperationType::CreateLine: lines++; break;
            case AiOperationType::CreateCircle: circles++; break;
            case AiOperationType::CreateRectangle: rects++; break;
            case AiOperationType::CreateArc: arcs++; break;
            case AiOperationType::CreatePolyline: plines++; break;
            case AiOperationType::CreateText: texts++; break;
            case AiOperationType::CreateDimension: dims++; break;
            case AiOperationType::ModifyMove:
            case AiOperationType::ModifyRotate:
            case AiOperationType::ModifyMirror:
            case AiOperationType::SetEntityLayer: mods++; break;
            case AiOperationType::DeleteEntity: dels++; break;
            default: break;
        }
    }

    QStringList parts;
    if (lines > 0) parts << QStringLiteral("+ %1 %2").arg(lines).arg(lines == 1 ? "Line" : "Lines");
    if (rects > 0) parts << QStringLiteral("+ %1 %2").arg(rects).arg(rects == 1 ? "Rectangle" : "Rectangles");
    if (circles > 0) parts << QStringLiteral("+ %1 %2").arg(circles).arg(circles == 1 ? "Circle" : "Circles");
    if (arcs > 0) parts << QStringLiteral("+ %1 %2").arg(arcs).arg(arcs == 1 ? "Arc" : "Arcs");
    if (plines > 0) parts << QStringLiteral("+ %1 %2").arg(plines).arg(plines == 1 ? "Polyline" : "Polylines");
    if (texts > 0) parts << QStringLiteral("+ %1 %2").arg(texts).arg(texts == 1 ? "Text" : "Texts");
    if (dims > 0) parts << QStringLiteral("+ %1 %2").arg(dims).arg(dims == 1 ? "Dimension" : "Dimensions");
    if (mods > 0) parts << QStringLiteral("~ %1 %2").arg(mods).arg(mods == 1 ? "Modified" : "Modified");
    if (dels > 0) parts << QStringLiteral("- %1 %2").arg(dels).arg(dels == 1 ? "Deleted" : "Deleted");

    if (parts.isEmpty()) {
        return QStringLiteral("0 operations proposed");
    }
    return parts.join(QStringLiteral(", "));
}

QJsonObject AiProposal::toJson() const {
    QJsonObject root;
    root["version"] = QStringLiteral("1.0");
    root["description"] = description;
    if (!prompt.isEmpty()) {
        root["prompt"] = prompt;
    }

    QJsonArray opsArr;
    for (const auto& op : operations) {
        opsArr.append(op.toJson());
    }
    root["operations"] = opsArr;
    return root;
}

bool AiProposal::fromJson(const QJsonObject& json, AiProposal* outProposal, QString* errorMessage) {
    if (!outProposal) return false;
    *outProposal = AiProposal();

    outProposal->description = json["description"].toString();
    outProposal->prompt = json["prompt"].toString();

    QJsonArray opsArr;
    if (json.contains("operations") && json["operations"].isArray()) {
        opsArr = json["operations"].toArray();
    } else if (json.contains("commands") && json["commands"].isArray()) {
        opsArr = json["commands"].toArray();
    } else {
        if (errorMessage) *errorMessage = QStringLiteral("Missing 'operations' array in proposal JSON");
        return false;
    }

    for (int i = 0; i < opsArr.size(); ++i) {
        QJsonObject opObj = opsArr[i].toObject();
        AiOperation op;
        QString opErr;
        if (!AiOperation::fromJson(opObj, &op, &opErr)) {
            if (errorMessage) *errorMessage = QStringLiteral("Operation #%1 invalid: %2").arg(i + 1).arg(opErr);
            return false;
        }
        outProposal->operations.push_back(op);
    }

    return true;
}

bool AiProposal::fromJsonString(const QString& jsonString, AiProposal* outProposal, QString* errorMessage) {
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(jsonString.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (errorMessage) *errorMessage = parseError.errorString();
        return false;
    }
    if (!doc.isObject()) {
        if (errorMessage) *errorMessage = QStringLiteral("Root JSON is not an object");
        return false;
    }
    return fromJson(doc.object(), outProposal, errorMessage);
}
