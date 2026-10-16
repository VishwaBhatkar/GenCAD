#include "layers/LayerManager.h"
#include <algorithm>

LayerManager::LayerManager(QObject* parent)
    : QObject(parent)
{
    clear();
}

LayerManager::~LayerManager() = default;

void LayerManager::clear() {
    m_layers.clear();
    // Default standard layer 0
    auto layer0 = std::make_shared<CadLayer>(QStringLiteral("0"), QColor(220, 220, 220), 1.0);
    m_layers.push_back(layer0);
    m_activeLayerName = QStringLiteral("0");
    emit layersReset();
}

QStringList LayerManager::layerNames() const {
    QStringList names;
    for (const auto& layer : m_layers) {
        names << layer->name();
    }
    return names;
}

std::shared_ptr<CadLayer> LayerManager::findLayer(const QString& name) const {
    for (const auto& layer : m_layers) {
        if (layer->name().compare(name, Qt::CaseInsensitive) == 0) {
            return layer;
        }
    }
    return nullptr;
}

bool LayerManager::addLayer(const QString& name, const QColor& color, double lineWeight) {
    if (name.trimmed().isEmpty()) return false;
    if (findLayer(name) != nullptr) return false;

    auto layer = std::make_shared<CadLayer>(name, color, lineWeight);
    m_layers.push_back(layer);
    emit layerAdded(name);
    return true;
}

bool LayerManager::removeLayer(const QString& name) {
    // Cannot delete layer "0"
    if (name.compare(QStringLiteral("0"), Qt::CaseInsensitive) == 0) {
        return false;
    }

    auto it = std::find_if(m_layers.begin(), m_layers.end(), [&name](const std::shared_ptr<CadLayer>& l) {
        return l->name().compare(name, Qt::CaseInsensitive) == 0;
    });

    if (it == m_layers.end()) return false;

    m_layers.erase(it);

    if (m_activeLayerName.compare(name, Qt::CaseInsensitive) == 0) {
        m_activeLayerName = QStringLiteral("0");
        emit activeLayerChanged(m_activeLayerName);
    }

    emit layerRemoved(name);
    return true;
}

void LayerManager::setActiveLayer(const QString& name) {
    if (m_activeLayerName != name && findLayer(name) != nullptr) {
        m_activeLayerName = name;
        emit activeLayerChanged(m_activeLayerName);
    }
}

std::shared_ptr<CadLayer> LayerManager::activeLayer() const {
    return findLayer(m_activeLayerName);
}

void LayerManager::setLayerVisible(const QString& name, bool visible) {
    auto layer = findLayer(name);
    if (layer && layer->isVisible() != visible) {
        layer->setVisible(visible);
        emit layerUpdated(name);
    }
}

void LayerManager::setLayerLocked(const QString& name, bool locked) {
    auto layer = findLayer(name);
    if (layer && layer->isLocked() != locked) {
        layer->setLocked(locked);
        emit layerUpdated(name);
    }
}

void LayerManager::setLayerColor(const QString& name, const QColor& color) {
    auto layer = findLayer(name);
    if (layer && layer->color() != color) {
        layer->setColor(color);
        emit layerUpdated(name);
    }
}

QJsonArray LayerManager::toJson() const {
    QJsonArray arr;
    for (const auto& layer : m_layers) {
        arr.append(layer->toJson());
    }
    return arr;
}

bool LayerManager::fromJson(const QJsonArray& arr) {
    m_layers.clear();
    for (const auto& val : arr) {
        if (val.isObject()) {
            auto layer = std::make_shared<CadLayer>();
            if (layer->fromJson(val.toObject())) {
                m_layers.push_back(layer);
            }
        }
    }
    if (m_layers.empty() || !findLayer(QStringLiteral("0"))) {
        auto layer0 = std::make_shared<CadLayer>(QStringLiteral("0"), QColor(220, 220, 220), 1.0);
        m_layers.insert(m_layers.begin(), layer0);
    }
    m_activeLayerName = QStringLiteral("0");
    emit layersReset();
    return true;
}
