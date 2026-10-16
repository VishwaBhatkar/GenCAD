#include "tools/OffsetTool.h"
#include "canvas/CadView.h"
#include "canvas/CadScene.h"
#include "document/CadDocument.h"
#include "geometry/LineEntity.h"
#include "geometry/CircleEntity.h"
#include "geometry/ArcEntity.h"
#include "geometry/RectangleEntity.h"
#include "commands/CreateEntityCommand.h"

#include <cmath>

OffsetTool::OffsetTool(CadDocument* doc, CadView* view)
    : Tool(doc, view)
{
}

void OffsetTool::activate() {
    m_state = State::WaitingForEntity;
    m_selectedEntity.reset();
    if (m_view) {
        m_view->setCursor(Qt::CrossCursor);
    }
}

void OffsetTool::deactivate() {
    cancel();
}

void OffsetTool::mousePress(QMouseEvent* event, const QPointF& worldPos) {
    if (!m_document || !m_view) return;

    if (event->button() == Qt::RightButton) {
        cancel();
        return;
    }

    if (event->button() != Qt::LeftButton) return;

    if (m_state == State::WaitingForEntity) {
        const double tolerance = 8.0;
        for (const auto& entity : m_document->entities()) {
            if (entity && entity->hitTest(worldPos, tolerance)) {
                m_selectedEntity = entity;
                m_state = State::WaitingForSidePoint;
                break;
            }
        }
    } else if (m_state == State::WaitingForSidePoint) {
        if (m_selectedEntity) {
            auto offsetEntity = createOffsetEntity(m_selectedEntity.get(), worldPos);
            if (offsetEntity) {
                m_document->undoStack()->push(new CreateEntityCommand(m_document, offsetEntity));
            }
        }
        cancel();
    }
}

void OffsetTool::mouseMove(QMouseEvent* event, const QPointF& worldPos) {
    Q_UNUSED(event);
    if (!m_view || !m_view->cadScene()) return;

    if (m_state == State::WaitingForSidePoint && m_selectedEntity) {
        auto preview = createOffsetEntity(m_selectedEntity.get(), worldPos);
        if (preview) {
            m_view->cadScene()->setPreviewEntity(preview);
        }
    }
}

void OffsetTool::keyPress(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        cancel();
    }
}

void OffsetTool::cancel() {
    m_state = State::WaitingForEntity;
    m_selectedEntity.reset();
    if (m_view && m_view->cadScene()) {
        m_view->cadScene()->clearPreview();
    }
}

std::shared_ptr<CadEntity> OffsetTool::createOffsetEntity(const CadEntity* entity, const QPointF& targetPt) const {
    if (!entity) return nullptr;

    switch (entity->type()) {
        case EntityType::Line: {
            const auto* line = static_cast<const LineEntity*>(entity);
            double dx = line->p2().x() - line->p1().x();
            double dy = line->p2().y() - line->p1().y();
            double len = std::hypot(dx, dy);
            if (len < 1e-4) return nullptr;

            double nx = -dy / len;
            double ny = dx / len;

            // Distance from line to targetPt along normal
            double dist = (targetPt.x() - line->p1().x()) * nx + (targetPt.y() - line->p1().y()) * ny;
            QPointF offP1(line->p1().x() + nx * dist, line->p1().y() + ny * dist);
            QPointF offP2(line->p2().x() + nx * dist, line->p2().y() + ny * dist);
            auto newLine = std::make_shared<LineEntity>(offP1, offP2);
            newLine->setColor(line->color());
            newLine->setStrokeWidth(line->strokeWidth());
            return newLine;
        }
        case EntityType::Circle: {
            const auto* circle = static_cast<const CircleEntity*>(entity);
            double r = std::hypot(targetPt.x() - circle->center().x(), targetPt.y() - circle->center().y());
            if (r < 0.1) return nullptr;
            auto newCircle = std::make_shared<CircleEntity>(circle->center(), r);
            newCircle->setColor(circle->color());
            newCircle->setStrokeWidth(circle->strokeWidth());
            return newCircle;
        }
        case EntityType::Arc: {
            const auto* arc = static_cast<const ArcEntity*>(entity);
            double r = std::hypot(targetPt.x() - arc->center().x(), targetPt.y() - arc->center().y());
            if (r < 0.1) return nullptr;
            auto newArc = std::make_shared<ArcEntity>(arc->center(), r, arc->startAngle(), arc->sweepAngle());
            newArc->setColor(arc->color());
            newArc->setStrokeWidth(arc->strokeWidth());
            return newArc;
        }
        case EntityType::Rectangle: {
            const auto* rect = static_cast<const RectangleEntity*>(entity);
            QRectF b = rect->boundingRect();
            QPointF center = b.center();
            double origW = b.width();
            double origH = b.height();
            if (origW < 1e-4 || origH < 1e-4) return nullptr;

            double dist = std::max(std::abs(targetPt.x() - center.x()) - origW * 0.5,
                                   std::abs(targetPt.y() - center.y()) - origH * 0.5);
            QRectF newB = b.adjusted(-dist, -dist, dist, dist);
            if (newB.width() <= 0.1 || newB.height() <= 0.1) return nullptr;

            auto newRect = std::make_shared<RectangleEntity>(newB.topLeft(), newB.width(), newB.height());
            newRect->setColor(rect->color());
            newRect->setStrokeWidth(rect->strokeWidth());
            return newRect;
        }
        default:
            return nullptr;
    }
}
