#include "ui/AiAssistantDock.h"
#include "ui/MainWindow.h"
#include "document/CadDocument.h"
#include "canvas/CadScene.h"
#include "canvas/CadView.h"
#include "ai/AiCommandExecutor.h"
#include "ai/AiDimensionGenerator.h"
#include "ai/AiFloorPlanProvider.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QScrollArea>

AiAssistantDock::AiAssistantDock(CadDocument* document, MainWindow* mainWindow, QWidget* parent)
    : QDockWidget(QStringLiteral("AI Assistant"), parent)
    , m_document(document)
    , m_mainWindow(mainWindow)
    , m_aiClient(std::make_unique<AiClient>(this))
{
    setObjectName(QStringLiteral("AiAssistantDock"));
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    setupUi();
}

void AiAssistantDock::setupUi() {
    auto container = new QWidget(this);
    auto mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(6);

    m_tabWidget = new QTabWidget(container);
    m_tabWidget->addTab(createGenerateTab(), QStringLiteral("Generate"));
    m_tabWidget->addTab(createAnalyzeTab(), QStringLiteral("Analyze & Clean"));
    m_tabWidget->addTab(createDimensionTab(), QStringLiteral("Dimension"));
    m_tabWidget->addTab(createRagTab(), QStringLiteral("CAD Knowledge"));

    mainLayout->addWidget(m_tabWidget);
    setWidget(container);
}

