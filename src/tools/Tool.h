#pragma once

#include <QPointF>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QString>

class CadDocument;
class CadView;
class CadScene;

class Tool {
public:
    Tool(CadDocument* doc, CadView* view)
        : m_document(doc)
        , m_view(view)
    {
    }
    virtual ~Tool() = default;

    virtual void activate() {}
    virtual void deactivate() {}

    virtual void mousePress(QMouseEvent* event, const QPointF& worldPos) {
        Q_UNUSED(event);
        Q_UNUSED(worldPos);
    }

    virtual void mouseMove(QMouseEvent* event, const QPointF& worldPos) {
        Q_UNUSED(event);
        Q_UNUSED(worldPos);
    }

    virtual void mouseRelease(QMouseEvent* event, const QPointF& worldPos) {
        Q_UNUSED(event);
        Q_UNUSED(worldPos);
    }

    virtual void keyPress(QKeyEvent* event) {
        Q_UNUSED(event);
    }

    virtual QString name() const = 0;
    virtual bool isDirectionalOrthoConstrained() const { return false; }

protected:
    CadDocument* m_document{nullptr};
    CadView* m_view{nullptr};
};
