#include "tools/TextTool.h"
#include "canvas/CadView.h"
#include "document/CadDocument.h"
#include "geometry/TextEntity.h"
#include "commands/CreateEntityCommand.h"

#include <QInputDialog>

TextTool::TextTool(CadDocument* doc, CadView* view)
    : Tool(doc, view)
{
}

void TextTool::activate() {
    if (m_view) {
        m_view->setCursor(Qt::IBeamCursor);
    }
}

void TextTool::deactivate() {
    if (m_view) {
        m_view->setCursor(Qt::ArrowCursor);
    }
}

void TextTool::mousePress(QMouseEvent* event, const QPointF& worldPos) {
    if (!m_document || !m_view) return;

    if (event->button() != Qt::LeftButton) return;

    bool ok = false;
    QString text = QInputDialog::getText(m_view,
                                         QStringLiteral("Place Text"),
                                         QStringLiteral("Enter text content:"),
                                         QLineEdit::Normal,
                                         QStringLiteral("GenCAD"),
                                         &ok);

    if (ok && !text.isEmpty()) {
        auto entity = std::make_shared<TextEntity>(worldPos, text, 10.0, 0.0);
        m_document->undoStack()->push(new CreateEntityCommand(m_document, entity));
    }
}

void TextTool::keyPress(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        deactivate();
    }
}
