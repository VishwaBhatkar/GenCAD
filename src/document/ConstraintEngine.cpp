#include "document/ConstraintEngine.h"
#include "document/CadDocument.h"
#include "geometry/LineEntity.h"
#include <cmath>

QJsonObject CadConstraint::toJson() const {
    QJsonObject obj;
    obj[QStringLiteral("id")] = id.toString();
    obj[QStringLiteral("type")] = ConstraintEngine::constraintTypeToString(type);
    obj[QStringLiteral("entityId1")] = entityId1.toString();
    obj[QStringLiteral("entityId2")] = entityId2.toString();
    obj[QStringLiteral("paramValue")] = paramValue;
    return obj;
}

CadConstraint CadConstraint::fromJson(const QJsonObject& json) {
    CadConstraint c;
    c.id = QUuid::fromString(json[QStringLiteral("id")].toString());
    if (c.id.isNull()) c.id = QUuid::createUuid();
    c.type = ConstraintEngine::stringToConstraintType(json[QStringLiteral("type")].toString());
    c.entityId1 = QUuid::fromString(json[QStringLiteral("entityId1")].toString());
    c.entityId2 = QUuid::fromString(json[QStringLiteral("entityId2")].toString());
    c.paramValue = json[QStringLiteral("paramValue")].toDouble(0.0);
    return c;
}

ConstraintEngine::ConstraintEngine(CadDocument* doc)
    : m_document(doc)
{
}

void ConstraintEngine::addConstraint(const CadConstraint& constraint) {
    m_constraints.push_back(constraint);
}

bool ConstraintEngine::removeConstraint(const QUuid& id) {
    auto it = std::remove_if(m_constraints.begin(), m_constraints.end(), [&](const CadConstraint& c) {
        return c.id == id;
    });
    if (it != m_constraints.end()) {
        m_constraints.erase(it, m_constraints.end());
        return true;
    }
    return false;
}

void ConstraintEngine::clear() {
    m_constraints.clear();
}

bool ConstraintEngine::applyConstraints() {
    if (!m_document) return false;
    bool anyApplied = false;

    for (const auto& c : m_constraints) {
        auto ent1 = m_document->findEntity(c.entityId1);
        if (!ent1) continue;

        if (c.type == ConstraintType::Horizontal) {
            if (ent1->type() == EntityType::Line) {
                auto line = std::dynamic_pointer_cast<LineEntity>(ent1);
                if (line) {
                    QPointF p1 = line->p1();
                    QPointF p2 = line->p2();
                    p2.setY(p1.y());
                    line->setP2(p2);
                    anyApplied = true;
                }
            }
        } else if (c.type == ConstraintType::Vertical) {
            if (ent1->type() == EntityType::Line) {
                auto line = std::dynamic_pointer_cast<LineEntity>(ent1);
                if (line) {
                    QPointF p1 = line->p1();
                    QPointF p2 = line->p2();
                    p2.setX(p1.x());
                    line->setP2(p2);
                    anyApplied = true;
                }
            }
        } else if (c.type == ConstraintType::FixedLength) {
            if (ent1->type() == EntityType::Line && c.paramValue > 1e-4) {
                auto line = std::dynamic_pointer_cast<LineEntity>(ent1);
                if (line) {
                    QPointF p1 = line->p1();
                    QPointF p2 = line->p2();
                    double dx = p2.x() - p1.x();
                    double dy = p2.y() - p1.y();
                    double curLen = std::hypot(dx, dy);
                    if (curLen > 1e-6) {
                        double scale = c.paramValue / curLen;
                        p2 = QPointF(p1.x() + dx * scale, p1.y() + dy * scale);
                        line->setP2(p2);
                        anyApplied = true;
                    }
                }
            }
        }
    }
    return anyApplied;
}

QJsonArray ConstraintEngine::toJson() const {
    QJsonArray arr;
    for (const auto& c : m_constraints) {
        arr.append(c.toJson());
    }
    return arr;
}

bool ConstraintEngine::fromJson(const QJsonArray& array) {
    m_constraints.clear();
    for (const auto& val : array) {
        if (val.isObject()) {
            m_constraints.push_back(CadConstraint::fromJson(val.toObject()));
        }
    }
    return true;
}

QString ConstraintEngine::constraintTypeToString(ConstraintType type) {
    switch (type) {
        case ConstraintType::Horizontal: return QStringLiteral("Horizontal");
        case ConstraintType::Vertical: return QStringLiteral("Vertical");
        case ConstraintType::FixedLength: return QStringLiteral("FixedLength");
        case ConstraintType::Coincident: return QStringLiteral("Coincident");
        case ConstraintType::Parallel: return QStringLiteral("Parallel");
        case ConstraintType::Perpendicular: return QStringLiteral("Perpendicular");
    }
    return QStringLiteral("Horizontal");
}

ConstraintType ConstraintEngine::stringToConstraintType(const QString& str, bool* ok) {
    QString s = str.trimmed().toLower();
    if (ok) *ok = true;
    if (s == QStringLiteral("horizontal")) return ConstraintType::Horizontal;
    if (s == QStringLiteral("vertical")) return ConstraintType::Vertical;
    if (s == QStringLiteral("fixedlength")) return ConstraintType::FixedLength;
    if (s == QStringLiteral("coincident")) return ConstraintType::Coincident;
    if (s == QStringLiteral("parallel")) return ConstraintType::Parallel;
    if (s == QStringLiteral("perpendicular")) return ConstraintType::Perpendicular;

    if (ok) *ok = false;
    return ConstraintType::Horizontal;
}
