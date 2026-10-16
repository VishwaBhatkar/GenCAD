#include "canvas/CadScene.h"
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
#include "layers/CadLayer.h"

#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QFontMetricsF>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

CadScene::CadScene(CadDocument* document, QObject* parent)
    : QGraphicsScene(parent)
    , m_document(document)
{
    setSceneRect(-50000, -50000, 100000, 100000);

    if (m_document) {
        connect(m_document, &CadDocument::entityAdded, this, &CadScene::onEntityAdded);
        connect(m_document, &CadDocument::entityRemoved, this, &CadScene::onEntityRemoved);
        connect(m_document, &CadDocument::entityModified, this, &CadScene::onEntityModified);
        connect(m_document, &CadDocument::documentCleared, this, &CadScene::onDocumentCleared);
        connect(m_document, &CadDocument::selectionChanged, this, &CadScene::onSelectionChanged);
        connect(m_document, &CadDocument::documentChanged, this, [this]() { update(); });
        connect(m_document, &CadDocument::pageSettingsChanged, this, [this]() { update(); });
        connect(m_document, &CadDocument::unitSettingsChanged, this, [this]() { update(); });
        connect(m_document, &CadDocument::gridSettingsChanged, this, [this]() { update(); });
    }
}

CadScene::~CadScene() = default;

void CadScene::setThemeMode(CadTheme::ThemeMode mode) {
    if (m_themeMode != mode) {
        m_themeMode = mode;
        update();
    }
}

void CadScene::setPreviewEntity(std::shared_ptr<CadEntity> entity) {
    m_previewEntity = entity;
    update();
}

void CadScene::clearPreview() {
    m_previewEntity.reset();
    update();
}

void CadScene::setPreviewEntities(const std::vector<std::shared_ptr<CadEntity>>& entities) {
    m_previewEntities = entities;
    update();
}

void CadScene::clearPreviewEntities() {
    if (!m_previewEntities.empty()) {
        m_previewEntities.clear();
        update();
    }
}

void CadScene::setGridVisible(bool visible) {
    if (m_gridVisible != visible) {
        m_gridVisible = visible;
        update();
    }
}

void CadScene::setGridSize(double size) {
    if (size > 0.1 && m_gridSize != size) {
        m_gridSize = size;
        update();
    }
}

void CadScene::setPageBorderVisible(bool visible) {
    if (m_pageBorderVisible != visible) {
        m_pageBorderVisible = visible;
        update();
    }
}

void CadScene::setSnapResult(const SnapResult& snap) {
    m_snapResult = snap;
    update();
}

void CadScene::clearSnapResult() {
    if (m_snapResult.snapped) {
        m_snapResult.snapped = false;
        m_snapResult.hasPolarLine = false;
        update();
    }
}

void CadScene::setSelectionBox(const QPointF& start, const QPointF& end) {
    m_selBoxStart = start;
    m_selBoxEnd = end;
    m_hasSelectionBox = true;
    update();
}

void CadScene::clearSelectionBox() {
    if (m_hasSelectionBox) {
        m_hasSelectionBox = false;
        update();
    }
}

void CadScene::setDynamicDimension(const QString& text, const QPointF& pos) {
    m_dynamicDimText = text;
    m_dynamicDimPos = pos;
    m_hasDynamicDim = true;
    update();
}

void CadScene::clearDynamicDimension() {
    if (m_hasDynamicDim) {
        m_hasDynamicDim = false;
        m_dynamicDimText.clear();
        update();
    }
}

void CadScene::onEntityAdded(std::shared_ptr<CadEntity> /*entity*/) {
    update();
}

void CadScene::onEntityRemoved(const QUuid& /*id*/) {
    update();
}

void CadScene::onEntityModified(std::shared_ptr<CadEntity> /*entity*/) {
    update();
}

void CadScene::onDocumentCleared() {
    clearPreview();
    clearPreviewEntities();
    clearSnapResult();
    clearDynamicDimension();
    clearSelectionBox();
    update();
}

