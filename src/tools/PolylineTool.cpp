#include "tools/PolylineTool.h"
#include "canvas/CadView.h"
#include "canvas/CadScene.h"
#include "document/CadDocument.h"
#include "document/CoordinateSystem.h"
#include "geometry/PolylineEntity.h"
#include "geometry/DimensionEntity.h"
#include "commands/CreateEntityCommand.h"
#include "ui/CadCommandLine.h"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PolylineTool::PolylineTool(CadDocument* doc, CadView* view, CadCommandLine* cmdLine)
    : Tool(doc, view)
    , m_cmdLine(cmdLine)
{
}

void PolylineTool::activate() {
    m_points.clear();
    if (m_view) {
        m_view->setCursor(Qt::CrossCursor);
    }
    if (m_cmdLine) {
        m_cmdLine->setPrompt(QObject::tr("POLYLINE Specify start point:"));
        m_cmdLine->showFields(QObject::tr("Start X:"), QObject::tr("Start Y:"));
    }
}

void PolylineTool::deactivate() {
    cancel();
}

void PolylineTool::mousePress(QMouseEvent* event, const QPointF& worldPos) {
    if (!m_document || !m_view) return;

    if (event->button() == Qt::RightButton) {
        finish(false);
        return;
    }

    if (event->button() != Qt::LeftButton) return;

    m_points.push_back(worldPos);

    if (m_view->snapManager()) {
        m_view->snapManager()->setBasePoint(worldPos, true);
    }

    if (m_cmdLine) {
        m_cmdLine->setPrompt(QStringLiteral("POLYLINE Specify next point (%1 points):").arg(m_points.size()));
        m_cmdLine->showFields(QObject::tr("Length:"), QObject::tr("Angle:"));
    }

    auto preview = std::make_shared<PolylineEntity>(m_points, false);
    if (m_view->cadScene()) {
        m_view->cadScene()->setPreviewEntity(preview);
    }
}

void PolylineTool::mouseMove(QMouseEvent* /*event*/, const QPointF& worldPos) {
    if (m_points.empty()) {
        if (m_cmdLine) {
            const auto& units = m_document->unitSettings();
            m_cmdLine->updateFieldValues(worldPos.x() * units.scaleFromMm(),
                                         worldPos.y() * units.scaleFromMm(),
                                         0.0, units.precision());
        }
        return;
    }

    if (m_view && m_view->cadScene()) {
        auto pts = m_points;
        pts.push_back(worldPos);
        auto preview = std::make_shared<PolylineEntity>(pts, false);
        m_view->cadScene()->setPreviewEntity(preview);

        const auto& units = m_document->unitSettings();
        const QPointF& prev = m_points.back();
        double dx = worldPos.x() - prev.x();
        double dy = worldPos.y() - prev.y();

        bool isOrtho = (m_view && m_view->isOrthoEnabled());
        double length = 0.0;
        double angle = 0.0;
        QString hudText;

        if (isOrtho) {
            bool isH = CoordinateSystem::isOrthoHorizontal(prev, worldPos);
            angle = CoordinateSystem::orthoAngleDeg(prev, worldPos);
            length = isH ? std::abs(dx) : std::abs(dy);
            hudText = QStringLiteral("ORTHO\nDirection: %1\nAngle: %2°\nLength: %3")
                .arg(isH ? QStringLiteral("Horizontal") : QStringLiteral("Vertical"))
                .arg(angle, 0, 'f', 0)
                .arg(units.formatLinear(length));
        } else {
            length = std::hypot(dx, dy);
            angle = CoordinateSystem::angleDeg(prev, worldPos);
            hudText = QStringLiteral("Segment: %1  Angle: %2\nΔX: %3  ΔY: %4")
                .arg(units.formatLinear(length))
                .arg(units.formatAngle(angle))
                .arg(units.formatLinear(std::abs(dx)))
                .arg(units.formatLinear(std::abs(dy)));
        }

        m_view->cadScene()->setDynamicDimension(hudText, worldPos);

        if (m_cmdLine) {
            m_cmdLine->updateFieldValues(length * units.scaleFromMm(), angle, 0.0, units.precision());
        }
    }
}

