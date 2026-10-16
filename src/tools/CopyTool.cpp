#include "tools/CopyTool.h"
#include "canvas/CadView.h"
#include "canvas/CadScene.h"
#include "document/CadDocument.h"
#include "geometry/LineEntity.h"
#include "commands/CopyEntityCommand.h"

#include <cmath>

CopyTool::CopyTool(CadDocument* doc, CadView* view)
    : Tool(doc, view)
{
}

void CopyTool::activate() {
    m_state = State::WaitingForBasePoint;
    if (m_document) {
        m_targetIds = m_document->selectedIds();
    }
    if (m_view) {
        m_view->setCursor(Qt::CrossCursor);
    }
}

void CopyTool::deactivate() {
    cancel();
}

void CopyTool::mousePress(QMouseEvent* event, const QPointF& worldPos) {
    if (!m_document || !m_view) return;

    if (event->button() == Qt::RightButton) {
        cancel();
        return;
    }

    if (event->button() != Qt::LeftButton) return;

    if (m_targetIds.empty()) {
        m_targetIds = m_document->selectedIds();
    }

    if (m_targetIds.empty()) {
        const double tolerance = 6.0;
        for (const auto& entity : m_document->entities()) {
            if (entity && entity->hitTest(worldPos, tolerance)) {
                m_document->select(entity->id(), false);
                m_targetIds.push_back(entity->id());
                break;
            }
        }
        if (m_targetIds.empty()) return;
    }

    if (m_state == State::WaitingForBasePoint) {
        m_basePoint = worldPos;
        m_state = State::WaitingForDestPoint;
    } else if (m_state == State::WaitingForDestPoint) {
        double dx = worldPos.x() - m_basePoint.x();
        double dy = worldPos.y() - m_basePoint.y();
        if (std::hypot(dx, dy) > 1e-4) {
            m_document->undoStack()->push(new CopyEntityCommand(m_document, m_targetIds, dx, dy));
        }
        cancel();
    }
}

void CopyTool::mouseMove(QMouseEvent* event, const QPointF& worldPos) {
    Q_UNUSED(event);
    if (!m_view || !m_view->cadScene()) return;

    if (m_state == State::WaitingForDestPoint) {
        auto displacementLine = std::make_shared<LineEntity>(m_basePoint, worldPos);
        m_view->cadScene()->setPreviewEntity(displacementLine);
    }
}

void CopyTool::keyPress(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        cancel();
    }
}

void CopyTool::cancel() {
    m_state = State::WaitingForBasePoint;
    m_targetIds.clear();
    if (m_view && m_view->cadScene()) {
        m_view->cadScene()->clearPreview();
    }
}
