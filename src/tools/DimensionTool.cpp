#include "tools/DimensionTool.h"
#include "canvas/CadView.h"
#include "canvas/CadScene.h"
#include "document/CadDocument.h"
#include "geometry/DimensionEntity.h"
#include "geometry/LineEntity.h"
#include "geometry/CircleEntity.h"
#include "geometry/RectangleEntity.h"
#include "geometry/ArcEntity.h"
#include "commands/CreateEntityCommand.h"
#include "ui/CadCommandLine.h"

#include <cmath>

DimensionTool::DimensionTool(CadDocument* doc, CadView* view, CadCommandLine* cmdLine)
    : Tool(doc, view)
    , m_cmdLine(cmdLine)
{
}

void DimensionTool::activate() {
    m_state = State::WaitingForP1;
    m_targetEntityId = QUuid();
    m_targetFeatureIndex = 0;
    if (m_view) {
        m_view->setCursor(Qt::CrossCursor);
    }
    if (m_cmdLine) {
        m_cmdLine->setPrompt(QObject::tr("DIMENSION Specify first extension line origin or select entity:"));
        m_cmdLine->hideFields();
    }
}

void DimensionTool::deactivate() {
    cancel();
}

void DimensionTool::mousePress(QMouseEvent* event, const QPointF& worldPos) {
    if (!m_document || !m_view) return;

    if (event->button() == Qt::RightButton) {
        cancel();
        return;
    }

    if (event->button() != Qt::LeftButton) return;

    if (m_state == State::WaitingForP1) {
        // Check if user clicked on an existing entity to create an associative dimension
        double tol = 8.0 / (m_view->transform().m11() > 1e-4 ? m_view->transform().m11() : 1.0);
        std::shared_ptr<CadEntity> hitEnt;
        for (const auto& ent : m_document->entities()) {
            if (ent && ent->type() != EntityType::Dimension && ent->hitTest(worldPos, tol)) {
                hitEnt = ent;
                break;
            }
        }

        if (hitEnt) {
            m_targetEntityId = hitEnt->id();
            if (hitEnt->type() == EntityType::Line) {
                auto line = std::static_pointer_cast<LineEntity>(hitEnt);
                m_p1 = line->p1();
                m_p2 = line->p2();
                m_dimType = DimensionType::Aligned;
                m_state = State::WaitingForDimPoint;
            } else if (hitEnt->type() == EntityType::Circle) {
                auto circle = std::static_pointer_cast<CircleEntity>(hitEnt);
                m_p1 = circle->center();
                m_p2 = circle->center() + QPointF(circle->radius(), 0.0);
                m_dimType = DimensionType::Radius;
                m_state = State::WaitingForDimPoint;
            } else if (hitEnt->type() == EntityType::Rectangle) {
                auto rect = std::static_pointer_cast<RectangleEntity>(hitEnt);
                const auto& c = rect->corners();
                // Find closest edge
                int closestEdge = 0;
                double minD = 1e9;
                for (int i = 0; i < 4; ++i) {
                    QPointF mid = (c[i] + c[(i + 1) % 4]) * 0.5;
                    double d = std::hypot(worldPos.x() - mid.x(), worldPos.y() - mid.y());
                    if (d < minD) {
                        minD = d;
                        closestEdge = i;
                    }
                }
                m_targetFeatureIndex = closestEdge;
                m_p1 = c[closestEdge];
                m_p2 = c[(closestEdge + 1) % 4];
                m_dimType = DimensionType::Aligned;
                m_state = State::WaitingForDimPoint;
            } else {
                m_p1 = worldPos;
                m_state = State::WaitingForP2;
            }
        } else {
            m_p1 = worldPos;
            m_state = State::WaitingForP2;
        }

        if (m_view->snapManager()) {
            m_view->snapManager()->setBasePoint(m_p1, true);
        }

        if (m_cmdLine) {
            if (m_state == State::WaitingForDimPoint) {
                m_cmdLine->setPrompt(QObject::tr("DIMENSION Specify dimension line location:"));
            } else {
                m_cmdLine->setPrompt(QObject::tr("DIMENSION Specify second extension line origin:"));
            }
        }
    } else if (m_state == State::WaitingForP2) {
        m_p2 = worldPos;
        m_state = State::WaitingForDimPoint;

        if (m_cmdLine) {
            m_cmdLine->setPrompt(QObject::tr("DIMENSION Specify dimension line location:"));
        }

        auto preview = std::make_shared<DimensionEntity>(m_p1, m_p2, worldPos, m_dimType);
        if (m_view->cadScene()) {
            m_view->cadScene()->setPreviewEntity(preview);
        }
    } else if (m_state == State::WaitingForDimPoint) {
        double dist = std::hypot(m_p2.x() - m_p1.x(), m_p2.y() - m_p1.y());
        if (dist > 1e-4) {
            auto dim = std::make_shared<DimensionEntity>(m_p1, m_p2, worldPos, m_dimType);
            if (!m_targetEntityId.isNull()) {
                dim->setTargetEntityId(m_targetEntityId);
                dim->setTargetFeatureIndex(m_targetFeatureIndex);
            }
            dim->setUnitSuffix(m_document->unitSettings().unitSuffix());
            dim->setPrecision(m_document->unitSettings().precision());
            m_document->undoStack()->push(new CreateEntityCommand(m_document, dim));
        }

        cancel();
    }
}

void DimensionTool::mouseMove(QMouseEvent* /*event*/, const QPointF& worldPos) {
    if (!m_view || !m_view->cadScene()) return;

    const auto& units = m_document->unitSettings();

    if (m_state == State::WaitingForP2) {
        double dist = std::hypot(worldPos.x() - m_p1.x(), worldPos.y() - m_p1.y());
        auto preview = std::make_shared<DimensionEntity>(m_p1, worldPos, worldPos, m_dimType);
        m_view->cadScene()->setPreviewEntity(preview);

        QString hudText = QStringLiteral("Dimension: %1").arg(units.formatLinear(dist));
        m_view->cadScene()->setDynamicDimension(hudText, worldPos);
    } else if (m_state == State::WaitingForDimPoint) {
        auto preview = std::make_shared<DimensionEntity>(m_p1, m_p2, worldPos, m_dimType);
        m_view->cadScene()->setPreviewEntity(preview);

        QString hudText = QStringLiteral("Dimension: %1").arg(preview->dimensionText());
        m_view->cadScene()->setDynamicDimension(hudText, worldPos);
    }
}

void DimensionTool::keyPress(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        cancel();
    }
}

void DimensionTool::cancel() {
    m_state = State::WaitingForP1;
    m_targetEntityId = QUuid();
    m_targetFeatureIndex = 0;
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
