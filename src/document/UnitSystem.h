#pragma once

#include <QString>
#include <QJsonObject>

enum class UnitType {
    Millimeter,
    Centimeter,
    Meter,
    Inch,
    Foot,
    Custom
};

class UnitSettings {
public:
    UnitSettings();
    explicit UnitSettings(UnitType type, int precision = 2, double customScaleToMm = 1.0, const QString& customName = QString());

    UnitType type() const { return m_type; }
    void setType(UnitType type);

    int precision() const { return m_precision; }
    void setPrecision(int precision);

    double customScaleToMm() const { return m_customScaleToMm; }
    void setCustomScaleToMm(double scale);

    QString customUnitName() const { return m_customUnitName; }
    void setCustomUnitName(const QString& name);

    // Conversion factors (internal base unit is always Millimeter)
    double scaleToMm() const;
    double scaleFromMm() const;

    double toUserUnit(double mmValue) const { return mmValue * scaleFromMm(); }
    double toInternal(double userValue) const { return userValue * scaleToMm(); }
    double fromInternal(double mmValue) const { return toUserUnit(mmValue); }

    // Unit suffixes
    QString unitSuffix() const;
    QString areaSuffix() const;
    QString unitName() const;

    // Formatting methods
    QString formatLinear(double mmValue, bool includeSuffix = true) const;
    QString formatArea(double sqMmValue, bool includeSuffix = true) const;
    QString formatAngle(double degValue, bool includeSuffix = true) const;

    // Parsing methods (user input in current display units -> internal mm)
    double parseLinear(const QString& text, bool* ok = nullptr) const;
    double parseAngle(const QString& text, bool* ok = nullptr) const;

    // Serialization
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject& json);

    // Helpers
    static QString unitTypeToString(UnitType type);
    static UnitType stringToUnitType(const QString& str, bool* ok = nullptr);

private:
    UnitType m_type{UnitType::Millimeter};
    int m_precision{2};
    double m_customScaleToMm{1.0};
    QString m_customUnitName{QStringLiteral("unit")};
};