QWidget* AiAssistantDock::createGenerateTab() {
    auto page = new QWidget(this);
    auto layout = new QVBoxLayout(page);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(8);

    auto lblPrompt = new QLabel(QStringLiteral("<b>Natural Language Architectural / CAD Prompt:</b>"), page);
    layout->addWidget(lblPrompt);

    // Input line & Action buttons
    auto inputLayout = new QVBoxLayout();
    m_promptInput = new QLineEdit(page);
    m_promptInput->setPlaceholderText(QStringLiteral("e.g. 'Create a 10 m × 12 m residential ground floor with 3 bedrooms, kitchen, living room, 2 toilets, staircase, doors, windows and dimensions.'"));
    connect(m_promptInput, &QLineEdit::returnPressed, this, &AiAssistantDock::onGenerateClicked);
    inputLayout->addWidget(m_promptInput);

    auto btnRowLayout = new QHBoxLayout();
    m_btnGenerate = new QPushButton(QStringLiteral("⚡ Generate Plan"), page);
    m_btnGenerate->setStyleSheet(QStringLiteral("font-weight: bold; background-color: #2b5b84; color: white; padding: 6px 12px; border-radius: 3px;"));
    connect(m_btnGenerate, &QPushButton::clicked, this, &AiAssistantDock::onGenerateClicked);
    btnRowLayout->addWidget(m_btnGenerate);

    m_btnPreviewPlan = new QPushButton(QStringLiteral("👁 Preview on Canvas"), page);
    m_btnPreviewPlan->setStyleSheet(QStringLiteral("font-weight: bold; background-color: #0288d1; color: white; padding: 6px 12px; border-radius: 3px;"));
    connect(m_btnPreviewPlan, &QPushButton::clicked, this, &AiAssistantDock::onPreviewPlanClicked);
    btnRowLayout->addWidget(m_btnPreviewPlan);

    inputLayout->addLayout(btnRowLayout);
    layout->addLayout(inputLayout);

    // Architectural Options GroupBox
    auto optGroup = new QGroupBox(QStringLiteral("Architectural Plan Settings & AI Provider"), page);
    auto optLayout = new QGridLayout(optGroup);
    optLayout->setContentsMargins(6, 6, 6, 6);
    optLayout->setHorizontalSpacing(8);
    optLayout->setVerticalSpacing(6);

    optLayout->addWidget(new QLabel(QStringLiteral("Wall Style:"), optGroup), 0, 0);
    m_wallStyleCombo = new QComboBox(optGroup);
    m_wallStyleCombo->addItems({ QStringLiteral("Double-Line (200/100mm)"), QStringLiteral("Single-Line (Centerline)") });
    optLayout->addWidget(m_wallStyleCombo, 0, 1);

    optLayout->addWidget(new QLabel(QStringLiteral("Ext Wall (mm):"), optGroup), 0, 2);
    m_extWallThickSpin = new QDoubleSpinBox(optGroup);
    m_extWallThickSpin->setRange(50.0, 500.0);
    m_extWallThickSpin->setValue(200.0);
    optLayout->addWidget(m_extWallThickSpin, 0, 3);

    optLayout->addWidget(new QLabel(QStringLiteral("Int Wall (mm):"), optGroup), 1, 0);
    m_intWallThickSpin = new QDoubleSpinBox(optGroup);
    m_intWallThickSpin->setRange(50.0, 300.0);
    m_intWallThickSpin->setValue(100.0);
    optLayout->addWidget(m_intWallThickSpin, 1, 1);

    optLayout->addWidget(new QLabel(QStringLiteral("AI Provider:"), optGroup), 1, 2);
    m_aiProviderCombo = new QComboBox(optGroup);
    for (const auto& p : AiFloorPlanEngine::instance().providers()) {
        m_aiProviderCombo->addItem(p->displayName(), p->id());
    }
    connect(m_aiProviderCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AiAssistantDock::onProviderChanged);
    optLayout->addWidget(m_aiProviderCombo, 1, 3);

    m_providerStatusLabel = new QLabel(AiFloorPlanEngine::instance().activeProvider()->configurationStatus(), optGroup);
    m_providerStatusLabel->setStyleSheet(QStringLiteral("font-size: 10px; color: #828997; font-style: italic;"));
    optLayout->addWidget(m_providerStatusLabel, 2, 0, 1, 4);

    layout->addWidget(optGroup);

    // Clarification & Ambiguity Feedback Box (Hidden by default)
    m_clarificationBox = new QGroupBox(QStringLiteral("❓ Clarification Needed (Ambiguity Detected)"), page);
    m_clarificationBox->setStyleSheet(QStringLiteral("QGroupBox { border: 1px solid #ffa000; border-radius: 4px; margin-top: 8px; font-weight: bold; color: #ffb300; background: #262010; } QGroupBox::title { subcontrol-origin: margin; left: 8px; }"));
    auto clarLayout = new QVBoxLayout(m_clarificationBox);
    m_clarificationLabel = new QLabel(m_clarificationBox);
    m_clarificationLabel->setWordWrap(true);
    m_clarificationLabel->setStyleSheet(QStringLiteral("color: #ffe082; font-weight: normal;"));
    clarLayout->addWidget(m_clarificationLabel);

    m_clarificationButtonsWidget = new QWidget(m_clarificationBox);
    auto clarBtnLayout = new QHBoxLayout(m_clarificationButtonsWidget);
    clarBtnLayout->setContentsMargins(0, 0, 0, 0);
    clarLayout->addWidget(m_clarificationButtonsWidget);
    m_clarificationBox->hide();
    layout->addWidget(m_clarificationBox);

    // Error Feedback Banner (Hidden by default)
    m_errorFeedbackLabel = new QLabel(page);
    m_errorFeedbackLabel->setWordWrap(true);
    m_errorFeedbackLabel->setStyleSheet(QStringLiteral("background: #3b1818; border: 1px solid #ef5350; color: #ffcdd2; padding: 6px; border-radius: 3px;"));
    m_errorFeedbackLabel->hide();
    layout->addWidget(m_errorFeedbackLabel);

    // Quick suggestion chips
    auto chipLabel = new QLabel(QStringLiteral("<small>Quick Actions / Architectural Presets:</small>"), page);
    layout->addWidget(chipLabel);

    auto chipsLayout = new QGridLayout();
    QStringList suggestions = {
        QStringLiteral("Create a 10 m × 12 m residential ground floor with 3 bedrooms, kitchen, living room, 2 toilets, staircase, doors, windows and dimensions."),
        QStringLiteral("12m x 15m single-line floor plan with 3 bedrooms, kitchen, living room, 2 toilets"),
        QStringLiteral("8m x 10m 2-BHK ground floor with kitchen, living room, bathroom, staircase"),
        QStringLiteral("Create a 5m x 4m room"),
        QStringLiteral("Add a 900mm door"),
        QStringLiteral("Create a staircase with 12 steps")
    };

    int row = 0, col = 0;
    for (const auto& text : suggestions) {
        auto btn = new QPushButton(text, page);
        btn->setStyleSheet(QStringLiteral("text-align: left; padding: 4px 6px; font-size: 11px;"));
        btn->setProperty("promptText", text);
        connect(btn, &QPushButton::clicked, this, &AiAssistantDock::onQuickPromptClicked);
        chipsLayout->addWidget(btn, row, col);
        col++;
        if (col > 1) {
            col = 0;
            row++;
        }
    }
    layout->addLayout(chipsLayout);

    // Human Approval Workflow Box
    m_proposalGroupBox = new QGroupBox(QStringLiteral("Proposed Plan (Canvas Preview & Approval)"), page);
    auto groupLayout = new QVBoxLayout(m_proposalGroupBox);

    m_proposalSummaryLabel = new QLabel(QStringLiteral("No pending proposal."), m_proposalGroupBox);
    m_proposalSummaryLabel->setStyleSheet(QStringLiteral("font-weight: bold; color: #00e676;"));
    groupLayout->addWidget(m_proposalSummaryLabel);

    m_proposalDescLabel = new QLabel(QStringLiteral(""), m_proposalGroupBox);
    m_proposalDescLabel->setWordWrap(true);
    groupLayout->addWidget(m_proposalDescLabel);

    m_proposalOpsList = new QListWidget(m_proposalGroupBox);
    m_proposalOpsList->setMaximumHeight(130);
    groupLayout->addWidget(m_proposalOpsList);

    auto btnActionLayout = new QHBoxLayout();
    m_btnApplyProposal = new QPushButton(QStringLiteral("✓ Apply to Drawing"), m_proposalGroupBox);
    m_btnApplyProposal->setStyleSheet(QStringLiteral("background-color: #2e7d32; color: white; font-weight: bold; padding: 7px; border-radius: 3px;"));
    m_btnApplyProposal->setEnabled(false);
    connect(m_btnApplyProposal, &QPushButton::clicked, this, &AiAssistantDock::onApplyProposalClicked);
    btnActionLayout->addWidget(m_btnApplyProposal);

    m_btnDiscardProposal = new QPushButton(QStringLiteral("✗ Cancel / Discard"), m_proposalGroupBox);
    m_btnDiscardProposal->setStyleSheet(QStringLiteral("background-color: #c62828; color: white; font-weight: bold; padding: 7px; border-radius: 3px;"));
    m_btnDiscardProposal->setEnabled(false);
    connect(m_btnDiscardProposal, &QPushButton::clicked, this, &AiAssistantDock::onDiscardProposalClicked);
    btnActionLayout->addWidget(m_btnDiscardProposal);

    groupLayout->addLayout(btnActionLayout);

    m_statusLabel = new QLabel(QStringLiteral(""), m_proposalGroupBox);
    m_statusLabel->setStyleSheet(QStringLiteral("color: #888888; font-style: italic;"));
    m_statusLabel->setWordWrap(true);
    groupLayout->addWidget(m_statusLabel);

    layout->addWidget(m_proposalGroupBox);
    layout->addStretch();
    return page;
}

