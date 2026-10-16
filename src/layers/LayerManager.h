#pragma once

#include <QObject>
#include <QStringList>
#include <QJsonArray>
#include <vector>
#include <memory>
#include "layers/CadLayer.h"

class LayerManager : public QObject {
    Q_OBJECT

public:
    explicit LayerManager(QObject* parent = nullptr);
    ~LayerManager() override;

    const std::vector<std::shared_ptr<CadLayer>>& layers() const { return m_layers; }
    QStringList layerNames() const;

    std::shared_ptr<CadLayer> findLayer(const QString& name) const;
    bool addLayer(const QString& name, const QColor& color = QColor(220, 220, 220), double lineWeight = 1.0);
    bool removeLayer(const QString& name);

    QString activeLayerName() const { return m_activeLayerName; }
    void setActiveLayer(const QString& name);
    std::shared_ptr<CadLayer> activeLayer() const;

    void setLayerVisible(const QString& name, bool visible);
    void setLayerLocked(const QString& name, bool locked);
    void setLayerColor(const QString& name, const QColor& color);

    void clear();

    QJsonArray toJson() const;
    bool fromJson(const QJsonArray& arr);

signals:
    void layerAdded(const QString& name);
    void layerRemoved(const QString& name);
    void layerUpdated(const QString& name);
    void activeLayerChanged(const QString& name);
    void layersReset();

private:
    std::vector<std::shared_ptr<CadLayer>> m_layers;
    QString m_activeLayerName{QStringLiteral("0")};
};
