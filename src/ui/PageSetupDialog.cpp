#include "ui/PageSetupDialog.h"
#include "document/CadDocument.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QButtonGroup>
#include <QLineEdit>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <cmath>
#include <algorithm>

PagePreviewWidget::PagePreviewWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(220, 180);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void PagePreviewWidget::setPageSettings(const PageSettings& settings) {
    m_settings = settings;
    update();
}

void PagePreviewWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Canvas background
    painter.fillRect(rect(), QColor(22, 24, 28));

    double pw = m_settings.widthMm();
    double ph = m_settings.heightMm();
    if (pw <= 0 || ph <= 0) return;

    // Compute fit scaling within widget
    double margin = 24.0;
    double availW = width() - 2.0 * margin;
    double availH = height() - 2.0 * margin - 20.0;
    double scale = std::min(availW / pw, availH / ph);

    double drawW = pw * scale;
    double drawH = ph * scale;
    double startX = (width() - drawW) * 0.5;
    double startY = (height() - 20.0 - drawH) * 0.5;

    QRectF sheetRect(startX, startY, drawW, drawH);

    // Drop shadow
    painter.fillRect(sheetRect.translated(4, 4), QColor(8, 10, 14, 180));

    // Sheet background
    painter.fillRect(sheetRect, QColor(36, 40, 48));

    // Printable limits box
    double minX = m_settings.minX();
    double minY = m_settings.minY();
    double maxX = m_settings.maxX();
    double maxY = m_settings.maxY();
    QRectF limitsRect(startX + minX * scale, startY + minY * scale,
                      (maxX - minX) * scale, (maxY - minY) * scale);

    QPen limitsPen(QColor(0, 180, 255, 160), 1.0, Qt::DashLine);
    painter.setPen(limitsPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(limitsRect);

    // Sheet border
    QPen sheetPen(QColor(160, 170, 190), 1.5, Qt::SolidLine);
    painter.setPen(sheetPen);
    painter.drawRect(sheetRect);

    // Dimensions text at bottom
    painter.setPen(QColor(200, 210, 230));
    QFont font = painter.font();
    font.setPointSize(9);
    font.setBold(true);
    painter.setFont(font);

    QString desc = QStringLiteral("%1 %2 (%3 × %4 mm)")
        .arg(m_settings.pageSizeName())
        .arg(m_settings.orientation() == PageOrientation::Portrait ? tr("Portrait") : tr("Landscape"))
        .arg(pw, 0, 'f', 0)
        .arg(ph, 0, 'f', 0);
    painter.drawText(QRectF(0, height() - 22, width(), 20), Qt::AlignCenter, desc);
}

// ============================================================================
// PageSetupDialog
// ============================================================================

PageSetupDialog::PageSetupDialog(const PageSettings& initialPage, const UnitSettings& initialUnits, QWidget* parent)
    : QDialog(parent)
    , m_pageSettings(initialPage)
    , m_unitSettings(initialUnits)
{
    setWindowTitle(tr("New Drawing Setup"));
    setModal(true);
    resize(640, 520);
    setupUi();
}

PageSetupDialog::PageSetupDialog(CadDocument* doc, QWidget* parent)
    : PageSetupDialog(doc ? doc->pageSettings() : PageSettings(),
                      doc ? doc->unitSettings() : UnitSettings(),
                      parent)
{
}

void PageSetupDialog::applySettings(CadDocument* doc) const {
    if (!doc) return;
    doc->setPageSettings(m_pageSettings);
    doc->setUnitSettings(m_unitSettings);
}

void PageSetupDialog::setupUi() {
    setStyleSheet(QStringLiteral(
        "QDialog { background-color: #2b2d30; color: #f0f0f0; }"
        "QGroupBox { font-weight: bold; border: 1px solid #43454a; border-radius: 4px; margin-top: 10px; padding-top: 10px; color: #00e5ff; }"
        "QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 10px; padding: 0 4px; }"
        "QLabel { color: #dcdcdc; }"
        "QComboBox, QDoubleSpinBox, QSpinBox, QLineEdit { background-color: #1e1f22; color: #ffffff; border: 1px solid #4e5157; border-radius: 3px; padding: 4px 8px; }"
        "QComboBox:focus, QDoubleSpinBox:focus, QSpinBox:focus, QLineEdit:focus { border: 1px solid #007acc; }"
        "QPushButton { background-color: #383a40; color: #ffffff; border: 1px solid #55575e; border-radius: 3px; padding: 6px 14px; min-width: 80px; }"
        "QPushButton:hover { background-color: #484b52; border-color: #007acc; }"
        "QPushButton:default { background-color: #007acc; border-color: #0098ff; }"
        "QRadioButton { color: #dcdcdc; }"
    ));

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(createPageLayoutStep());
    m_stack->addWidget(createUnitsStep());
    mainLayout->addWidget(m_stack, 1);

    updatePagePreview();
    updateUnitPreview();
}

