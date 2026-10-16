#include "document/UnitSystem.h"
#include <cmath>
#include <algorithm>

UnitSettings::UnitSettings()
    : m_type(UnitType::Millimeter)
    , m_precision(2)
    , m_customScaleToMm(1.0)
    , m_customUnitName(QStringLiteral("unit"))
{
}

UnitSettings::UnitSettings(UnitType type, int precision, double customScaleToMm, const QString& customName)
    : m_type(type)
    , m_precision(std::clamp(precision, 0, 6))
    , m_customScaleToMm(customScaleToMm > 1e-9 ? customScaleToMm : 1.0)
    , m_customUnitName(customName.isEmpty() ? QStringLiteral("unit") : customName)
{
}

void UnitSettings::setType(UnitType type) {
    m_type = type;
}

void UnitSettings::setPrecision(int precision) {
    m_precision = std::clamp(precision, 0, 6);
}

void UnitSettings::setCustomScaleToMm(double scale) {
    if (scale > 1e-9) {
        m_customScaleToMm = scale;
    }
}

void UnitSettings::setCustomUnitName(const QString& name) {
    if (!name.trimmed().isEmpty()) {
        m_customUnitName = name.trimmed();
    }
}

double UnitSettings::scaleToMm() const {
    switch (m_type) {
        case UnitType::Millimeter: return 1.0;
        case UnitType::Centimeter: return 10.0;
        case UnitType::Meter:      return 1000.0;
        case UnitType::Inch:       return 25.4;
        case UnitType::Foot:       return 304.8;
        case UnitType::Custom:     return m_customScaleToMm;
    }
    return 1.0;
}

double UnitSettings::scaleFromMm() const {
    double s = scaleToMm();
    return s > 1e-9 ? (1.0 / s) : 1.0;
}

QString UnitSettings::unitSuffix() const {
    switch (m_type) {
        case UnitType::Millimeter: return QStringLiteral("mm");
        case UnitType::Centimeter: return QStringLiteral("cm");
        case UnitType::Meter:      return QStringLiteral("m");
        case UnitType::Inch:       return QStringLiteral("in");
        case UnitType::Foot:       return QStringLiteral("ft");
        case UnitType::Custom:     return m_customUnitName;
    }
    return QStringLiteral("mm");
}

QString UnitSettings::areaSuffix() const {
    switch (m_type) {
        case UnitType::Millimeter: return QStringLiteral("mm²");
        case UnitType::Centimeter: return QStringLiteral("cm²");
        case UnitType::Meter:      return QStringLiteral("m²");
        case UnitType::Inch:       return QStringLiteral("in²");
        case UnitType::Foot:       return QStringLiteral("ft²");
        case UnitType::Custom:     return m_customUnitName + QStringLiteral("²");
    }
    return QStringLiteral("mm²");
}

QString UnitSettings::unitName() const {
    switch (m_type) {
        case UnitType::Millimeter: return QStringLiteral("Millimeter");
        case UnitType::Centimeter: return QStringLiteral("Centimeter");
        case UnitType::Meter:      return QStringLiteral("Meter");
        case UnitType::Inch:       return QStringLiteral("Inch");
        case UnitType::Foot:       return QStringLiteral("Foot");
        case UnitType::Custom:     return QStringLiteral("Custom");
    }
    return QStringLiteral("Millimeter");
}

QString UnitSettings::formatLinear(double mmValue, bool includeSuffix) const {
    double valInUnits = mmValue * scaleFromMm();
    QString formatted = QString::number(valInUnits, 'f', m_precision);
    if (includeSuffix) {
        formatted += QStringLiteral(" ") + unitSuffix();
    }
    return formatted;
}

QString UnitSettings::formatArea(double sqMmValue, bool includeSuffix) const {
    double scale = scaleFromMm();
    double valInSqUnits = sqMmValue * (scale * scale);
    QString formatted = QString::number(valInSqUnits, 'f', m_precision);
    if (includeSuffix) {
        formatted += QStringLiteral(" ") + areaSuffix();
    }
    return formatted;
}

QString UnitSettings::formatAngle(double degValue, bool includeSuffix) const {
    QString formatted = QString::number(degValue, 'f', std::min(m_precision, 2));
    if (includeSuffix) {
        formatted += QStringLiteral("°");
    }
    return formatted;
}