QWidget* AiAssistantDock::createAnalyzeTab() {
    auto page = new QWidget(this);
    auto layout = new QVBoxLayout(page);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(8);

    auto btnLayout = new QHBoxLayout();
    m_btnAnalyze = new QPushButton(QStringLiteral("Run Geometry Analysis"), page);
    m_btnAnalyze->setStyleSheet(QStringLiteral("background-color: #1976d2; color: white; font-weight: bold; padding: 6px;"));
    connect(m_btnAnalyze, &QPushButton::clicked, this, &AiAssistantDock::onAnalyzeClicked);
    btnLayout->addWidget(m_btnAnalyze);

    m_btnAutoClean = new QPushButton(QStringLiteral("Auto-Clean Duplicates"), page);
    m_btnAutoClean->setStyleSheet(QStringLiteral("background-color: #e65100; color: white; font-weight: bold; padding: 6px;"));
    connect(m_btnAutoClean, &QPushButton::clicked, this, &AiAssistantDock::onCleanClicked);
    btnLayout->addWidget(m_btnAutoClean);
    layout->addLayout(btnLayout);

    m_statsLabel = new QLabel(QStringLiteral("Click 'Run Geometry Analysis' to inspect drawing."), page);
    m_statsLabel->setStyleSheet(QStringLiteral("background: #2a2a2a; border-radius: 4px; padding: 8px; font-family: monospace;"));
    m_statsLabel->setWordWrap(true);
    layout->addWidget(m_statsLabel);

    auto lblIssues = new QLabel(QStringLiteral("<b>Detected Issues & Recommendations:</b>"), page);
    layout->addWidget(lblIssues);

    m_issuesList = new QListWidget(page);
    layout->addWidget(m_issuesList);

    return page;
}

