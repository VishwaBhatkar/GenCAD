#include "tools/EllipseTool.h"
#include "canvas/CadView.h"
#include "canvas/CadScene.h"
#include "document/CadDocument.h"
#include "geometry/EllipseEntity.h"
#include "commands/CreateEntityCommand.h"
#include "ui/CadCommandLine.h"

#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

EllipseTool::EllipseTool(CadDocument* doc, CadView* view, CadCommandLine* cmdLine)
    : Tool(doc, view)
    , m_cmdLine(cmdLine)
{
}

void EllipseTool::activate() {
    m_state = State::WaitingForCenter;
    if (m_view) {
        m_view->setCursor(Qt::CrossCursor);
    }
    if (m_cmdLine) {
        m_cmdLine->setPrompt(QObject::tr("ELLIPSE Specify center point:"));
        m_cmdLine->hideFields();
    }
}

void EllipseTool::deactivate() {
    cancel();
}

void EllipseTool::mousePress(QMouseEvent* event, const QPointF& worldPos) {
    if (!m_document || !m_view) return;

    if (event->button() == Qt::RightButton) {
        cancel();
        return;
    }

    if (event->button() != Qt::LeftButton) return;

    if (m_state == State::WaitingForCenter) {
        m_center = worldPos;
        m_state = State::WaitingForMajorAxis;

        if (m_view->snapManager()) {
            m_view->snapManager()->setBasePoint(m_center, true);
        }

        if (m_cmdLine) {
            m_cmdLine->setPrompt(QObject::tr("ELLIPSE Specify major and minor axis:"));
            m_cmdLine->showFields(QObject::tr("Major:"), QObject::tr("Minor:"), QObject::tr("Rotation:"));
            m_cmdLine->updateFieldValues(200.0, 100.0, 0.0, m_document->unitSettings().precision());
        }

        auto preview = std::make_shared<EllipseEntity>(m_center, 10.0, 5.0, 0.0);
        if (m_view->cadScene()) {
            m_view->cadScene()->setPreviewEntity(preview);
        }
    } else if (m_state == State::WaitingForMajorAxis) {
        double dx = worldPos.x() - m_center.x();
        double dy = worldPos.y() - m_center.y();
        m_radiusX = std::hypot(dx, dy);
        m_rotationDeg = std::atan2(dy, dx) * 180.0 / M_PI;
        if (m_radiusX < 1e-4) return;

        m_state = State::WaitingForMinorAxis;
    } else if (m_state == State::WaitingForMinorAxis) {
        // Calculate distance from worldPos to major axis line
        double rad = m_rotationDeg * M_PI / 180.0;
        double cosA = std::cos(rad);
        double sinA = std::sin(rad);
        double dx = worldPos.x() - m_center.x();
        double dy = worldPos.y() - m_center.y();
        double perpDist = std::abs(-dx * sinA + dy * cosA);
        m_radiusY = std::max(1e-4, perpDist);

        if (m_radiusX > 1e-4 && m_radiusY > 1e-4) {
            auto ellipse = std::make_shared<EllipseEntity>(m_center, m_radiusX, m_radiusY, m_rotationDeg);
            m_document->undoStack()->push(new CreateEntityCommand(m_document, ellipse));
        }

        cancel();
    }
}

void EllipseTool::mouseMove(QMouseEvent* /*event*/, const QPointF& worldPos) {
    if (!m_view || !m_view->cadScene()) return;

    const auto& units = m_document->unitSettings();

    if (m_state == State::WaitingForMajorAxis) {
        double dx = worldPos.x() - m_center.x();
        double dy = worldPos.y() - m_center.y();
        m_radiusX = std::hypot(dx, dy);
        m_radiusY = m_radiusX * 0.5;
        m_rotationDeg = std::atan2(dy, dx) * 180.0 / M_PI;
        if (m_rotationDeg < 0.0) m_rotationDeg += 360.0;

        auto preview = std::make_shared<EllipseEntity>(m_center, m_radiusX, m_radiusY, m_rotationDeg);
        m_view->cadScene()->setPreviewEntity(preview);

        QString hudText = QStringLiteral("Major: %1  Rotation: %2")
            .arg(units.formatLinear(2.0 * m_radiusX))
            .arg(units.formatAngle(m_rotationDeg));
        m_view->cadScene()->setDynamicDimension(hudText, worldPos);

        if (m_cmdLine) {
            m_cmdLine->updateFieldValues(2.0 * m_radiusX * units.scaleFromMm(),
                                         2.0 * m_radiusY * units.scaleFromMm(),
                                         m_rotationDeg, units.precision());
        }
    } else if (m_state == State::WaitingForMinorAxis) {
        double rad = m_rotationDeg * M_PI / 180.0;
        double sinA = std::sin(rad);
        double cosA = std::cos(rad);
        double dx = worldPos.x() - m_center.x();
        double dy = worldPos.y() - m_center.y();
        double perpDist = std::abs(-dx * sinA + dy * cosA);
        m_radiusY = std::max(1e-4, perpDist);

        auto preview = std::make_shared<EllipseEntity>(m_center, m_radiusX, m_radiusY, m_rotationDeg);
        m_view->cadScene()->setPreviewEntity(preview);

        QString hudText = QStringLiteral("Major: %1  Minor: %2  Rotation: %3")
            .arg(units.formatLinear(2.0 * m_radiusX))
            .arg(units.formatLinear(2.0 * m_radiusY))
            .arg(units.formatAngle(m_rotationDeg));
        m_view->cadScene()->setDynamicDimension(hudText, worldPos);

        if (m_cmdLine) {
            m_cmdLine->updateFieldValues(2.0 * m_radiusX * units.scaleFromMm(),
                                         2.0 * m_radiusY * units.scaleFromMm(),
                                         m_rotationDeg, units.precision());
        }
    }
}

void EllipseTool::commitExact(double majorAxis, double minorAxis, double rotationDeg) {
    if (!m_document) return;
    const auto& units = m_document->unitSettings();
    double rxMm = (majorAxis * units.scaleToMm()) * 0.5;
    double ryMm = (minorAxis * units.scaleToMm()) * 0.5;

    if (rxMm > 1e-4 && ryMm > 1e-4) {
        auto ellipse = std::make_shared<EllipseEntity>(m_center, rxMm, ryMm, rotationDeg);
        m_document->undoStack()->push(new CreateEntityCommand(m_document, ellipse));
    }
    cancel();
}

void EllipseTool::keyPress(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        cancel();
    }
}

void EllipseTool::cancel() {
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