QWidget* PageSetupDialog::createPageLayoutStep() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    auto* header = new QLabel(tr("<h3>1. Page / Drawing Layout</h3><p style='color:#aaa'>Select the standard sheet size, orientation, and CAD drawing limits.</p>"), page);
    layout->addWidget(header);

    auto* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(14);

    // Left controls
    auto* leftWidget = new QWidget(page);
    auto* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);

    // Page size group
    auto* sizeGroup = new QGroupBox(tr("Paper Format"), leftWidget);
    auto* sizeLayout = new QGridLayout(sizeGroup);

    sizeLayout->addWidget(new QLabel(tr("Page Size:"), sizeGroup), 0, 0);
    m_comboPageSize = new QComboBox(sizeGroup);
    m_comboPageSize->addItems({QStringLiteral("A0 (841 × 1189 mm)"),
                              QStringLiteral("A1 (594 × 841 mm)"),
                              QStringLiteral("A2 (420 × 594 mm)"),
                              QStringLiteral("A3 (297 × 420 mm)"),
                              QStringLiteral("A4 (210 × 297 mm)"),
                              QStringLiteral("A5 (148 × 210 mm)"),
                              QStringLiteral("Custom")});
    m_comboPageSize->setCurrentIndex(static_cast<int>(m_pageSettings.pageSize()));
    connect(m_comboPageSize, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PageSetupDialog::onPageSizeChanged);
    sizeLayout->addWidget(m_comboPageSize, 0, 1);

    sizeLayout->addWidget(new QLabel(tr("Orientation:"), sizeGroup), 1, 0);
    auto* orientLayout = new QHBoxLayout();
    m_radioPortrait = new QRadioButton(tr("Portrait"), sizeGroup);
    m_radioLandscape = new QRadioButton(tr("Landscape"), sizeGroup);
    if (m_pageSettings.orientation() == PageOrientation::Portrait) {
        m_radioPortrait->setChecked(true);
    } else {
        m_radioLandscape->setChecked(true);
    }
    connect(m_radioPortrait, &QRadioButton::toggled, this, &PageSetupDialog::onOrientationChanged);
    connect(m_radioLandscape, &QRadioButton::toggled, this, &PageSetupDialog::onOrientationChanged);
    orientLayout->addWidget(m_radioLandscape);
    orientLayout->addWidget(m_radioPortrait);
    sizeLayout->addLayout(orientLayout, 1, 1);

    sizeLayout->addWidget(new QLabel(tr("Custom Width:"), sizeGroup), 2, 0);
    m_spinCustomW = new QDoubleSpinBox(sizeGroup);
    m_spinCustomW->setRange(10.0, 50000.0);
    m_spinCustomW->setSuffix(QStringLiteral(" mm"));
    m_spinCustomW->setValue(m_pageSettings.customWidth());
    m_spinCustomW->setEnabled(m_pageSettings.pageSize() == PageSize::Custom);
    connect(m_spinCustomW, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PageSetupDialog::onCustomDimensionsChanged);
    sizeLayout->addWidget(m_spinCustomW, 2, 1);

    sizeLayout->addWidget(new QLabel(tr("Custom Height:"), sizeGroup), 3, 0);
    m_spinCustomH = new QDoubleSpinBox(sizeGroup);
    m_spinCustomH->setRange(10.0, 50000.0);
    m_spinCustomH->setSuffix(QStringLiteral(" mm"));
    m_spinCustomH->setValue(m_pageSettings.customHeight());
    m_spinCustomH->setEnabled(m_pageSettings.pageSize() == PageSize::Custom);
    connect(m_spinCustomH, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PageSetupDialog::onCustomDimensionsChanged);
    sizeLayout->addWidget(m_spinCustomH, 3, 1);

    leftLayout->addWidget(sizeGroup);

    // Drawing Limits group
    auto* limitsGroup = new QGroupBox(tr("Drawing Area / Limits"), leftWidget);
    auto* limitsLayout = new QGridLayout(limitsGroup);

    limitsLayout->addWidget(new QLabel(tr("Min X:"), limitsGroup), 0, 0);
    m_spinMinX = new QDoubleSpinBox(limitsGroup);
    m_spinMinX->setRange(-100000.0, 100000.0);
    m_spinMinX->setValue(m_pageSettings.minX());
    connect(m_spinMinX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PageSetupDialog::onLimitsChanged);
    limitsLayout->addWidget(m_spinMinX, 0, 1);

    limitsLayout->addWidget(new QLabel(tr("Min Y:"), limitsGroup), 0, 2);
    m_spinMinY = new QDoubleSpinBox(limitsGroup);
    m_spinMinY->setRange(-100000.0, 100000.0);
    m_spinMinY->setValue(m_pageSettings.minY());
    connect(m_spinMinY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PageSetupDialog::onLimitsChanged);
    limitsLayout->addWidget(m_spinMinY, 0, 3);

    limitsLayout->addWidget(new QLabel(tr("Max X:"), limitsGroup), 1, 0);
    m_spinMaxX = new QDoubleSpinBox(limitsGroup);
    m_spinMaxX->setRange(-100000.0, 100000.0);
    m_spinMaxX->setValue(m_pageSettings.maxX());
    connect(m_spinMaxX, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PageSetupDialog::onLimitsChanged);
    limitsLayout->addWidget(m_spinMaxX, 1, 1);

    limitsLayout->addWidget(new QLabel(tr("Max Y:"), limitsGroup), 1, 2);
    m_spinMaxY = new QDoubleSpinBox(limitsGroup);
    m_spinMaxY->setRange(-100000.0, 100000.0);
    m_spinMaxY->setValue(m_pageSettings.maxY());
    connect(m_spinMaxY, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PageSetupDialog::onLimitsChanged);
    limitsLayout->addWidget(m_spinMaxY, 1, 3);

    leftLayout->addWidget(limitsGroup);
    contentLayout->addWidget(leftWidget, 1);

    // Right live preview
    auto* rightWidget = new QWidget(page);
    auto* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    auto* previewGroup = new QGroupBox(tr("Live Page Preview"), rightWidget);
    auto* prevGroupLayout = new QVBoxLayout(previewGroup);
    m_pagePreview = new PagePreviewWidget(previewGroup);
    prevGroupLayout->addWidget(m_pagePreview);
    rightLayout->addWidget(previewGroup);

    contentLayout->addWidget(rightWidget, 1);
    layout->addLayout(contentLayout, 1);

    // Bottom navigation
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    auto* btnCancel = new QPushButton(tr("Cancel"), page);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(btnCancel);

    auto* btnNext = new QPushButton(tr("Next: Drawing Units >"), page);
    btnNext->setDefault(true);
    connect(btnNext, &QPushButton::clicked, this, &PageSetupDialog::goToStep2);
    btnLayout->addWidget(btnNext);
    layout->addLayout(btnLayout);

    return page;
}