void CadScene::onSelectionChanged() {
    update();
}

void CadScene::drawBackground(QPainter* painter, const QRectF& rect) {
    // 1. Viewport canvas fill
    painter->fillRect(rect, CadTheme::canvasBackground(m_themeMode));

    double gridSpacing = m_gridSize;
    int majorMult = 10;
    if (m_document) {
        gridSpacing = m_document->gridSettings().spacing();
        majorMult = m_document->gridSettings().majorMultiplier();
    }

    // 2. Render Page / Paper Boundary if enabled
    if (m_pageBorderVisible && m_document) {
        QRectF pageRect = m_document->pageSettings().pageRect();

        // Drop shadow around sheet
        painter->fillRect(pageRect.translated(3.0, 3.0), QColor(0, 0, 0, m_themeMode == CadTheme::ThemeMode::Dark ? 100 : 40));

        // Drawing sheet fill
        painter->fillRect(pageRect, CadTheme::paperBackground(m_themeMode));

        // Printable drawing limits
        QRectF limits = m_document->pageSettings().drawingLimits();
        QPen limitsPen(CadTheme::paperLimits(m_themeMode), 1.0, Qt::DashLine);
        limitsPen.setCosmetic(true);
        painter->setPen(limitsPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(limits);

        // Page sheet border
        QPen borderPen(CadTheme::paperBorder(m_themeMode), 1.2, Qt::SolidLine);
        borderPen.setCosmetic(true);
        painter->setPen(borderPen);
        painter->drawRect(pageRect);
    }

    // 3. Render CAD Grid with Adaptive LOD
    if (!m_gridVisible || gridSpacing < 0.05) return;

    double zoom = painter->transform().m11();
    if (zoom < 1e-4) zoom = 1.0;

    // Adapt grid spacing so lines don't clamp into a solid blur when zoomed far out
    double effectiveSpacing = gridSpacing;
    while (effectiveSpacing * zoom < 10.0) {
        effectiveSpacing *= 5.0;
        if (effectiveSpacing * zoom >= 10.0) break;
        effectiveSpacing *= 2.0;
    }

    qreal left = std::floor(rect.left() / effectiveSpacing) * effectiveSpacing;
    qreal top = std::floor(rect.top() / effectiveSpacing) * effectiveSpacing;

    QPen minorPen(CadTheme::minorGridColor(m_themeMode), 0.5, Qt::SolidLine);
    minorPen.setCosmetic(true);

    double majorSpacing = effectiveSpacing * majorMult;
    QPen majorPen(CadTheme::majorGridColor(m_themeMode), 0.8, Qt::SolidLine);
    majorPen.setCosmetic(true);

    QPen xAxisPen(CadTheme::xAxisColor(m_themeMode), 1.2, Qt::SolidLine);
    xAxisPen.setCosmetic(true);
    QPen yAxisPen(CadTheme::yAxisColor(m_themeMode), 1.2, Qt::SolidLine);
    yAxisPen.setCosmetic(true);

    for (qreal x = left; x <= rect.right(); x += effectiveSpacing) {
        bool isMajor = (std::abs(std::fmod(x, majorSpacing)) < 1e-3);
        bool isOrigin = (std::abs(x) < 1e-3);

        if (isOrigin) {
            painter->setPen(yAxisPen);
        } else if (isMajor) {
            painter->setPen(majorPen);
        } else {
            painter->setPen(minorPen);
        }
        painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
    }

    for (qreal y = top; y <= rect.bottom(); y += effectiveSpacing) {
        bool isMajor = (std::abs(std::fmod(y, majorSpacing)) < 1e-3);
        bool isOrigin = (std::abs(y) < 1e-3);

        if (isOrigin) {
            painter->setPen(xAxisPen);
        } else if (isMajor) {
            painter->setPen(majorPen);
        } else {
            painter->setPen(minorPen);
        }
        painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
    }
}

void CadScene::drawForeground(QPainter* painter, const QRectF& /*rect*/) {
    painter->setRenderHint(QPainter::Antialiasing, true);

    double zoom = painter->transform().m11();
    if (zoom < 1e-4) zoom = 1.0;

    // 1. Render all document entities with layer visibility check
    if (m_document) {
        for (const auto& entity : m_document->entities()) {
            if (!entity) continue;

            if (m_document->layerManager()) {
                auto layer = m_document->layerManager()->findLayer(entity->layer());
                if (layer && !layer->isVisible()) {
                    continue;
                }
            }

            renderEntity(painter, entity.get(), false);
        }
    }

    // 2. Render tool preview entity
    if (m_previewEntity) {
        renderEntity(painter, m_previewEntity.get(), true);
    }

    // Render multi-entity preview (e.g. AI floor plan generator preview)
    for (const auto& entity : m_previewEntities) {
        if (entity) {
            renderEntity(painter, entity.get(), true);
        }
    }

    // 3. Render Selection Box (AutoCAD Window vs Crossing selection)
    if (m_hasSelectionBox) {
        qreal x1 = std::min(m_selBoxStart.x(), m_selBoxEnd.x());
        qreal y1 = std::min(m_selBoxStart.y(), m_selBoxEnd.y());
        qreal x2 = std::max(m_selBoxStart.x(), m_selBoxEnd.x());
        qreal y2 = std::max(m_selBoxStart.y(), m_selBoxEnd.y());
        QRectF box(QPointF(x1, y1), QPointF(x2, y2));

        bool isWindowSelection = (m_selBoxStart.x() <= m_selBoxEnd.x());
        if (isWindowSelection) {
            // Window selection: Blue solid
            painter->fillRect(box, CadTheme::selectionWindowFill());
            QPen boxPen(CadTheme::selectionWindowBorder(), 1.0, Qt::SolidLine);
            boxPen.setCosmetic(true);
            painter->setPen(boxPen);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(box);
        } else {
            // Crossing selection: Green dashed
            painter->fillRect(box, CadTheme::selectionCrossingFill());
            QPen boxPen(CadTheme::selectionCrossingBorder(), 1.0, Qt::DashLine);
            boxPen.setCosmetic(true);
            painter->setPen(boxPen);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(box);
        }
    }

    // 4. Render UCS origin icon
    drawUcsIcon(painter, zoom);

    // 5. Render snap marker
    if (m_snapResult.snapped) {
        SnapManager::drawSnapMarker(painter, m_snapResult, zoom);
    }

    // 6. Render Dynamic Dimension HUD near cursor
    if (m_hasDynamicDim && !m_dynamicDimText.isEmpty()) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        QFont font = painter->font();
        font.setPointSizeF(9.0);
        painter->setFont(font);

        QFontMetricsF fm(font);
        QRectF textRect = fm.boundingRect(m_dynamicDimText);
        double pad = 4.0 / zoom;
        double badgeW = textRect.width() / zoom + pad * 2.0;
        double badgeH = textRect.height() / zoom + pad * 1.5;

        QPointF badgePos = m_dynamicDimPos + QPointF(15.0 / zoom, -badgeH - 8.0 / zoom);
        QRectF badgeRect(badgePos.x(), badgePos.y(), badgeW, badgeH);

        painter->setPen(QPen(QColor(0, 180, 255, 200), 1.0));
        painter->setBrush(QColor(20, 24, 32, 230));
        painter->drawRoundedRect(badgeRect, 3.0 / zoom, 3.0 / zoom);

        painter->setPen(QColor(240, 245, 255));
        painter->drawText(badgeRect, Qt::AlignCenter, m_dynamicDimText);
        painter->restore();
    }
}