double UnitSettings::parseLinear(const QString& text, bool* ok) const {
    QString cleaned = text.trimmed();
    // Remove known unit suffixes if present
    QStringList suffixes = {unitSuffix(), QStringLiteral("mm"), QStringLiteral("cm"), QStringLiteral("m"), QStringLiteral("in"), QStringLiteral("ft"), QStringLiteral("\""), QStringLiteral("'")};
    for (const auto& s : suffixes) {
        if (cleaned.endsWith(s, Qt::CaseInsensitive)) {
            cleaned.chop(s.length());
            cleaned = cleaned.trimmed();
            break;
        }
    }

    bool parseOk = false;
    double val = cleaned.toDouble(&parseOk);
    if (ok) *ok = parseOk;
    if (!parseOk) return 0.0;

    // Convert from current display units to mm
    return val * scaleToMm();
}

double UnitSettings::parseAngle(const QString& text, bool* ok) const {
    QString cleaned = text.trimmed();
    if (cleaned.endsWith(QStringLiteral("°")) || cleaned.endsWith(QStringLiteral("deg"), Qt::CaseInsensitive)) {
        if (cleaned.endsWith(QStringLiteral("deg"), Qt::CaseInsensitive)) {
            cleaned.chop(3);
        } else {
            cleaned.chop(1);
        }
        cleaned = cleaned.trimmed();
    }

    bool parseOk = false;
    double val = cleaned.toDouble(&parseOk);
    if (ok) *ok = parseOk;
    return parseOk ? val : 0.0;
}

QJsonObject UnitSettings::toJson() const {
    QJsonObject obj;
    obj[QStringLiteral("unitType")] = unitTypeToString(m_type);
    obj[QStringLiteral("precision")] = m_precision;
    obj[QStringLiteral("customScaleToMm")] = m_customScaleToMm;
    obj[QStringLiteral("customUnitName")] = m_customUnitName;
    return obj;
}

bool UnitSettings::fromJson(const QJsonObject& json) {
    if (json.contains(QStringLiteral("unitType"))) {
        m_type = stringToUnitType(json[QStringLiteral("unitType")].toString());
    }
    if (json.contains(QStringLiteral("precision"))) {
        m_precision = std::clamp(json[QStringLiteral("precision")].toInt(2), 0, 6);
    }
    if (json.contains(QStringLiteral("customScaleToMm"))) {
        m_customScaleToMm = json[QStringLiteral("customScaleToMm")].toDouble(1.0);
    }
    if (json.contains(QStringLiteral("customUnitName"))) {
        m_customUnitName = json[QStringLiteral("customUnitName")].toString(QStringLiteral("unit"));
    }
    return true;
}

QString UnitSettings::unitTypeToString(UnitType type) {
    switch (type) {
        case UnitType::Millimeter: return QStringLiteral("mm");
        case UnitType::Centimeter: return QStringLiteral("cm");
        case UnitType::Meter:      return QStringLiteral("m");
        case UnitType::Inch:       return QStringLiteral("in");
        case UnitType::Foot:       return QStringLiteral("ft");
        case UnitType::Custom:     return QStringLiteral("custom");
    }
    return QStringLiteral("mm");
}

UnitType UnitSettings::stringToUnitType(const QString& str, bool* ok) {
    QString s = str.trimmed().toLower();
    if (ok) *ok = true;
    if (s == QStringLiteral("mm") || s == QStringLiteral("millimeter") || s == QStringLiteral("millimeters")) return UnitType::Millimeter;
    if (s == QStringLiteral("cm") || s == QStringLiteral("centimeter") || s == QStringLiteral("centimeters")) return UnitType::Centimeter;
    if (s == QStringLiteral("m") || s == QStringLiteral("meter") || s == QStringLiteral("meters")) return UnitType::Meter;
    if (s == QStringLiteral("in") || s == QStringLiteral("inch") || s == QStringLiteral("inches")) return UnitType::Inch;
    if (s == QStringLiteral("ft") || s == QStringLiteral("foot") || s == QStringLiteral("feet")) return UnitType::Foot;
    if (s == QStringLiteral("custom")) return UnitType::Custom;

    if (ok) *ok = false;
    return UnitType::Millimeter;
}