QWidget* AiAssistantDock::createDimensionTab() {
    auto page = new QWidget(this);
    auto layout = new QVBoxLayout(page);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(8);

    auto lblTitle = new QLabel(QStringLiteral("<b>AI Assisted Dimensioning:</b>"), page);
    layout->addWidget(lblTitle);

    auto desc = new QLabel(QStringLiteral("Inspects geometry shapes and automatically places engineering dimensions according to standard offset rules."), page);
    desc->setWordWrap(true);
    layout->addWidget(desc);

    auto offsetLayout = new QHBoxLayout();
    offsetLayout->addWidget(new QLabel(QStringLiteral("Dimension Line Offset (mm):"), page));
    m_dimOffsetSpin = new QDoubleSpinBox(page);
    m_dimOffsetSpin->setRange(5.0, 500.0);
    m_dimOffsetSpin->setValue(25.0);
    offsetLayout->addWidget(m_dimOffsetSpin);
    layout->addLayout(offsetLayout);

    m_btnDimensionAll = new QPushButton(QStringLiteral("Dimension Entire Drawing"), page);
    m_btnDimensionAll->setStyleSheet(QStringLiteral("background-color: #0288d1; color: white; font-weight: bold; padding: 6px;"));
    connect(m_btnDimensionAll, &QPushButton::clicked, this, &AiAssistantDock::onDimensionAllClicked);
    layout->addWidget(m_btnDimensionAll);

    m_btnDimensionSelected = new QPushButton(QStringLiteral("Dimension Selected Entities"), page);
    connect(m_btnDimensionSelected, &QPushButton::clicked, this, &AiAssistantDock::onDimensionSelectedClicked);
    layout->addWidget(m_btnDimensionSelected);

    layout->addStretch();
    return page;
}

