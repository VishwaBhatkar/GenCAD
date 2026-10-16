#include "tools/MoveTool.h"
#include "canvas/CadView.h"
#include "canvas/CadScene.h"
#include "document/CadDocument.h"
#include "commands/MoveEntityCommand.h"
#include "geometry/RectangleEntity.h"

#include <cmath>

MoveTool::MoveTool(CadDocument* doc, CadView* view)
    : Tool(doc, view)
{
}

void MoveTool::activate() {
    m_state = State::WaitingForBasePoint;
    m_entitiesToMove.clear();

    if (m_document) {
        m_entitiesToMove = m_document->selectedIds();
    }

    if (m_view) {
        m_view->setCursor(Qt::SizeAllCursor);
    }
}

void MoveTool::deactivate() {
    cancel();
}

void MoveTool::mousePress(QMouseEvent* event, const QPointF& worldPos) {
    if (!m_document || !m_view) return;

    if (event->button() == Qt::RightButton) {
        cancel();
        return;
    }

    if (event->button() != Qt::LeftButton) return;

    if (m_state == State::WaitingForBasePoint) {
        // If nothing was selected before, try to pick an entity under the click
        if (m_entitiesToMove.empty()) {
            double scale = m_view->transform().m11();
            if (scale <= 0) scale = 1.0;
            double tolerance = 6.0 / scale;

            const auto& entities = m_document->entities();
            for (auto it = entities.rbegin(); it != entities.rend(); ++it) {
                if ((*it)->hitTest(worldPos, tolerance)) {
                    m_document->select((*it)->id(), false);
                    m_entitiesToMove.push_back((*it)->id());
                    break;
                }
            }
        }

        if (!m_entitiesToMove.empty()) {
            m_basePoint = worldPos;
            m_state = State::WaitingForDestinationPoint;
        }
    } else if (m_state == State::WaitingForDestinationPoint) {
        double deltaX = worldPos.x() - m_basePoint.x();
        double deltaY = worldPos.y() - m_basePoint.y();

        if (std::hypot(deltaX, deltaY) > 1e-4) {
            m_document->undoStack()->push(
                new MoveEntityCommand(m_document, m_entitiesToMove, deltaX, deltaY)
            );
        }

        cancel();
    }
}

void MoveTool::mouseMove(QMouseEvent* event, const QPointF& worldPos) {
    Q_UNUSED(event);
    if (m_state == State::WaitingForDestinationPoint && m_view && m_view->cadScene()) {
        double deltaX = worldPos.x() - m_basePoint.x();
        double deltaY = worldPos.y() - m_basePoint.y();

        if (m_entitiesToMove.size() == 1) {
            auto entity = m_document->findEntity(m_entitiesToMove.front());
            if (entity) {
                auto preview = entity->clone();
                preview->move(deltaX, deltaY);
                m_view->cadScene()->setPreviewEntity(preview);
            }
        } else if (m_entitiesToMove.size() > 1) {
            // Preview moved bounding box of selection
            QRectF box;
            bool first = true;
            for (const auto& id : m_entitiesToMove) {
                auto e = m_document->findEntity(id);
                if (e) {
                    if (first) {
                        box = e->boundingRect();
                        first = false;
                    } else {
                        box = box.united(e->boundingRect());
                    }
                }
            }
            if (!first) {
                box.translate(deltaX, deltaY);
                auto previewBox = std::make_shared<RectangleEntity>(box.topLeft(), box.width(), box.height());
                previewBox->setColor(QColor(0, 200, 255));
                m_view->cadScene()->setPreviewEntity(previewBox);
            }
        }
    }
}

void MoveTool::keyPress(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        cancel();
    }
}

void MoveTool::cancel() {
    m_state = State::WaitingForBasePoint;
    if (m_view && m_view->cadScene()) {
        m_view->cadScene()->clearPreview();
    }
}