QWidget* PageSetupDialog::createUnitsStep() {
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    auto* header = new QLabel(tr("<h3>2. Drawing Units & Precision</h3><p style='color:#aaa'>Set the authoritative real-world CAD measurement units and display precision.</p>"), page);
    layout->addWidget(header);

    auto* unitsGroup = new QGroupBox(tr("Unit System Configuration"), page);
    auto* uLayout = new QGridLayout(unitsGroup);
    uLayout->setSpacing(10);

    uLayout->addWidget(new QLabel(tr("Base Unit:"), unitsGroup), 0, 0);
    m_comboUnits = new QComboBox(unitsGroup);
    m_comboUnits->addItems({
        QStringLiteral("Millimeter (mm)"),
        QStringLiteral("Centimeter (cm)"),
        QStringLiteral("Meter (m)"),
        QStringLiteral("Inch (in)"),
        QStringLiteral("Foot (ft)"),
        QStringLiteral("Custom Unit")
    });
    m_comboUnits->setCurrentIndex(static_cast<int>(m_unitSettings.type()));
    connect(m_comboUnits, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PageSetupDialog::onUnitChanged);
    uLayout->addWidget(m_comboUnits, 0, 1);

    uLayout->addWidget(new QLabel(tr("Decimal Precision:"), unitsGroup), 1, 0);
    m_spinPrecision = new QSpinBox(unitsGroup);
    m_spinPrecision->setRange(0, 6);
    m_spinPrecision->setValue(m_unitSettings.precision());
    connect(m_spinPrecision, QOverload<int>::of(&QSpinBox::valueChanged), this, &PageSetupDialog::onPrecisionChanged);
    uLayout->addWidget(m_spinPrecision, 1, 1);

    uLayout->addWidget(new QLabel(tr("Custom Scale to mm:"), unitsGroup), 2, 0);
    m_spinCustomScale = new QDoubleSpinBox(unitsGroup);
    m_spinCustomScale->setRange(0.0001, 100000.0);
    m_spinCustomScale->setValue(m_unitSettings.customScaleToMm());
    m_spinCustomScale->setEnabled(m_unitSettings.type() == UnitType::Custom);
    connect(m_spinCustomScale, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this](double val) {
        m_unitSettings.setCustomScaleToMm(val);
        updateUnitPreview();
    });
    uLayout->addWidget(m_spinCustomScale, 2, 1);

    uLayout->addWidget(new QLabel(tr("Custom Unit Name:"), unitsGroup), 3, 0);
    m_editCustomUnitName = new QLineEdit(m_unitSettings.customUnitName(), unitsGroup);
    m_editCustomUnitName->setEnabled(m_unitSettings.type() == UnitType::Custom);
    connect(m_editCustomUnitName, &QLineEdit::textChanged, this, [this](const QString& text) {
        m_unitSettings.setCustomUnitName(text);
        updateUnitPreview();
    });
    uLayout->addWidget(m_editCustomUnitName, 3, 1);

    layout->addWidget(unitsGroup);

    // Live preview group
    auto* prevGroup = new QGroupBox(tr("Dimension Format Preview"), page);
    auto* pLayout = new QVBoxLayout(prevGroup);
    m_labelUnitPreview = new QLabel(prevGroup);
    m_labelUnitPreview->setStyleSheet(QStringLiteral("font-family: 'Consolas', monospace; font-size: 13px; color: #00ff7f; background-color: #1e1e24; padding: 10px; border-radius: 4px;"));
    pLayout->addWidget(m_labelUnitPreview);
    layout->addWidget(prevGroup);

    layout->addStretch();

    // Bottom navigation
    auto* btnLayout = new QHBoxLayout();
    auto* btnBack = new QPushButton(tr("< Back"), page);
    connect(btnBack, &QPushButton::clicked, this, &PageSetupDialog::goToStep1);
    btnLayout->addWidget(btnBack);

    btnLayout->addStretch();

    auto* btnCancel = new QPushButton(tr("Cancel"), page);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(btnCancel);

    auto* btnFinish = new QPushButton(tr("Create Drawing"), page);
    btnFinish->setDefault(true);
    connect(btnFinish, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(btnFinish);
    layout->addLayout(btnLayout);

    return page;
}

