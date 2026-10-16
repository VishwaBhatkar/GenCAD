#include "tools/CircleTool.h"
#include "canvas/CadView.h"
#include "canvas/CadScene.h"
#include "document/CadDocument.h"
#include "document/UnitSystem.h"
#include "geometry/CircleEntity.h"
#include "geometry/DimensionEntity.h"
#include "commands/CreateEntityCommand.h"
#include "ui/CadCommandLine.h"

#include <QUndoCommand>
#include <cmath>

CircleTool::CircleTool(CadDocument* doc, CadView* view, CadCommandLine* cmdLine)
    : Tool(doc, view)
    , m_cmdLine(cmdLine)
{
}

void CircleTool::activate() {
    m_state = State::WaitingForCenter;
    if (m_view) {
        m_view->setCursor(Qt::CrossCursor);
    }
    if (m_cmdLine) {
        m_cmdLine->setPrompt(QObject::tr("CIRCLE Specify center point:"));
        m_cmdLine->showFields(QObject::tr("Center X:"), QObject::tr("Center Y:"));
    }
}

void CircleTool::deactivate() {
    cancel();
}

void CircleTool::mousePress(QMouseEvent* event, const QPointF& worldPos) {
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
            m_cmdLine->setPrompt(QObject::tr("CIRCLE Specify radius or diameter:"));
            m_cmdLine->showFields(QObject::tr("Radius:"), QObject::tr("Diameter:"));
        }

        auto preview = std::make_shared<CircleEntity>(m_center, 0.0);
        if (m_view->cadScene()) {
            m_view->cadScene()->setPreviewEntity(preview);
        }
    } else if (m_state == State::WaitingForRadius) {
        double radius = std::hypot(worldPos.x() - m_center.x(), worldPos.y() - m_center.y());

        if (radius > 1e-4 && !std::isnan(radius) && !std::isinf(radius)) {
            const auto& units = m_document->unitSettings();
            auto circle = std::make_shared<CircleEntity>(m_center, radius);
            auto* parentCmd = new QUndoCommand(QStringLiteral("Create Circle"));
            new CreateEntityCommand(m_document, circle, parentCmd);

            QPointF dimPt = m_center + QPointF(radius * 0.7071, radius * 0.7071);
            auto dim = std::make_shared<DimensionEntity>(m_center, m_center + QPointF(radius, 0.0), dimPt, DimensionType::Radius);
            dim->setTargetEntityId(circle->id());
            dim->setTargetFeatureIndex(0);
            dim->setUnitSuffix(units.unitSuffix());
            dim->setPrecision(units.precision());
            new CreateEntityCommand(m_document, dim, parentCmd);

            m_document->undoStack()->push(parentCmd);
        }

        cancel();
    }
}

void CircleTool::mouseMove(QMouseEvent* /*event*/, const QPointF& worldPos) {
    if (m_state == State::WaitingForCenter) {
        if (m_cmdLine) {
            const auto& units = m_document->unitSettings();
            m_cmdLine->updateFieldValues(worldPos.x() * units.scaleFromMm(),
                                         worldPos.y() * units.scaleFromMm(),
                                         0.0, units.precision());
        }
    } else if (m_state == State::WaitingForRadius && m_view && m_view->cadScene()) {
        double radius = std::hypot(worldPos.x() - m_center.x(), worldPos.y() - m_center.y());
        auto preview = std::make_shared<CircleEntity>(m_center, radius);
        m_view->cadScene()->setPreviewEntity(preview);

        const auto& units = m_document->unitSettings();
        QString hudText = QStringLiteral("Radius: %1\nDiameter: %2")
            .arg(units.formatLinear(radius))
            .arg(units.formatLinear(2.0 * radius));
        m_view->cadScene()->setDynamicDimension(hudText, worldPos);

        if (m_cmdLine) {
            double rDisp = radius * units.scaleFromMm();
            m_cmdLine->updateFieldValues(rDisp, 2.0 * rDisp, 0.0, units.precision());
        }
    }
}

void CircleTool::commitExact(double radius, double diameter) {
    if (!m_document) return;
    if (std::isnan(radius) || std::isinf(radius) || std::isnan(diameter) || std::isinf(diameter)) {
        return;
    }

    const auto& units = m_document->unitSettings();
    double rMm = 0.0;
    DimensionType dimType = DimensionType::Radius;
    if (diameter > 1e-4 && radius <= 1e-4) {
        rMm = (diameter * units.scaleToMm()) * 0.5;
        dimType = DimensionType::Diameter;
    } else {
        rMm = radius * units.scaleToMm();
    }

    if (rMm > 1e-4 && !std::isnan(rMm) && !std::isinf(rMm)) {
        auto circle = std::make_shared<CircleEntity>(m_center, rMm);
        auto* parentCmd = new QUndoCommand(QStringLiteral("Create Circle"));
        new CreateEntityCommand(m_document, circle, parentCmd);

        QPointF dimPt = m_center + QPointF(rMm * 0.7071, rMm * 0.7071);
        auto dim = std::make_shared<DimensionEntity>(m_center, m_center + QPointF(rMm, 0.0), dimPt, dimType);
        dim->setTargetEntityId(circle->id());
        dim->setTargetFeatureIndex(0);
        dim->setUnitSuffix(units.unitSuffix());
        dim->setPrecision(units.precision());
        new CreateEntityCommand(m_document, dim, parentCmd);

        m_document->undoStack()->push(parentCmd);
    }
    cancel();
}

void CircleTool::keyPress(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        cancel();
    }
}

void CircleTool::cancel() {
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