QWidget* AiAssistantDock::createRagTab() {
    auto page = new QWidget(this);
    auto layout = new QVBoxLayout(page);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(8);

    auto lblTitle = new QLabel(QStringLiteral("<b>GenCAD RAG & Engineering Assistant:</b>"), page);
    layout->addWidget(lblTitle);

    auto inputLayout = new QHBoxLayout();
    m_ragQueryInput = new QLineEdit(page);
    m_ragQueryInput->setPlaceholderText(QStringLiteral("Ask a CAD or standard question, e.g. 'How to use Trim', 'ISO 128'"));
    connect(m_ragQueryInput, &QLineEdit::returnPressed, this, &AiAssistantDock::onRagSearchClicked);
    inputLayout->addWidget(m_ragQueryInput);

    m_btnRagSearch = new QPushButton(QStringLiteral("Ask"), page);
    connect(m_btnRagSearch, &QPushButton::clicked, this, &AiAssistantDock::onRagSearchClicked);
    inputLayout->addWidget(m_btnRagSearch);
    layout->addLayout(inputLayout);

    // Topic quick buttons
    auto topicLayout = new QHBoxLayout();
    QStringList topics = { QStringLiteral("Tools Guide"), QStringLiteral("ISO 128"), QStringLiteral("ANSI Y14.5"), QStringLiteral("AIA Layers"), QStringLiteral("Shortcuts") };
    for (const auto& t : topics) {
        auto b = new QPushButton(t, page);
        b->setStyleSheet(QStringLiteral("font-size: 10px; padding: 2px 4px;"));
        b->setProperty("topic", t);
        connect(b, &QPushButton::clicked, this, &AiAssistantDock::onQuickRagTopicClicked);
        topicLayout->addWidget(b);
    }
    layout->addLayout(topicLayout);

    m_ragBrowser = new QTextBrowser(page);
    m_ragBrowser->setOpenExternalLinks(true);
    m_ragBrowser->setMarkdown(QStringLiteral(
        "### Welcome to GenCAD AI CAD Assistant\n\n"
        "Ask questions about drawing tools, modify operations, drafting standards (ISO 128, ANSI Y14.5), AIA layer conventions, or shortcuts."
    ));
    layout->addWidget(m_ragBrowser);

    return page;
}

void AiAssistantDock::onQuickPromptClicked() {
    auto btn = qobject_cast<QPushButton*>(sender());
    if (btn) {
        QString text = btn->property("promptText").toString();
        m_promptInput->setText(text);
        onGenerateClicked();
    }
}

void AiAssistantDock::onQuickRagTopicClicked() {
    auto btn = qobject_cast<QPushButton*>(sender());
    if (btn) {
        QString topic = btn->property("topic").toString();
        m_ragQueryInput->setText(topic);
        onRagSearchClicked();
    }
}

void AiAssistantDock::promptForGeneration(const QString& samplePrompt) {
    if (!samplePrompt.isEmpty()) {
        m_promptInput->setText(samplePrompt);
    }
    m_tabWidget->setCurrentIndex(0);
    show();
    raise();
    m_promptInput->setFocus();
}

void AiAssistantDock::runAnalysis() {
    m_tabWidget->setCurrentIndex(1);
    show();
    raise();
    onAnalyzeClicked();
}

void AiAssistantDock::runAutoClean() {
    m_tabWidget->setCurrentIndex(1);
    show();
    raise();
    onCleanClicked();
}

void AiAssistantDock::runAutoDimension() {
    m_tabWidget->setCurrentIndex(2);
    show();
    raise();
    onDimensionAllClicked();
}

