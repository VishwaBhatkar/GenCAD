#include "tools/RotateTool.h"
#include "canvas/CadView.h"
#include "canvas/CadScene.h"
#include "document/CadDocument.h"
#include "geometry/LineEntity.h"
#include "commands/RotateEntityCommand.h"

#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

RotateTool::RotateTool(CadDocument* doc, CadView* view)
    : Tool(doc, view)
{
}

void RotateTool::activate() {
    m_state = State::WaitingForBasePoint;
    if (m_document) {
        m_targetIds = m_document->selectedIds();
    }
    if (m_view) {
        m_view->setCursor(Qt::CrossCursor);
    }
}

void RotateTool::deactivate() {
    cancel();
}

void RotateTool::mousePress(QMouseEvent* event, const QPointF& worldPos) {
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
        // Try to pick an entity directly under cursor
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
        m_state = State::WaitingForAngle;
    } else if (m_state == State::WaitingForAngle) {
        double dx = worldPos.x() - m_basePoint.x();
        double dy = worldPos.y() - m_basePoint.y();
        if (std::hypot(dx, dy) > 1e-4) {
            double angleRad = std::atan2(dy, dx);
            m_document->undoStack()->push(new RotateEntityCommand(m_document, m_targetIds, m_basePoint, angleRad));
        }
        cancel();
    }
}

void RotateTool::mouseMove(QMouseEvent* event, const QPointF& worldPos) {
    Q_UNUSED(event);
    if (!m_view || !m_view->cadScene()) return;

    if (m_state == State::WaitingForAngle) {
        auto previewLine = std::make_shared<LineEntity>(m_basePoint, worldPos);
        m_view->cadScene()->setPreviewEntity(previewLine);
    }
}

void RotateTool::keyPress(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        cancel();
    }
}

void RotateTool::cancel() {
    m_state = State::WaitingForBasePoint;
    m_targetIds.clear();
    if (m_view && m_view->cadScene()) {
        m_view->cadScene()->clearPreview();
    }
}
