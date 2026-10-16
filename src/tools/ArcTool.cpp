#include "tools/ArcTool.h"
#include "canvas/CadView.h"
#include "canvas/CadScene.h"
#include "document/CadDocument.h"
#include "geometry/ArcEntity.h"
#include "commands/CreateEntityCommand.h"
#include "ui/CadCommandLine.h"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ArcTool::ArcTool(CadDocument* doc, CadView* view, CadCommandLine* cmdLine)
    : Tool(doc, view)
    , m_cmdLine(cmdLine)
{
}

void ArcTool::activate() {
    m_state = State::WaitingForCenter;
    if (m_view) {
        m_view->setCursor(Qt::CrossCursor);
    }
    if (m_cmdLine) {
        m_cmdLine->setPrompt(QObject::tr("ARC Specify center point:"));
        m_cmdLine->showFields(QObject::tr("Center X:"), QObject::tr("Center Y:"));
    }
}

void ArcTool::deactivate() {
    cancel();
}

void ArcTool::mousePress(QMouseEvent* event, const QPointF& worldPos) {
    if (!m_document || !m_view) return;

    if (event->button() == Qt::RightButton) {
        cancel();
        return;
    }

    if (event->button() != Qt::LeftButton) return;

    if (m_state == State::WaitingForCenter) {
        m_center = worldPos;
        m_state = State::WaitingForStartPoint;

        if (m_view->snapManager()) {
            m_view->snapManager()->setBasePoint(m_center, true);
        }

        if (m_cmdLine) {
            m_cmdLine->setPrompt(QObject::tr("ARC Specify start point or radius and start angle:"));
            m_cmdLine->showFields(QObject::tr("Radius:"), QObject::tr("Start Angle:"));
        }
    } else if (m_state == State::WaitingForStartPoint) {
        double dx = worldPos.x() - m_center.x();
        double dy = worldPos.y() - m_center.y();
        m_radius = std::hypot(dx, dy);

        if (m_radius < 1e-4) return;

        m_startAngle = std::atan2(dy, dx) * 180.0 / M_PI;
        if (m_startAngle < 0.0) m_startAngle += 360.0;
        m_state = State::WaitingForEndPoint;

        if (m_cmdLine) {
            m_cmdLine->setPrompt(QObject::tr("ARC Specify end point or end angle:"));
            m_cmdLine->showFields(QObject::tr("Radius:"), QObject::tr("Start Angle:"), QObject::tr("End Angle:"));
        }

        auto preview = std::make_shared<ArcEntity>(m_center, m_radius, m_startAngle, 30.0);
        if (m_view->cadScene()) {
            m_view->cadScene()->setPreviewEntity(preview);
        }
    } else if (m_state == State::WaitingForEndPoint) {
        double dx = worldPos.x() - m_center.x();
        double dy = worldPos.y() - m_center.y();
        double endAngle = std::atan2(dy, dx) * 180.0 / M_PI;
        if (endAngle < 0.0) endAngle += 360.0;

        double sweep = endAngle - m_startAngle;
        while (sweep <= 0.0) sweep += 360.0;
        while (sweep > 360.0) sweep -= 360.0;

        if (m_radius > 1e-4 && sweep > 0.01) {
            auto arc = std::make_shared<ArcEntity>(m_center, m_radius, m_startAngle, sweep);
            m_document->undoStack()->push(new CreateEntityCommand(m_document, arc));
        }

        cancel();
    }
}

void ArcTool::mouseMove(QMouseEvent* /*event*/, const QPointF& worldPos) {
    if (!m_view || !m_view->cadScene()) return;

    const auto& units = m_document->unitSettings();

    if (m_state == State::WaitingForStartPoint) {
        double r = std::hypot(worldPos.x() - m_center.x(), worldPos.y() - m_center.y());
        double a = std::atan2(worldPos.y() - m_center.y(), worldPos.x() - m_center.x()) * 180.0 / M_PI;
        if (a < 0.0) a += 360.0;

        if (r > 0.1) {
            auto preview = std::make_shared<ArcEntity>(m_center, r, 0.0, 360.0);
            m_view->cadScene()->setPreviewEntity(preview);
        }

        QString hudText = QStringLiteral("Radius: %1  Angle: %2")
            .arg(units.formatLinear(r))
            .arg(units.formatAngle(a));
        m_view->cadScene()->setDynamicDimension(hudText, worldPos);

        if (m_cmdLine) {
            m_cmdLine->updateFieldValues(r * units.scaleFromMm(), a, 0.0, units.precision());
        }
    } else if (m_state == State::WaitingForEndPoint) {
        double dx = worldPos.x() - m_center.x();
        double dy = worldPos.y() - m_center.y();
        double endAngle = std::atan2(dy, dx) * 180.0 / M_PI;
        if (endAngle < 0.0) endAngle += 360.0;

        double sweep = endAngle - m_startAngle;
        while (sweep <= 0.0) sweep += 360.0;
        while (sweep > 360.0) sweep -= 360.0;

        auto preview = std::make_shared<ArcEntity>(m_center, m_radius, m_startAngle, sweep);
        m_view->cadScene()->setPreviewEntity(preview);

        double arcLen = m_radius * (sweep * M_PI / 180.0);
        QString hudText = QStringLiteral("Radius: %1\nStart: %2  End: %3\nSweep: %4  ArcLen: %5")
            .arg(units.formatLinear(m_radius))
            .arg(units.formatAngle(m_startAngle))
            .arg(units.formatAngle(endAngle))
            .arg(units.formatAngle(sweep))
            .arg(units.formatLinear(arcLen));
        m_view->cadScene()->setDynamicDimension(hudText, worldPos);

        if (m_cmdLine) {
            m_cmdLine->updateFieldValues(m_radius * units.scaleFromMm(), m_startAngle, endAngle, units.precision());
        }
    }
}

void ArcTool::commitExact(double radius, double startAngle, double endAngle) {
    if (!m_document) return;
    const auto& units = m_document->unitSettings();
    double rMm = radius * units.scaleToMm();

    double sweep = endAngle - startAngle;
    while (sweep <= 0.0) sweep += 360.0;
    while (sweep > 360.0) sweep -= 360.0;

    if (rMm > 1e-4 && sweep > 0.01) {
        auto arc = std::make_shared<ArcEntity>(m_center, rMm, startAngle, sweep);
        m_document->undoStack()->push(new CreateEntityCommand(m_document, arc));
    }
    cancel();
}

void ArcTool::keyPress(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        cancel();
    }
}

void ArcTool::cancel() {
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