void AiAssistantDock::onGenerateClicked() {
    if (m_clarificationBox) m_clarificationBox->hide();
    if (m_errorFeedbackLabel) m_errorFeedbackLabel->hide();

    QString prompt = m_promptInput->text().trimmed();
    if (prompt.isEmpty()) return;

    QString lower = prompt.toLower();

    // Check if prompt is an architectural floor plan generation instruction
    bool isFloorPlan = lower.contains("residential") || lower.contains("floor plan") ||
                       lower.contains("ground floor") || lower.contains("bhk") ||
                       lower.contains("bedroom") || (lower.contains("kitchen") && lower.contains("living")) ||
                       (lower.contains("kitchen") && lower.contains("toilet")) ||
                       (lower.contains("x") && (lower.contains("m") || lower.contains("room")));

    if (isFloorPlan) {
        FloorPlanConfig baseConfig;
        if (m_wallStyleCombo) {
            baseConfig.wallStyle = (m_wallStyleCombo->currentIndex() == 1)
                                   ? FloorPlanWallStyle::SingleLine
                                   : FloorPlanWallStyle::DoubleLine;
        }
        if (m_extWallThickSpin) {
            baseConfig.exteriorWallThickness = m_extWallThickSpin->value();
        }
        if (m_intWallThickSpin) {
            baseConfig.interiorWallThickness = m_intWallThickSpin->value();
        }

        m_statusLabel->setText(QStringLiteral("Generating architectural floor plan..."));
        FloorPlanProviderResult res = AiFloorPlanEngine::instance().generatePlan(prompt, &baseConfig);

        if (res.needsClarification) {
            if (m_clarificationBox) {
                m_clarificationLabel->setText(res.clarificationQuestion);

                auto layout = m_clarificationButtonsWidget->layout();
                QLayoutItem* item;
                while ((item = layout->takeAt(0)) != nullptr) {
                    delete item->widget();
                    delete item;
                }
                for (const auto& opt : res.clarificationOptions) {
                    auto btn = new QPushButton(opt, m_clarificationButtonsWidget);
                    btn->setStyleSheet(QStringLiteral("background: #5d4037; color: white; padding: 4px 8px; font-size: 11px; border-radius: 2px;"));
                    btn->setProperty("clarificationText", opt);
                    connect(btn, &QPushButton::clicked, this, &AiAssistantDock::onClarificationOptionClicked);
                    layout->addWidget(btn);
                }
                m_clarificationBox->show();
            }
            m_statusLabel->setText(QStringLiteral("Essential requirements ambiguous. Please select or specify dimensions."));
            clearProposal();
            return;
        }

        if (!res.success) {
            if (m_errorFeedbackLabel) {
                m_errorFeedbackLabel->setText(QStringLiteral("⚠️ Validation Error:\n%1").arg(res.errorMessage));
                m_errorFeedbackLabel->show();
            }
            m_statusLabel->setText(QStringLiteral("Plan generation rejected by geometry validation."));
            clearProposal();
            return;
        }

        m_stagedFloorPlan = res.layout;
        m_stagedPreviewEntities = res.layout.instantiateEntities();

        // Canvas Preview
        if (m_mainWindow && m_mainWindow->view() && m_mainWindow->view()->cadScene()) {
            m_mainWindow->view()->cadScene()->setPreviewEntities(m_stagedPreviewEntities);
            m_mainWindow->zoomFit();
        }

        displayProposal(res.proposal);
        m_statusLabel->setText(QStringLiteral("Plan previewed on canvas (%1 entities, %2). Click 'Apply to Drawing' to commit.")
                               .arg(m_stagedPreviewEntities.size())
                               .arg(res.providerUsed));
        return;
    }

    m_statusLabel->setText(QStringLiteral("Generating CAD DSL..."));
    AiProposal proposal = m_aiClient->generate(prompt, m_document);
    m_stagedPreviewEntities = AiCommandExecutor::instantiateEntities(proposal);
    if (m_mainWindow && m_mainWindow->view() && m_mainWindow->view()->cadScene()) {
        m_mainWindow->view()->cadScene()->setPreviewEntities(m_stagedPreviewEntities);
        m_mainWindow->zoomFit();
    }
    displayProposal(proposal);
}

void AiAssistantDock::onPreviewPlanClicked() {
    onGenerateClicked();
}

void AiAssistantDock::onProviderChanged(int index) {
    auto providers = AiFloorPlanEngine::instance().providers();
    if (index >= 0 && index < static_cast<int>(providers.size())) {
        AiFloorPlanEngine::instance().setActiveProvider(providers[index]->id());
        if (m_providerStatusLabel) {
            m_providerStatusLabel->setText(providers[index]->configurationStatus());
        }
    }
}

