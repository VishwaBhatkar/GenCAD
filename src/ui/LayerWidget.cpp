#include "ui/LayerWidget.h"
#include "document/CadDocument.h"
#include "layers/LayerManager.h"
#include "layers/CadLayer.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLineEdit>
#include <QColorDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
#include <QIcon>
#include <QPixmap>
#include <QPainter>

LayerWidget::LayerWidget(CadDocument* document, QWidget* parent)
    : QWidget(parent)
    , m_document(document)
{
    setupUi();
    setDocument(document);
}

void LayerWidget::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    mainLayout->setSpacing(4);

    // 1. Search Filter Bar
    m_filterEdit = new QLineEdit(this);
    m_filterEdit->setPlaceholderText(tr("Search layers..."));
    m_filterEdit->setClearButtonEnabled(true);
    connect(m_filterEdit, &QLineEdit::textChanged, this, &LayerWidget::onFilterTextChanged);
    mainLayout->addWidget(m_filterEdit);

    // 2. Action Toolbar buttons
    auto* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(3);

    m_btnAdd = new QPushButton(QIcon(QStringLiteral(":/icons/new.svg")), tr("New"), this);
    m_btnAdd->setToolTip(tr("Create a new layer"));

    m_btnDelete = new QPushButton(QIcon(QStringLiteral(":/icons/delete.svg")), tr("Delete"), this);
    m_btnDelete->setToolTip(tr("Delete selected layer"));

    m_btnSetActive = new QPushButton(QIcon(QStringLiteral(":/icons/check.svg")), tr("Current"), this);
    m_btnSetActive->setToolTip(tr("Set selected layer as current active"));

    m_btnTurnAllOn = new QPushButton(QIcon(QStringLiteral(":/icons/bulb_on.svg")), QString(), this);
    m_btnTurnAllOn->setToolTip(tr("Turn all layers ON"));
    m_btnTurnAllOn->setFixedWidth(28);

    m_btnTurnAllOff = new QPushButton(QIcon(QStringLiteral(":/icons/bulb_off.svg")), QString(), this);
    m_btnTurnAllOff->setToolTip(tr("Turn all layers OFF"));
    m_btnTurnAllOff->setFixedWidth(28);

    btnLayout->addWidget(m_btnAdd);
    btnLayout->addWidget(m_btnDelete);
    btnLayout->addWidget(m_btnSetActive);
    btnLayout->addWidget(m_btnTurnAllOn);
    btnLayout->addWidget(m_btnTurnAllOff);
    mainLayout->addLayout(btnLayout);

    // 3. Layer Table
    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({
        tr("Current"),
        tr("Name"),
        tr("Color"),
        tr("Visible"),
        tr("Lock"),
        tr("Objects")
    });

    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(5, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->setVisible(false);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);

    mainLayout->addWidget(m_table);

    connect(m_btnAdd, &QPushButton::clicked, this, &LayerWidget::onAddLayerClicked);
    connect(m_btnDelete, &QPushButton::clicked, this, &LayerWidget::onDeleteLayerClicked);
    connect(m_btnSetActive, &QPushButton::clicked, this, &LayerWidget::onSetActiveClicked);
    connect(m_btnTurnAllOn, &QPushButton::clicked, this, &LayerWidget::onTurnAllOnClicked);
    connect(m_btnTurnAllOff, &QPushButton::clicked, this, &LayerWidget::onTurnAllOffClicked);
    connect(m_table, &QTableWidget::cellClicked, this, &LayerWidget::onCellClicked);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, &LayerWidget::onCellDoubleClicked);
    connect(m_table, &QTableWidget::customContextMenuRequested, this, &LayerWidget::onContextMenuRequested);
}

void LayerWidget::setDocument(CadDocument* document) {
    m_document = document;
    if (m_document && m_document->layerManager()) {
        connect(m_document->layerManager(), &LayerManager::layerAdded, this, &LayerWidget::refresh);
        connect(m_document->layerManager(), &LayerManager::layerRemoved, this, &LayerWidget::refresh);
        connect(m_document->layerManager(), &LayerManager::layerUpdated, this, &LayerWidget::refresh);
        connect(m_document->layerManager(), &LayerManager::activeLayerChanged, this, &LayerWidget::refresh);
        connect(m_document->layerManager(), &LayerManager::layersReset, this, &LayerWidget::refresh);
        connect(m_document, &CadDocument::entityAdded, this, &LayerWidget::refresh);
        connect(m_document, &CadDocument::entityRemoved, this, &LayerWidget::refresh);
        connect(m_document, &CadDocument::documentCleared, this, &LayerWidget::refresh);
    }
    refresh();
}

int LayerWidget::countEntitiesOnLayer(const QString& layerName) const {
    if (!m_document) return 0;
    int count = 0;
    for (const auto& ent : m_document->entities()) {
        if (ent && ent->layer() == layerName) {
            count++;
        }
    }
    return count;
}

