#pragma once

#include <QGraphicsScene>
#include <memory>
#include "document/CadDocument.h"
#include "snap/SnapManager.h"
#include "ui/CadTheme.h"

class CadScene : public QGraphicsScene {
    Q_OBJECT

public:
    explicit CadScene(CadDocument* document, QObject* parent = nullptr);
    ~CadScene() override;

    CadDocument* document() const { return m_document; }

    // Dynamic interactive preview rendering for active tool
    void setPreviewEntity(std::shared_ptr<CadEntity> entity);
    void clearPreview();
    std::shared_ptr<CadEntity> previewEntity() const { return m_previewEntity; }

    // Multi-entity preview rendering (e.g. AI floor plan generator preview)
    void setPreviewEntities(const std::vector<std::shared_ptr<CadEntity>>& entities);
    void clearPreviewEntities();
    const std::vector<std::shared_ptr<CadEntity>>& previewEntities() const { return m_previewEntities; }

    // Visual settings
    void setGridVisible(bool visible);
    bool isGridVisible() const { return m_gridVisible; }

    void setGridSize(double size);
    double gridSize() const { return m_gridSize; }

    void setPageBorderVisible(bool visible);
    bool isPageBorderVisible() const { return m_pageBorderVisible; }

    // Snap marker display
    void setSnapResult(const SnapResult& snap);
    void clearSnapResult();
    const SnapResult& snapResult() const { return m_snapResult; }

    // Selection Box (AutoCAD Window vs Crossing)
    void setSelectionBox(const QPointF& start, const QPointF& end);
    void clearSelectionBox();

    // Dynamic HUD dimension overlay near cursor
    void setDynamicDimension(const QString& text, const QPointF& pos);
    void clearDynamicDimension();

    // Theme Mode
    void setThemeMode(CadTheme::ThemeMode mode);
    CadTheme::ThemeMode themeMode() const { return m_themeMode; }

protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void drawForeground(QPainter* painter, const QRectF& rect) override;

private slots:
    void onEntityAdded(std::shared_ptr<CadEntity> entity);
    void onEntityRemoved(const QUuid& id);
    void onEntityModified(std::shared_ptr<CadEntity> entity);
    void onDocumentCleared();
    void onSelectionChanged();

private:
    void renderEntity(QPainter* painter, const CadEntity* entity, bool isPreview = false);
    void renderDimension(QPainter* painter, const class DimensionEntity* dim, const QColor& strokeColor, double width, bool isPreview);
    void drawUcsIcon(QPainter* painter, double zoom);

    CadDocument* m_document{nullptr};
    std::shared_ptr<CadEntity> m_previewEntity;
    std::vector<std::shared_ptr<CadEntity>> m_previewEntities;
    SnapResult m_snapResult;
    bool m_gridVisible{true};
    double m_gridSize{10.0};
    bool m_pageBorderVisible{true};

    // Selection Box
    bool m_hasSelectionBox{false};
    QPointF m_selBoxStart;
    QPointF m_selBoxEnd;

    // Dynamic dimension HUD
    QString m_dynamicDimText;
    QPointF m_dynamicDimPos;
    bool m_hasDynamicDim{false};

    // Theme mode
    CadTheme::ThemeMode m_themeMode{CadTheme::ThemeMode::Dark};
};
