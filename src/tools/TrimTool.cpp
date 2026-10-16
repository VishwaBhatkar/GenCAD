#include "tools/TrimTool.h"
#include "canvas/CadView.h"
#include "document/CadDocument.h"
#include "commands/DeleteEntityCommand.h"

TrimTool::TrimTool(CadDocument* doc, CadView* view)
    : Tool(doc, view)
{
}

void TrimTool::activate() {
    if (m_view) {
        m_view->setCursor(Qt::CrossCursor);
    }
}

void TrimTool::deactivate() {
    if (m_view) {
        m_view->setCursor(Qt::ArrowCursor);
    }
}

void TrimTool::mousePress(QMouseEvent* event, const QPointF& worldPos) {
    if (!m_document || !m_view) return;

    if (event->button() != Qt::LeftButton) return;

    const double tolerance = 8.0;
    for (const auto& entity : m_document->entities()) {
        if (entity && entity->hitTest(worldPos, tolerance)) {
            m_document->undoStack()->push(new DeleteEntityCommand(m_document, entity));
            break;
        }
    }
}

void TrimTool::keyPress(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        deactivate();
    }
}