void PolylineTool::commitExactSegment(double length, double angleDeg) {
    if (!m_document) return;
    const auto& units = m_document->unitSettings();

    if (m_points.empty()) {
        QPointF startPt(length * units.scaleToMm(), angleDeg * units.scaleToMm());
        m_points.push_back(startPt);
        if (m_view && m_view->snapManager()) {
            m_view->snapManager()->setBasePoint(startPt, true);
        }
        if (m_cmdLine) {
            m_cmdLine->setPrompt(QStringLiteral("POLYLINE Specify next point (%1 points):").arg(m_points.size()));
            m_cmdLine->showFields(QObject::tr("Length:"), QObject::tr("Angle:"));
        }
        return;
    }

    double lenMm = length * units.scaleToMm();
    if (lenMm <= 1e-4 || std::isnan(lenMm) || std::isinf(lenMm) || std::isnan(angleDeg) || std::isinf(angleDeg)) {
        if (m_cmdLine) {
            m_cmdLine->setPrompt(QObject::tr("Invalid dimension. Please enter a value greater than 0."));
        }
        return;
    }

    double finalAngle = CoordinateSystem::normalizeAngleDeg(angleDeg);
    if (m_view && m_view->isOrthoEnabled()) {
        finalAngle = CoordinateSystem::constrainToOrthoAngle(finalAngle);
    }

    QPointF nextPt = CoordinateSystem::polarPoint(m_points.back(), lenMm, finalAngle);
    m_points.push_back(nextPt);

    if (m_view && m_view->snapManager()) {
        m_view->snapManager()->setBasePoint(nextPt, true);
    }

    auto preview = std::make_shared<PolylineEntity>(m_points, false);
    if (m_view && m_view->cadScene()) {
        m_view->cadScene()->setPreviewEntity(preview);
    }

    if (m_cmdLine) {
        m_cmdLine->setPrompt(QStringLiteral("POLYLINE Specify next point (%1 points):").arg(m_points.size()));
    }
}

void PolylineTool::keyPress(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        cancel();
    } else if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        finish(false);
    } else if (event->key() == Qt::Key_C) {
        finish(true); // Close polyline
    }
}

void PolylineTool::finish(bool close) {
    if (m_points.size() >= 2 && m_document) {
        auto poly = std::make_shared<PolylineEntity>(m_points, close);
        auto* parentCmd = new QUndoCommand(QStringLiteral("Create Polyline"));
        new CreateEntityCommand(m_document, poly, parentCmd);

        const auto& units = m_document->unitSettings();
        size_t segCount = close ? m_points.size() : m_points.size() - 1;
        for (size_t i = 0; i < segCount; ++i) {
            const QPointF& p1 = m_points[i];
            const QPointF& p2 = m_points[(i + 1) % m_points.size()];
            double dx = p2.x() - p1.x();
            double dy = p2.y() - p1.y();
            double len = std::hypot(dx, dy);
            if (len > 1e-4) {
                QPointF normal(-dy / len, dx / len);
                QPointF dimPt = (p1 + p2) * 0.5 + normal * 15.0;
                DimensionType dimType = DimensionType::Aligned;
                if (m_view && m_view->isOrthoEnabled()) {
                    dimType = std::abs(dx) >= std::abs(dy) ? DimensionType::Horizontal : DimensionType::Vertical;
                }
                auto dim = std::make_shared<DimensionEntity>(p1, p2, dimPt, dimType);
                dim->setTargetEntityId(poly->id());
                dim->setTargetFeatureIndex(static_cast<int>(i));
                dim->setUnitSuffix(units.unitSuffix());
                dim->setPrecision(units.precision());
                new CreateEntityCommand(m_document, dim, parentCmd);
            }
        }

        m_document->undoStack()->push(parentCmd);
    }
    cancel();
}

void PolylineTool::cancel() {
    m_points.clear();
    if (m_view) {
        if (m_view->cadScene()) {
            m_view->cadScene()->clearPreview();
            m_view->cadScene()->clearDynamicDimension();
        }
        if (m_view->snapManager()) {
            m_view->snapManager()->clearBasePoint();
        }
    }
    if (m_cmdLine) {
        m_cmdLine->clearPrompt();
    }
}
