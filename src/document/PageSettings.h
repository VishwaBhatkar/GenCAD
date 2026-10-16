#pragma once

#include <QString>
#include <QRectF>
#include <QPointF>
#include <QJsonObject>

enum class PageSize {
    A0,
    A1,
    A2,
    A3,
    A4,
    A5,
    Custom
};

enum class PageOrientation {
    Portrait,
    Landscape
};

class PageSettings {
public:
    PageSettings();
    PageSettings(PageSize size, PageOrientation orientation, double customWidth = 297.0, double customHeight = 210.0);

    PageSize pageSize() const { return m_pageSize; }
    void setPageSize(PageSize size);

    PageOrientation orientation() const { return m_orientation; }
    void setOrientation(PageOrientation orientation);

    double customWidth() const { return m_customWidth; }
    void setCustomWidth(double w);

    double customHeight() const { return m_customHeight; }
    void setCustomHeight(double h);

    // Page dimensions in millimeters
    double widthMm() const;
    double heightMm() const;
    QRectF pageRect() const;

    // Drawing limits (in millimeters)
    double minX() const { return m_minX; }
    double minY() const { return m_minY; }
    double maxX() const { return m_maxX; }
    double maxY() const { return m_maxY; }
    void setDrawingLimits(double minX, double minY, double maxX, double maxY);
    QRectF drawingLimits() const;

    bool isLimitsCheckEnabled() const { return m_limitsCheck; }
    void setLimitsCheckEnabled(bool enable) { m_limitsCheck = enable; }
    bool enforceLimits() const { return isLimitsCheckEnabled(); }
    void setEnforceLimits(bool enable) { setLimitsCheckEnabled(enable); }
    QRectF limitsRect() const { return drawingLimits(); }
    bool isWithinLimits(const QPointF& pt) const;

    // Display string
    QString pageSizeName() const;
    QString dimensionsText() const;

    // Serialization
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject& json);

    // Helper conversions
    static QString pageSizeToString(PageSize size);
    static PageSize stringToPageSize(const QString& str, bool* ok = nullptr);
    static QString orientationToString(PageOrientation orient);
    static PageOrientation stringToOrientation(const QString& str, bool* ok = nullptr);

    // Standard ISO A-series base dimensions (width <= height in portrait)
    static QPair<double, double> standardDimensionsMm(PageSize size);

private:
    void updateDefaultLimits();

    PageSize m_pageSize{PageSize::A4};
    PageOrientation m_orientation{PageOrientation::Landscape};
    double m_customWidth{297.0};
    double m_customHeight{210.0};
    double m_minX{0.0};
    double m_minY{0.0};
    double m_maxX{297.0};
    double m_maxY{210.0};
    bool m_limitsCheck{false};
};