void PageSetupDialog::onPageSizeChanged(int index) {
    auto size = static_cast<PageSize>(index);
    m_pageSettings.setPageSize(size);
    m_spinCustomW->setEnabled(size == PageSize::Custom);
    m_spinCustomH->setEnabled(size == PageSize::Custom);

    m_spinMinX->setValue(m_pageSettings.minX());
    m_spinMinY->setValue(m_pageSettings.minY());
    m_spinMaxX->setValue(m_pageSettings.maxX());
    m_spinMaxY->setValue(m_pageSettings.maxY());

    updatePagePreview();
}

void PageSetupDialog::onOrientationChanged() {
    m_pageSettings.setOrientation(m_radioPortrait->isChecked() ? PageOrientation::Portrait : PageOrientation::Landscape);
    m_spinMinX->setValue(m_pageSettings.minX());
    m_spinMinY->setValue(m_pageSettings.minY());
    m_spinMaxX->setValue(m_pageSettings.maxX());
    m_spinMaxY->setValue(m_pageSettings.maxY());
    updatePagePreview();
}

void PageSetupDialog::onCustomDimensionsChanged() {
    m_pageSettings.setCustomWidth(m_spinCustomW->value());
    m_pageSettings.setCustomHeight(m_spinCustomH->value());
    m_spinMaxX->setValue(m_pageSettings.maxX());
    m_spinMaxY->setValue(m_pageSettings.maxY());
    updatePagePreview();
}

void PageSetupDialog::onLimitsChanged() {
    m_pageSettings.setDrawingLimits(m_spinMinX->value(), m_spinMinY->value(),
                                   m_spinMaxX->value(), m_spinMaxY->value());
    updatePagePreview();
}

void PageSetupDialog::onUnitChanged(int index) {
    auto type = static_cast<UnitType>(index);
    m_unitSettings.setType(type);
    m_spinCustomScale->setEnabled(type == UnitType::Custom);
    m_editCustomUnitName->setEnabled(type == UnitType::Custom);
    updateUnitPreview();
}

void PageSetupDialog::onPrecisionChanged(int val) {
    m_unitSettings.setPrecision(val);
    updateUnitPreview();
}

void PageSetupDialog::updatePagePreview() {
    if (m_pagePreview) {
        m_pagePreview->setPageSettings(m_pageSettings);
    }
}

void PageSetupDialog::updateUnitPreview() {
    if (m_labelUnitPreview) {
        QString text = QStringLiteral(
            "Unit: %1\n"
            "Precision: %2 decimal places\n\n"
            "Examples:\n"
            "   %3\n"
            "   %4\n"
            "   %5"
        ).arg(m_unitSettings.unitName())
         .arg(m_unitSettings.precision())
         .arg(m_unitSettings.formatLinear(100.0))
         .arg(m_unitSettings.formatLinear(2500.5))
         .arg(m_unitSettings.formatArea(3000000.0));
        m_labelUnitPreview->setText(text);
    }
}

void PageSetupDialog::goToStep2() {
    m_stack->setCurrentIndex(1);
}

void PageSetupDialog::goToStep1() {
    m_stack->setCurrentIndex(0);
}