void LayerWidget::refresh() {
    if (!m_document || !m_document->layerManager() || !m_table) return;

    m_updatingTable = true;
    m_table->setRowCount(0);

    const auto& layers = m_document->layerManager()->layers();
    QString activeName = m_document->layerManager()->activeLayerName();
    QString filter = m_filterEdit ? m_filterEdit->text().trimmed().toLower() : QString();

    for (const auto& layer : layers) {
        if (!filter.isEmpty() && !layer->name().toLower().contains(filter)) {
            continue;
        }

        int row = m_table->rowCount();
        m_table->insertRow(row);
        bool isActive = (layer->name() == activeName);

        // Column 0: Active status
        auto* activeItem = new QTableWidgetItem();
        if (isActive) {
            activeItem->setIcon(QIcon(QStringLiteral(":/icons/check.svg")));
        }
        activeItem->setTextAlignment(Qt::AlignCenter);
        activeItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_table->setItem(row, 0, activeItem);

        // Column 1: Name
        auto* nameItem = new QTableWidgetItem(layer->name());
        if (isActive) {
            QFont f = nameItem->font();
            f.setBold(true);
            nameItem->setFont(f);
            nameItem->setForeground(QColor(0, 229, 255));
        }
        nameItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_table->setItem(row, 1, nameItem);

        // Column 2: Color swatch
        auto* colorItem = new QTableWidgetItem();
        QPixmap pix(16, 16);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        p.setBrush(layer->color());
        p.setPen(QPen(Qt::white, 1));
        p.drawRoundedRect(1, 1, 14, 14, 3, 3);
        p.end();
        colorItem->setIcon(QIcon(pix));
        colorItem->setTextAlignment(Qt::AlignCenter);
        colorItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        colorItem->setToolTip(layer->color().name().toUpper());
        m_table->setItem(row, 2, colorItem);

        // Column 3: Visible toggle
        auto* visItem = new QTableWidgetItem();
        visItem->setIcon(QIcon(layer->isVisible() ? QStringLiteral(":/icons/bulb_on.svg") : QStringLiteral(":/icons/bulb_off.svg")));
        visItem->setTextAlignment(Qt::AlignCenter);
        visItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        visItem->setToolTip(layer->isVisible() ? tr("Layer is Visible (Click to turn off)") : tr("Layer is Hidden (Click to turn on)"));
        m_table->setItem(row, 3, visItem);

        // Column 4: Lock toggle
        auto* lockItem = new QTableWidgetItem();
        lockItem->setIcon(QIcon(layer->isLocked() ? QStringLiteral(":/icons/lock.svg") : QStringLiteral(":/icons/unlock.svg")));
        lockItem->setTextAlignment(Qt::AlignCenter);
        lockItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        lockItem->setToolTip(layer->isLocked() ? tr("Layer is Locked") : tr("Layer is Unlocked"));
        m_table->setItem(row, 4, lockItem);

        // Column 5: Entities count
        int entCount = countEntitiesOnLayer(layer->name());
        auto* countItem = new QTableWidgetItem(QString::number(entCount));
        countItem->setTextAlignment(Qt::AlignCenter);
        countItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        m_table->setItem(row, 5, countItem);
    }

    m_updatingTable = false;
}

void LayerWidget::showNewLayerDialog() {
    onAddLayerClicked();
}

void LayerWidget::onAddLayerClicked() {
    if (!m_document || !m_document->layerManager()) return;

    bool ok = false;
    QString name = QInputDialog::getText(this,
                                         tr("New CAD Layer"),
                                         tr("Layer name:"),
                                         QLineEdit::Normal,
                                         QStringLiteral("Layer_%1").arg(m_document->layerManager()->layers().size()),
                                         &ok);

    if (ok && !name.trimmed().isEmpty()) {
        m_document->layerManager()->addLayer(name.trimmed());
    }
}

void LayerWidget::onDeleteLayerClicked() {
    if (!m_document || !m_document->layerManager()) return;

    int row = m_table->currentRow();
    if (row < 0 || row >= m_table->rowCount()) return;

    QString layerName = m_table->item(row, 1)->text();
    if (layerName == QStringLiteral("0")) {
        QMessageBox::warning(this, tr("Cannot Delete Layer"), tr("The default '0' layer cannot be deleted."));
        return;
    }

    if (layerName == m_document->layerManager()->activeLayerName()) {
        QMessageBox::warning(this, tr("Cannot Delete Active Layer"), tr("Cannot delete current active layer. Set another layer active first."));
        return;
    }

    int count = countEntitiesOnLayer(layerName);
    if (count > 0) {
        auto res = QMessageBox::question(this, tr("Layer Contains Entities"),
            tr("Layer '%1' contains %2 entities.\nAre you sure you want to delete it?").arg(layerName).arg(count));
        if (res != QMessageBox::Yes) return;
    }

    m_document->layerManager()->removeLayer(layerName);
}

