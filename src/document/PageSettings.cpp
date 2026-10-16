#include "document/PageSettings.h"
#include <algorithm>

PageSettings::PageSettings()
    : m_pageSize(PageSize::A4)
    , m_orientation(PageOrientation::Landscape)
    , m_customWidth(297.0)
    , m_customHeight(210.0)
{
    updateDefaultLimits();
}

PageSettings::PageSettings(PageSize size, PageOrientation orientation, double customWidth, double customHeight)
    : m_pageSize(size)
    , m_orientation(orientation)
    , m_customWidth(customWidth > 1.0 ? customWidth : 297.0)
    , m_customHeight(customHeight > 1.0 ? customHeight : 210.0)
{
    updateDefaultLimits();
}

void PageSettings::setPageSize(PageSize size) {
    m_pageSize = size;
    updateDefaultLimits();
}

void PageSettings::setOrientation(PageOrientation orientation) {
    m_orientation = orientation;
    updateDefaultLimits();
}

void PageSettings::setCustomWidth(double w) {
    if (w > 1.0) {
        m_customWidth = w;
        if (m_pageSize == PageSize::Custom) {
            updateDefaultLimits();
        }
    }
}

void PageSettings::setCustomHeight(double h) {
    if (h > 1.0) {
        m_customHeight = h;
        if (m_pageSize == PageSize::Custom) {
            updateDefaultLimits();
        }
    }
}

QPair<double, double> PageSettings::standardDimensionsMm(PageSize size) {
    switch (size) {
        case PageSize::A0: return {841.0, 1189.0};
        case PageSize::A1: return {594.0, 841.0};
        case PageSize::A2: return {420.0, 594.0};
        case PageSize::A3: return {297.0, 420.0};
        case PageSize::A4: return {210.0, 297.0};
        case PageSize::A5: return {148.0, 210.0};
        case PageSize::Custom: return {297.0, 210.0};
    }
    return {210.0, 297.0};
}

double PageSettings::widthMm() const {
    if (m_pageSize == PageSize::Custom) {
        return m_orientation == PageOrientation::Portrait ? std::min(m_customWidth, m_customHeight) : std::max(m_customWidth, m_customHeight);
    }
    auto base = standardDimensionsMm(m_pageSize);
    return m_orientation == PageOrientation::Portrait ? base.first : base.second;
}

double PageSettings::heightMm() const {
    if (m_pageSize == PageSize::Custom) {
        return m_orientation == PageOrientation::Portrait ? std::max(m_customWidth, m_customHeight) : std::min(m_customWidth, m_customHeight);
    }
    auto base = standardDimensionsMm(m_pageSize);
    return m_orientation == PageOrientation::Portrait ? base.second : base.first;
}

QRectF PageSettings::pageRect() const {
    return QRectF(0.0, 0.0, widthMm(), heightMm());
}

void PageSettings::setDrawingLimits(double minX, double minY, double maxX, double maxY) {
    m_minX = std::min(minX, maxX);
    m_minY = std::min(minY, maxY);
    m_maxX = std::max(minX, maxX);
    m_maxY = std::max(minY, maxY);
}

QRectF PageSettings::drawingLimits() const {
    return QRectF(QPointF(m_minX, m_minY), QPointF(m_maxX, m_maxY));
}

bool PageSettings::isWithinLimits(const QPointF& pt) const {
    if (!m_limitsCheck) return true;
    return pt.x() >= m_minX && pt.x() <= m_maxX && pt.y() >= m_minY && pt.y() <= m_maxY;
}

QString PageSettings::pageSizeName() const {
    return pageSizeToString(m_pageSize);
}

QString PageSettings::dimensionsText() const {
    return QStringLiteral("%1 mm × %2 mm").arg(widthMm(), 0, 'f', 0).arg(heightMm(), 0, 'f', 0);
}

void PageSettings::updateDefaultLimits() {
    m_minX = 0.0;
    m_minY = 0.0;
    m_maxX = widthMm();
    m_maxY = heightMm();
}

