#pragma once

#include "geometry/CadEntity.h"

enum class DimensionType {
    Aligned,
    Horizontal,
    Vertical,
    Radius,
    Diameter,
    Angular,
    ArcLength
};

class DimensionEntity : public CadEntity {
public:
    DimensionEntity();
    DimensionEntity(const QPointF& p1, const QPointF& p2, const QPointF& dimPoint, DimensionType type = DimensionType::Aligned);
    DimensionEntity(const QUuid& id, const QPointF& p1, const QPointF& p2, const QPointF& dimPoint, DimensionType type = DimensionType::Aligned);

    DimensionType dimensionType() const { return m_dimType; }
    void setDimensionType(DimensionType type) { m_dimType = type; }

    QPointF p1() const { return m_p1; }
    void setP1(const QPointF& p1) { m_p1 = p1; }

    QPointF p2() const { return m_p2; }
    void setP2(const QPointF& p2) { m_p2 = p2; }

    QPointF dimPoint() const { return m_dimPoint; }
    void setDimPoint(const QPointF& pt) { m_dimPoint = pt; }

    // Associative reference to target geometry
    QUuid targetEntityId() const { return m_targetEntityId; }
    void setTargetEntityId(const QUuid& id) { m_targetEntityId = id; }

    int targetFeatureIndex() const { return m_targetFeatureIndex; }
    void setTargetFeatureIndex(int idx) { m_targetFeatureIndex = idx; }

    bool isAssociative() const { return !m_targetEntityId.isNull(); }

    // Measurement & text formatting
    double measuredValue() const;
    QString dimensionText() const;

    QString unitSuffix() const { return m_unitSuffix; }
    void setUnitSuffix(const QString& suffix) { m_unitSuffix = suffix; }

    int precision() const { return m_precision; }
    void setPrecision(int prec) { m_precision = prec; }

    // Updates coordinates associatively from target entity
    bool updateFromTarget(const CadEntity* target);

    std::shared_ptr<CadEntity> clone() const override;
    QRectF boundingRect() const override;
    void move(double deltaX, double deltaY) override;
    void rotate(const QPointF& origin, double angleRad) override;
    void mirror(const QPointF& p1, const QPointF& p2) override;
    bool hitTest(const QPointF& worldPt, double tolerance) const override;
    std::vector<QPointF> snapPoints() const override;

    QJsonObject toJson() const override;
    bool fromJson(const QJsonObject& json) override;

    static QString dimensionTypeToString(DimensionType type);
    static DimensionType stringToDimensionType(const QString& str, bool* ok = nullptr);

private:
    DimensionType m_dimType{DimensionType::Aligned};
    QPointF m_p1;
    QPointF m_p2;
    QPointF m_dimPoint;
    QUuid m_targetEntityId;
    int m_targetFeatureIndex{0};
    QString m_unitSuffix;
    int m_precision{2};
};