void CadScene::drawUcsIcon(QPainter* painter, double zoom) {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    double ucsLen = 35.0 / zoom;
    double arrowLen = 8.0 / zoom;
    double boxSize = 5.0 / zoom;

    // Small origin square at (0, 0)
    QPen originPen(CadTheme::ucsOriginColor(m_themeMode), 1.2);
    originPen.setCosmetic(true);
    painter->setPen(originPen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(QRectF(-boxSize * 0.5, -boxSize * 0.5, boxSize, boxSize));

    // X Axis (Red)
    QPen xPen(QColor(230, 50, 50), 1.5);
    xPen.setCosmetic(true);
    painter->setPen(xPen);
    painter->drawLine(QPointF(0, 0), QPointF(ucsLen, 0));
    painter->drawLine(QPointF(ucsLen, 0), QPointF(ucsLen - arrowLen * 0.7, -arrowLen * 0.35));
    painter->drawLine(QPointF(ucsLen, 0), QPointF(ucsLen - arrowLen * 0.7, arrowLen * 0.35));

    // Y Axis (Green)
    QPen yPen(QColor(40, 180, 50), 1.5);
    yPen.setCosmetic(true);
    painter->setPen(yPen);
    painter->drawLine(QPointF(0, 0), QPointF(0, ucsLen));
    painter->drawLine(QPointF(0, ucsLen), QPointF(-arrowLen * 0.35, ucsLen - arrowLen * 0.7));
    painter->drawLine(QPointF(0, ucsLen), QPointF(arrowLen * 0.35, ucsLen - arrowLen * 0.7));

    // Text labels "X" and "Y"
    QFont font = painter->font();
    font.setPointSizeF(8.0);
    font.setBold(true);
    painter->setFont(font);

    painter->setPen(QColor(230, 50, 50));
    painter->drawText(QPointF(ucsLen + 3.0 / zoom, 4.0 / zoom), QStringLiteral("X"));

    painter->setPen(QColor(40, 180, 50));
    painter->drawText(QPointF(-4.0 / zoom, ucsLen + 10.0 / zoom), QStringLiteral("Y"));

    painter->restore();
}

void CadScene::renderDimension(QPainter* painter, const DimensionEntity* dim, const QColor& strokeColor, double width, bool isPreview) {
    if (!dim) return;

    QPen pen(strokeColor, width, isPreview ? Qt::DashLine : Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    QPointF p1 = dim->p1();
    QPointF p2 = dim->p2();
    QPointF d = dim->dimPoint();

    double dx = p2.x() - p1.x();
    double dy = p2.y() - p1.y();
    double len = std::hypot(dx, dy);

    if (dim->dimensionType() == DimensionType::Radius || dim->dimensionType() == DimensionType::Diameter) {
        painter->drawLine(p1, p2);

        if (len > 1e-4) {
            double ux = dx / len;
            double uy = dy / len;
            double tick = 4.0;
            painter->drawLine(p2, p2 - QPointF((ux - uy * 0.4) * tick, (uy + ux * 0.4) * tick));
            painter->drawLine(p2, p2 - QPointF((ux + uy * 0.4) * tick, (uy - ux * 0.4) * tick));
        }

        QPointF midPt = (p1 + p2) * 0.5;
        painter->save();
        painter->translate(midPt);
        QFont font = painter->font();
        font.setPointSizeF(9.0);
        painter->setFont(font);
        QString txt = dim->dimensionText();
        QFontMetricsF fm(font);
        QRectF txtRect = fm.boundingRect(txt);
        painter->setPen(strokeColor);
        painter->drawText(QPointF(-txtRect.width() * 0.5, -4.0), txt);
        painter->restore();
    } else {
        if (len > 1e-4) {
            double ux = dx / len;
            double uy = dy / len;
            double nx = -uy;
            double ny = ux;

            double projOffset = (d.x() - p1.x()) * nx + (d.y() - p1.y()) * ny;
            QPointF dp1 = p1 + QPointF(nx * projOffset, ny * projOffset);
            QPointF dp2 = p2 + QPointF(nx * projOffset, ny * projOffset);

            // Witness lines
            QPen witnessPen(strokeColor, std::max(1.0, width * 0.6), Qt::SolidLine);
            witnessPen.setCosmetic(true);
            painter->setPen(witnessPen);
            painter->drawLine(p1, dp1 + QPointF(nx * (projOffset >= 0 ? 3.0 : -3.0), ny * (projOffset >= 0 ? 3.0 : -3.0)));
            painter->drawLine(p2, dp2 + QPointF(nx * (projOffset >= 0 ? 3.0 : -3.0), ny * (projOffset >= 0 ? 3.0 : -3.0)));

            // Dimension line
            painter->setPen(pen);
            painter->drawLine(dp1, dp2);

            // Arrowheads / tick marks
            double tick = 4.0;
            painter->drawLine(dp1 - QPointF((ux + nx) * tick, (uy + ny) * tick),
                              dp1 + QPointF((ux + nx) * tick, (uy + ny) * tick));
            painter->drawLine(dp2 - QPointF((ux + nx) * tick, (uy + ny) * tick),
                              dp2 + QPointF((ux + nx) * tick, (uy + ny) * tick));

            // Measurement label
            QPointF midDim = (dp1 + dp2) * 0.5;
            painter->save();
            painter->translate(midDim);
            double angleDeg = std::atan2(dy, dx) * 180.0 / M_PI;
            if (angleDeg > 90.0 || angleDeg < -90.0) angleDeg += 180.0;
            painter->rotate(angleDeg);

            QFont font = painter->font();
            font.setPointSizeF(9.0);
            painter->setFont(font);
            QString txt = dim->dimensionText();
            QFontMetricsF fm(font);
            QRectF txtRect = fm.boundingRect(txt);
            painter->setPen(strokeColor);
            painter->drawText(QPointF(-txtRect.width() * 0.5, -3.0), txt);
            painter->restore();
        }
    }
}

void CadScene::renderEntity(QPainter* painter, const CadEntity* entity, bool isPreview) {
    if (!entity) return;

    QColor strokeColor = entity->color();
    Qt::PenStyle penStyle = Qt::SolidLine;
    double width = entity->strokeWidth();

    // Check layer color if entity color is default white/black
    if (m_document && m_document->layerManager()) {
        auto layer = m_document->layerManager()->findLayer(entity->layer());
        if (layer && (strokeColor == Qt::white || strokeColor == Qt::black)) {
            strokeColor = layer->color();
        }
    }

    if (isPreview) {
        strokeColor = QColor(0, 180, 255);
        penStyle = Qt::DashLine;
        width = 1.5;
    } else if (entity->isSelected()) {
        strokeColor = QColor(0, 220, 255);
        penStyle = Qt::SolidLine;
        width = std::max(2.0, width + 0.8);
    }

    QPen pen(strokeColor, width, penStyle, Qt::RoundCap, Qt::RoundJoin);
    pen.setCosmetic(true);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    const qreal r = 3.5;
    auto drawHandle = [&](const QPointF& pt) {
        painter->setBrush(QColor(0, 220, 255));
        QPen handlePen(Qt::white, 1);
        handlePen.setCosmetic(true);
        painter->setPen(handlePen);
        painter->drawRect(QRectF(pt.x() - r, pt.y() - r, 2 * r, 2 * r));
    };

    switch (entity->type()) {
        case EntityType::Line: {
            const auto* line = static_cast<const LineEntity*>(entity);
            painter->drawLine(line->p1(), line->p2());

            if (entity->isSelected() && !isPreview) {
                drawHandle(line->p1());
                drawHandle(line->p2());
            }
            break;
        }
        case EntityType::Circle: {
            const auto* circle = static_cast<const CircleEntity*>(entity);
            painter->drawEllipse(circle->center(), circle->radius(), circle->radius());

            if (entity->isSelected() && !isPreview) {
                drawHandle(circle->center());
                drawHandle(QPointF(circle->center().x() + circle->radius(), circle->center().y()));
                drawHandle(QPointF(circle->center().x() - circle->radius(), circle->center().y()));
                drawHandle(QPointF(circle->center().x(), circle->center().y() + circle->radius()));
                drawHandle(QPointF(circle->center().x(), circle->center().y() - circle->radius()));
            }
            break;
        }
        case EntityType::Rectangle: {
            const auto* rect = static_cast<const RectangleEntity*>(entity);
            painter->drawPolygon(rect->corners().data(), 4);

            if (entity->isSelected() && !isPreview) {
                for (size_t i = 0; i < 4; ++i) {
                    drawHandle(rect->corners()[i]);
                }
            }
            break;
        }
        case EntityType::Arc: {
            const auto* arc = static_cast<const ArcEntity*>(entity);
            QRectF rect(arc->center().x() - arc->radius(),
                        arc->center().y() - arc->radius(),
                        2.0 * arc->radius(),
                        2.0 * arc->radius());
            painter->drawArc(rect, static_cast<int>(arc->startAngle() * 16.0), static_cast<int>(arc->sweepAngle() * 16.0));

            if (entity->isSelected() && !isPreview) {
                drawHandle(arc->center());
                drawHandle(arc->startPoint());
                drawHandle(arc->endPoint());
                drawHandle(arc->midPoint());
            }
            break;
        }
        case EntityType::Polyline: {
            const auto* poly = static_cast<const PolylineEntity*>(entity);
            const auto& pts = poly->points();
            if (pts.size() >= 2) {
                if (poly->isClosed()) {
                    painter->drawPolygon(pts.data(), static_cast<int>(pts.size()));
                } else {
                    painter->drawPolyline(pts.data(), static_cast<int>(pts.size()));
                }

                if (entity->isSelected() && !isPreview) {
                    for (const auto& pt : pts) {
                        drawHandle(pt);
                    }
                }
            }
            break;
        }
        case EntityType::Polygon: {
            const auto* polygon = static_cast<const PolygonEntity*>(entity);
            auto v = polygon->vertices();
            if (v.size() >= 3) {
                painter->drawPolygon(v.data(), static_cast<int>(v.size()));

                if (entity->isSelected() && !isPreview) {
                    drawHandle(polygon->center());
                    for (const auto& pt : v) {
                        drawHandle(pt);
                    }
                }
            }
            break;
        }
        case EntityType::Ellipse: {
            const auto* ellipse = static_cast<const EllipseEntity*>(entity);
            painter->save();
            painter->translate(ellipse->center());
            painter->rotate(ellipse->rotationDeg());
            painter->drawEllipse(QPointF(0, 0), ellipse->radiusX(), ellipse->radiusY());

            if (entity->isSelected() && !isPreview) {
                drawHandle(QPointF(0, 0));
                drawHandle(QPointF(ellipse->radiusX(), 0));
                drawHandle(QPointF(-ellipse->radiusX(), 0));
                drawHandle(QPointF(0, ellipse->radiusY()));
                drawHandle(QPointF(0, -ellipse->radiusY()));
            }
            painter->restore();
            break;
        }
        case EntityType::Text: {
            const auto* text = static_cast<const TextEntity*>(entity);
            painter->save();
            painter->translate(text->position());
            painter->rotate(text->rotationDeg());

            QFont font;
            font.setPixelSize(static_cast<int>(std::max(1.0, text->height())));
            painter->setFont(font);
            painter->setPen(strokeColor);
            painter->drawText(QPointF(0, 0), text->text());

            if (entity->isSelected() && !isPreview) {
                drawHandle(QPointF(0, 0));
            }
            painter->restore();
            break;
        }
        case EntityType::Dimension: {
            const auto* dim = static_cast<const DimensionEntity*>(entity);
            renderDimension(painter, dim, strokeColor, width, isPreview);
            if (entity->isSelected() && !isPreview) {
                drawHandle(dim->p1());
                drawHandle(dim->p2());
                drawHandle(dim->dimPoint());
            }
            break;
        }
        default:
            break;
    }
}