void LayerWidget::onSetActiveClicked() {
    if (!m_document || !m_document->layerManager()) return;

    int row = m_table->currentRow();
    if (row >= 0 && row < m_table->rowCount()) {
        QString layerName = m_table->item(row, 1)->text();
        m_document->layerManager()->setActiveLayer(layerName);
    }
}

void LayerWidget::onTurnAllOnClicked() {
    if (!m_document || !m_document->layerManager()) return;
    for (const auto& layer : m_document->layerManager()->layers()) {
        m_document->layerManager()->setLayerVisible(layer->name(), true);
    }
}

void LayerWidget::onTurnAllOffClicked() {
    if (!m_document || !m_document->layerManager()) return;
    QString active = m_document->layerManager()->activeLayerName();
    for (const auto& layer : m_document->layerManager()->layers()) {
        if (layer->name() != active) {
            m_document->layerManager()->setLayerVisible(layer->name(), false);
        }
    }
}

void LayerWidget::onFilterTextChanged(const QString& /*text*/) {
    refresh();
}

void LayerWidget::onCellClicked(int row, int column) {
    if (m_updatingTable || !m_document || !m_document->layerManager()) return;
    if (row < 0 || row >= m_table->rowCount()) return;

    QString layerName = m_table->item(row, 1)->text();
    auto layer = m_document->layerManager()->findLayer(layerName);
    if (!layer) return;

    if (column == 0) {
        // Set current
        m_document->layerManager()->setActiveLayer(layerName);
    } else if (column == 2) {
        // Change color
        QColor chosen = QColorDialog::getColor(layer->color(), this, tr("Select Layer Color"));
        if (chosen.isValid()) {
            m_document->layerManager()->setLayerColor(layerName, chosen);
        }
    } else if (column == 3) {
        // Toggle visibility
        m_document->layerManager()->setLayerVisible(layerName, !layer->isVisible());
    } else if (column == 4) {
        // Toggle lock
        m_document->layerManager()->setLayerLocked(layerName, !layer->isLocked());
    }
}

void LayerWidget::onCellDoubleClicked(int row, int column) {
    if (row < 0 || row >= m_table->rowCount()) return;
    QString layerName = m_table->item(row, 1)->text();

    if (column == 1) {
        // Rename layer
        if (layerName == QStringLiteral("0")) return;
        bool ok = false;
        QString newName = QInputDialog::getText(this, tr("Rename Layer"), tr("New name:"), QLineEdit::Normal, layerName, &ok);
        if (ok && !newName.trimmed().isEmpty() && newName != layerName) {
            auto layer = m_document->layerManager()->findLayer(layerName);
            if (layer) {
                layer->setName(newName.trimmed());
                refresh();
            }
        }
    } else {
        onCellClicked(row, column);
    }
}

void LayerWidget::onContextMenuRequested(const QPoint& pos) {
    int row = m_table->rowAt(pos.y());
    if (row < 0 || row >= m_table->rowCount()) return;

    QString layerName = m_table->item(row, 1)->text();
    auto layer = m_document->layerManager()->findLayer(layerName);
    if (!layer) return;

    QMenu menu(this);
    menu.addAction(QIcon(QStringLiteral(":/icons/check.svg")), tr("Set Current"), this, [this, layerName]() {
        m_document->layerManager()->setActiveLayer(layerName);
    });

    menu.addSeparator();
    menu.addAction(tr("Toggle Visibility"), this, [this, layer, layerName]() {
        m_document->layerManager()->setLayerVisible(layerName, !layer->isVisible());
    });

    menu.addAction(tr("Toggle Lock"), this, [this, layer, layerName]() {
        m_document->layerManager()->setLayerLocked(layerName, !layer->isLocked());
    });

    menu.addAction(tr("Change Color..."), this, [this, layer, layerName]() {
        QColor chosen = QColorDialog::getColor(layer->color(), this, tr("Select Layer Color"));
        if (chosen.isValid()) {
            m_document->layerManager()->setLayerColor(layerName, chosen);
        }
    });

    menu.addSeparator();
    menu.addAction(tr("Select All Objects on Layer"), this, [this, layerName]() {
        if (!m_document) return;
        m_document->clearSelection();
        for (const auto& ent : m_document->entities()) {
            if (ent && ent->layer() == layerName) {
                m_document->select(ent->id(), true);
            }
        }
    });

    if (layerName != QStringLiteral("0")) {
        menu.addSeparator();
        menu.addAction(QIcon(QStringLiteral(":/icons/delete.svg")), tr("Delete Layer"), this, &LayerWidget::onDeleteLayerClicked);
    }

    menu.exec(m_table->viewport()->mapToGlobal(pos));
}
