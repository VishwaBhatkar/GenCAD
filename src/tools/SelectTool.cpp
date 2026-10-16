#include "tools/SelectTool.h"
#include "canvas/CadView.h"
#include "canvas/CadScene.h"
#include "document/CadDocument.h"
#include "commands/DeleteEntityCommand.h"

#include <algorithm>
#include <cmath>

SelectTool::SelectTool(CadDocument* doc, CadView* view)
    : Tool(doc, view)
{
}

void SelectTool::activate() {
    if (m_view) {
        m_view->setCursor(Qt::ArrowCursor);
    }
}

void SelectTool::deactivate() {
    m_isBoxSelecting = false;
    if (m_view && m_view->cadScene()) {
        m_view->cadScene()->clearSelectionBox();
    }
}

void SelectTool::mousePress(QMouseEvent* event, const QPointF& worldPos) {
    if (event->button() != Qt::LeftButton || !m_document || !m_view) return;

    bool multiSelectModifier = (event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier));

    // Calculate tolerance based on zoom level: ~6 screen pixels
    double scale = m_view->transform().m11();
    if (scale <= 0) scale = 1.0;
    double tolerance = 6.0 / scale;

    // Hit test existing entities (in reverse order to select topmost first)
    const auto& entities = m_document->entities();
    std::shared_ptr<CadEntity> hitEntity = nullptr;

    for (auto it = entities.rbegin(); it != entities.rend(); ++it) {
        if ((*it)->hitTest(worldPos, tolerance)) {
            hitEntity = *it;
            break;
        }
    }

    if (hitEntity) {
        if (multiSelectModifier) {
            if (hitEntity->isSelected()) {
                m_document->deselect(hitEntity->id());
            } else {
                m_document->select(hitEntity->id(), true);
            }
        } else {
            m_document->select(hitEntity->id(), false);
        }
    } else {
        // Clicked empty space
        if (!multiSelectModifier) {
            m_document->clearSelection();
        }
        // Start drag selection box
        m_isBoxSelecting = true;
        m_boxStart = worldPos;
        m_boxEnd = worldPos;
    }
}

void SelectTool::mouseMove(QMouseEvent* event, const QPointF& worldPos) {
    Q_UNUSED(event);
    if (!m_isBoxSelecting || !m_view || !m_view->cadScene()) return;

    m_boxEnd = worldPos;
    m_view->cadScene()->setSelectionBox(m_boxStart, m_boxEnd);
}

void SelectTool::mouseRelease(QMouseEvent* event, const QPointF& worldPos) {
    if (event->button() != Qt::LeftButton || !m_isBoxSelecting || !m_document || !m_view) return;

    m_isBoxSelecting = false;
    m_boxEnd = worldPos;

    if (m_view->cadScene()) {
        m_view->cadScene()->clearSelectionBox();
    }

    qreal x1 = std::min(m_boxStart.x(), m_boxEnd.x());
    qreal y1 = std::min(m_boxStart.y(), m_boxEnd.y());
    qreal x2 = std::max(m_boxStart.x(), m_boxEnd.x());
    qreal y2 = std::max(m_boxStart.y(), m_boxEnd.y());
    QRectF box(QPointF(x1, y1), QPointF(x2, y2));

    bool multiSelectModifier = (event->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier));

    // Check if the user made a drag box of significant size
    double scale = m_view->transform().m11();
    if (scale <= 0) scale = 1.0;
    double minBoxSize = 4.0 / scale;

    if (box.width() > minBoxSize && box.height() > minBoxSize) {
        if (!multiSelectModifier) {
            m_document->clearSelection();
        }

        bool isWindowSelection = (m_boxStart.x() <= m_boxEnd.x());

        for (const auto& entity : m_document->entities()) {
            if (!entity) continue;

            if (isWindowSelection) {
                // Window selection (left to right): must be completely inside box
                if (box.contains(entity->boundingRect())) {
                    m_document->select(entity->id(), true);
                }
            } else {
                // Crossing selection (right to left): touches or inside box
                if (box.intersects(entity->boundingRect())) {
                    m_document->select(entity->id(), true);
                }
            }
        }
    }
}

void SelectTool::keyPress(QKeyEvent* event) {
    if (!m_document) return;

    if (event->key() == Qt::Key_Delete) {
        auto selected = m_document->selectedEntities();
        if (!selected.empty()) {
            m_document->undoStack()->push(new DeleteEntityCommand(m_document, selected));
        }
    } else if (event->key() == Qt::Key_Escape) {
        if (m_isBoxSelecting) {
            m_isBoxSelecting = false;
            if (m_view && m_view->cadScene()) {
                m_view->cadScene()->clearSelectionBox();
            }
        } else {
            m_document->clearSelection();
        }
    }
}