void AiAssistantDock::onClarificationOptionClicked() {
    auto btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    QString opt = btn->property("clarificationText").toString();

    QRegularExpression re(QStringLiteral(R"((\d+m\s*(?:×|x)\s*\d+m))"));
    auto m = re.match(opt);
    QString dimStr = m.hasMatch() ? m.captured(1) : QStringLiteral("10m x 12m");

    QString currentPrompt = m_promptInput->text().trimmed();
    if (currentPrompt.isEmpty()) {
        m_promptInput->setText(QStringLiteral("Create a %1 residential ground floor with 3 bedrooms, kitchen, living room, 2 toilets, staircase, doors, windows and dimensions").arg(dimStr));
    } else {
        m_promptInput->setText(QStringLiteral("%1 %2").arg(dimStr, currentPrompt));
    }

    if (m_clarificationBox) m_clarificationBox->hide();
    onGenerateClicked();
}

void AiAssistantDock::displayProposal(const AiProposal& proposal) {
    m_stagedProposal = proposal;
    m_proposalSummaryLabel->setText(proposal.summaryText());
    m_proposalDescLabel->setText(proposal.description);

    m_proposalOpsList->clear();
    int idx = 1;
    for (const auto& op : proposal.operations) {
        QString opStr = QStringLiteral("%1. %2").arg(idx).arg(AiOperation::operationTypeToString(op.type));
        if (op.type == AiOperationType::CreateRectangle) {
            opStr += QStringLiteral(" (%1 x %2 mm)").arg(op.width).arg(op.height);
        } else if (op.type == AiOperationType::CreateCircle) {
            opStr += QStringLiteral(" (r = %1 mm)").arg(op.radius);
        } else if (op.type == AiOperationType::CreateLine) {
            opStr += QStringLiteral(" (%1,%2 -> %3,%4)")
                     .arg(op.p1.x(), 0, 'f', 0).arg(op.p1.y(), 0, 'f', 0)
                     .arg(op.p2.x(), 0, 'f', 0).arg(op.p2.y(), 0, 'f', 0);
        } else if (op.type == AiOperationType::CreateDimension) {
            opStr += QStringLiteral(" (layer: %1)").arg(op.layer);
        } else if (op.type == AiOperationType::DeleteEntity) {
            opStr += QStringLiteral(" (entity ID: %1)").arg(op.targetId.toString(QUuid::WithoutBraces));
        }
        m_proposalOpsList->addItem(opStr);
        idx++;
    }

    bool hasOps = !proposal.operations.empty();
    m_btnApplyProposal->setEnabled(hasOps);
    m_btnDiscardProposal->setEnabled(hasOps);
    m_statusLabel->setText(hasOps ? QStringLiteral("Proposal staged. Click 'Apply to Drawing' to execute.")
                                  : QStringLiteral("No operations generated."));
}

void AiAssistantDock::clearProposal() {
    if (m_mainWindow && m_mainWindow->view() && m_mainWindow->view()->cadScene()) {
        m_mainWindow->view()->cadScene()->clearPreviewEntities();
    }
    m_stagedPreviewEntities.clear();
    m_stagedFloorPlan = FloorPlanLayout();
    m_stagedProposal = AiProposal();
    m_proposalSummaryLabel->setText(QStringLiteral("No pending proposal."));
    m_proposalDescLabel->clear();
    m_proposalOpsList->clear();
    m_btnApplyProposal->setEnabled(false);
    m_btnDiscardProposal->setEnabled(false);
    m_statusLabel->clear();
}

void AiAssistantDock::onApplyProposalClicked() {
    if (m_stagedProposal.operations.empty() || !m_document) return;

    if (m_mainWindow && m_mainWindow->view() && m_mainWindow->view()->cadScene()) {
        m_mainWindow->view()->cadScene()->clearPreviewEntities();
    }
    m_stagedPreviewEntities.clear();

    QString err;
    bool ok = AiCommandExecutor::executeProposal(m_stagedProposal, m_document, &err);
    if (!ok) {
        QMessageBox::critical(this, QStringLiteral("AI Execution Failed"),
                              QStringLiteral("Failed to execute proposal:\n%1").arg(err));
        return;
    }

    m_statusLabel->setText(QStringLiteral("✓ Applied successfully to drawing! Press Ctrl+Z to undo."));
    clearProposal();
    if (m_mainWindow) {
        m_mainWindow->zoomFit();
    }
}