QJsonObject PageSettings::toJson() const {
    QJsonObject obj;
    obj[QStringLiteral("pageSize")] = pageSizeToString(m_pageSize);
    obj[QStringLiteral("orientation")] = orientationToString(m_orientation);
    obj[QStringLiteral("customWidth")] = m_customWidth;
    obj[QStringLiteral("customHeight")] = m_customHeight;
    obj[QStringLiteral("minX")] = m_minX;
    obj[QStringLiteral("minY")] = m_minY;
    obj[QStringLiteral("maxX")] = m_maxX;
    obj[QStringLiteral("maxY")] = m_maxY;
    obj[QStringLiteral("limitsCheck")] = m_limitsCheck;
    return obj;
}

bool PageSettings::fromJson(const QJsonObject& json) {
    if (json.contains(QStringLiteral("pageSize"))) {
        m_pageSize = stringToPageSize(json[QStringLiteral("pageSize")].toString());
    }
    if (json.contains(QStringLiteral("orientation"))) {
        m_orientation = stringToOrientation(json[QStringLiteral("orientation")].toString());
    }
    if (json.contains(QStringLiteral("customWidth"))) {
        m_customWidth = json[QStringLiteral("customWidth")].toDouble(297.0);
    }
    if (json.contains(QStringLiteral("customHeight"))) {
        m_customHeight = json[QStringLiteral("customHeight")].toDouble(210.0);
    }
    if (json.contains(QStringLiteral("minX"))) {
        m_minX = json[QStringLiteral("minX")].toDouble(0.0);
    }
    if (json.contains(QStringLiteral("minY"))) {
        m_minY = json[QStringLiteral("minY")].toDouble(0.0);
    }
    if (json.contains(QStringLiteral("maxX"))) {
        m_maxX = json[QStringLiteral("maxX")].toDouble(widthMm());
    }
    if (json.contains(QStringLiteral("maxY"))) {
        m_maxY = json[QStringLiteral("maxY")].toDouble(heightMm());
    }
    if (json.contains(QStringLiteral("limitsCheck"))) {
        m_limitsCheck = json[QStringLiteral("limitsCheck")].toBool(false);
    }
    return true;
}

QString PageSettings::pageSizeToString(PageSize size) {
    switch (size) {
        case PageSize::A0: return QStringLiteral("A0");
        case PageSize::A1: return QStringLiteral("A1");
        case PageSize::A2: return QStringLiteral("A2");
        case PageSize::A3: return QStringLiteral("A3");
        case PageSize::A4: return QStringLiteral("A4");
        case PageSize::A5: return QStringLiteral("A5");
        case PageSize::Custom: return QStringLiteral("Custom");
    }
    return QStringLiteral("A4");
}

PageSize PageSettings::stringToPageSize(const QString& str, bool* ok) {
    QString s = str.trimmed().toUpper();
    if (ok) *ok = true;
    if (s == QStringLiteral("A0")) return PageSize::A0;
    if (s == QStringLiteral("A1")) return PageSize::A1;
    if (s == QStringLiteral("A2")) return PageSize::A2;
    if (s == QStringLiteral("A3")) return PageSize::A3;
    if (s == QStringLiteral("A4")) return PageSize::A4;
    if (s == QStringLiteral("A5")) return PageSize::A5;
    if (s == QStringLiteral("CUSTOM")) return PageSize::Custom;

    if (ok) *ok = false;
    return PageSize::A4;
}

QString PageSettings::orientationToString(PageOrientation orient) {
    return orient == PageOrientation::Portrait ? QStringLiteral("Portrait") : QStringLiteral("Landscape");
}

PageOrientation PageSettings::stringToOrientation(const QString& str, bool* ok) {
    QString s = str.trimmed().toLower();
    if (ok) *ok = true;
    if (s == QStringLiteral("portrait")) return PageOrientation::Portrait;
    if (s == QStringLiteral("landscape")) return PageOrientation::Landscape;

    if (ok) *ok = false;
    return PageOrientation::Landscape;
}
