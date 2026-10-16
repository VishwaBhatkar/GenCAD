#include "tools/PolygonTool.h"
#include "canvas/CadView.h"
#include "canvas/CadScene.h"
#include "document/CadDocument.h"
#include "geometry/PolygonEntity.h"
#include "commands/CreateEntityCommand.h"
#include "ui/CadCommandLine.h"

#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PolygonTool::PolygonTool(CadDocument* doc, CadView* view, CadCommandLine* cmdLine)
    : Tool(doc, view)
    , m_cmdLine(cmdLine)
{
}

void PolygonTool::activate() {
    m_state = State::WaitingForCenter;
    if (m_view) {
        m_view->setCursor(Qt::CrossCursor);
    }
    if (m_cmdLine) {
        m_cmdLine->setPrompt(QObject::tr("POLYGON Specify center point:"));
        m_cmdLine->hideFields();
    }
}

void PolygonTool::deactivate() {
    cancel();
}

void PolygonTool::mousePress(QMouseEvent* event, const QPointF& worldPos) {
    if (!m_document || !m_view) return;

    if (event->button() == Qt::RightButton) {
        cancel();
        return;
    }

    if (event->button() != Qt::LeftButton) return;

    if (m_state == State::WaitingForCenter) {
        m_center = worldPos;
        m_state = State::WaitingForRadius;

        if (m_view->snapManager()) {
            m_view->snapManager()->setBasePoint(m_center, true);
        }

        if (m_cmdLine) {
            m_cmdLine->setPrompt(QObject::tr("POLYGON Specify radius and options:"));
            m_cmdLine->showFields(QObject::tr("Sides:"), QObject::tr("Radius:"), QObject::tr("Angle:"));
            m_cmdLine->updateFieldValues(m_sides, 100.0, 0.0, m_document->unitSettings().precision());
        }

        auto preview = std::make_shared<PolygonEntity>(m_center, m_sides, 10.0, 0.0);
        if (m_view->cadScene()) {
            m_view->cadScene()->setPreviewEntity(preview);
        }
    } else if (m_state == State::WaitingForRadius) {
        double dx = worldPos.x() - m_center.x();
        double dy = worldPos.y() - m_center.y();
        m_radius = std::hypot(dx, dy);
        m_rotationDeg = std::atan2(dy, dx) * 180.0 / M_PI;

        if (m_radius > 1e-4) {
            auto poly = std::make_shared<PolygonEntity>(m_center, m_sides, m_radius, m_rotationDeg);
            m_document->undoStack()->push(new CreateEntityCommand(m_document, poly));
        }

        cancel();
    }
}

void PolygonTool::mouseMove(QMouseEvent* /*event*/, const QPointF& worldPos) {
    if (m_state == State::WaitingForRadius && m_view && m_view->cadScene()) {
        double dx = worldPos.x() - m_center.x();
        double dy = worldPos.y() - m_center.y();
        m_radius = std::hypot(dx, dy);
        m_rotationDeg = std::atan2(dy, dx) * 180.0 / M_PI;
        if (m_rotationDeg < 0.0) m_rotationDeg += 360.0;

        auto preview = std::make_shared<PolygonEntity>(m_center, m_sides, m_radius, m_rotationDeg);
        m_view->cadScene()->setPreviewEntity(preview);

        const auto& units = m_document->unitSettings();
        QString hudText = QStringLiteral("Sides: %1  Radius: %2  Angle: %3")
            .arg(m_sides)
            .arg(units.formatLinear(m_radius))
            .arg(units.formatAngle(m_rotationDeg));
        m_view->cadScene()->setDynamicDimension(hudText, worldPos);

        if (m_cmdLine) {
            double displayRad = m_radius * units.scaleFromMm();
            m_cmdLine->updateFieldValues(m_sides, displayRad, m_rotationDeg, units.precision());
        }
    }
}

void PolygonTool::commitExact(double sides, double radius, double rotationDeg) {
    if (!m_document) return;
    int s = std::max(3, static_cast<int>(std::round(sides)));
    const auto& units = m_document->unitSettings();
    double radMm = radius * units.scaleToMm();

    if (radMm > 1e-4) {
        auto poly = std::make_shared<PolygonEntity>(m_center, s, radMm, rotationDeg);
        m_document->undoStack()->push(new CreateEntityCommand(m_document, poly));
    }
    cancel();
}

void PolygonTool::keyPress(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        cancel();
    }
}

void PolygonTool::cancel() {
    m_state = State::WaitingForCenter;
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