void AiAssistantDock::onDiscardProposalClicked() {
    if (m_mainWindow && m_mainWindow->view() && m_mainWindow->view()->cadScene()) {
        m_mainWindow->view()->cadScene()->clearPreviewEntities();
    }
    m_stagedPreviewEntities.clear();
    clearProposal();
    m_statusLabel->setText(QStringLiteral("Plan discarded. Drawing remains completely unchanged."));
}

void AiAssistantDock::onAnalyzeClicked() {
    if (!m_document) return;
    AiAnalysisReport report = m_aiClient->analyze(m_document);
    displayAnalysisReport(report);
}

void AiAssistantDock::displayAnalysisReport(const AiAnalysisReport& report) {
    m_statsLabel->setText(report.summaryText());

    m_issuesList->clear();
    for (const auto& d : report.duplicates) {
        auto item = new QListWidgetItem(QStringLiteral("⚠️ %1 (%2)").arg(d.description, d.entityType));
        item->setForeground(QColor(255, 152, 0));
        m_issuesList->addItem(item);
    }
    for (const auto& o : report.overlaps) {
        auto item = new QListWidgetItem(QStringLiteral("⚠️ %1").arg(o.description));
        item->setForeground(QColor(255, 87, 34));
        m_issuesList->addItem(item);
    }
    for (const auto& l : report.layerIssues) {
        auto item = new QListWidgetItem(QStringLiteral("ℹ️ %1").arg(l.issueDescription));
        item->setForeground(QColor(33, 150, 243));
        m_issuesList->addItem(item);
    }
    for (const auto& s : report.suggestions) {
        auto item = new QListWidgetItem(QStringLiteral("💡 %1").arg(s));
        item->setForeground(QColor(76, 175, 80));
        m_issuesList->addItem(item);
    }

    if (m_issuesList->count() == 0) {
        auto item = new QListWidgetItem(QStringLiteral("✓ No issues detected. Drawing is clean!"));
        item->setForeground(QColor(76, 175, 80));
        m_issuesList->addItem(item);
    }
}

void AiAssistantDock::onCleanClicked() {
    if (!m_document) return;
    AiProposal proposal = m_aiClient->cleanup(m_document);
    if (proposal.operations.empty()) {
        QMessageBox::information(this, QStringLiteral("Clean Drawing"), QStringLiteral("No duplicate entities found. Drawing is already clean."));
        return;
    }

    m_tabWidget->setCurrentIndex(0);
    displayProposal(proposal);
}

void AiAssistantDock::onDimensionAllClicked() {
    if (!m_document) return;
    double offset = m_dimOffsetSpin->value();
    AiProposal proposal = AiDimensionGenerator::generateDimensions(m_document, {}, offset);
    if (proposal.operations.empty()) {
        QMessageBox::information(this, QStringLiteral("Auto-Dimension"), QStringLiteral("No geometry found to dimension."));
        return;
    }

    m_tabWidget->setCurrentIndex(0);
    displayProposal(proposal);
}

void AiAssistantDock::onDimensionSelectedClicked() {
    if (!m_document) return;
    auto selectedIds = m_document->selectedIds();
    if (selectedIds.empty()) {
        QMessageBox::information(this, QStringLiteral("Auto-Dimension"), QStringLiteral("Please select entities on the canvas first."));
        return;
    }

    double offset = m_dimOffsetSpin->value();
    AiProposal proposal = AiDimensionGenerator::generateDimensions(m_document, selectedIds, offset);
    m_tabWidget->setCurrentIndex(0);
    displayProposal(proposal);
}

void AiAssistantDock::onRagSearchClicked() {
    QString q = m_ragQueryInput->text().trimmed();
    if (q.isEmpty()) return;

    QString answer = m_aiClient->askRag(q);
    m_ragBrowser->setMarkdown(answer);
}
