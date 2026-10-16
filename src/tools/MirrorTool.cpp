#include "tools/MirrorTool.h"
#include "canvas/CadView.h"
#include "canvas/CadScene.h"
#include "document/CadDocument.h"
#include "geometry/LineEntity.h"
#include "commands/MirrorEntityCommand.h"

#include <cmath>

MirrorTool::MirrorTool(CadDocument* doc, CadView* view)
    : Tool(doc, view)
{
}

void MirrorTool::activate() {
    m_state = State::WaitingForAxisP1;
    if (m_document) {
        m_targetIds = m_document->selectedIds();
    }
    if (m_view) {
        m_view->setCursor(Qt::CrossCursor);
    }
}

void MirrorTool::deactivate() {
    cancel();
}

void MirrorTool::mousePress(QMouseEvent* event, const QPointF& worldPos) {
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

    if (m_state == State::WaitingForAxisP1) {
        m_p1 = worldPos;
        m_state = State::WaitingForAxisP2;
    } else if (m_state == State::WaitingForAxisP2) {
        if (std::hypot(worldPos.x() - m_p1.x(), worldPos.y() - m_p1.y()) > 1e-4) {
            m_document->undoStack()->push(new MirrorEntityCommand(m_document, m_targetIds, m_p1, worldPos, true));
        }
        cancel();
    }
}

void MirrorTool::mouseMove(QMouseEvent* event, const QPointF& worldPos) {
    Q_UNUSED(event);
    if (!m_view || !m_view->cadScene()) return;

    if (m_state == State::WaitingForAxisP2) {
        auto axisLine = std::make_shared<LineEntity>(m_p1, worldPos);
        m_view->cadScene()->setPreviewEntity(axisLine);
    }
}

void MirrorTool::keyPress(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        cancel();
    }
}

void MirrorTool::cancel() {
    m_state = State::WaitingForAxisP1;
    m_targetIds.clear();
    if (m_view && m_view->cadScene()) {
        m_view->cadScene()->clearPreview();
    }
}
