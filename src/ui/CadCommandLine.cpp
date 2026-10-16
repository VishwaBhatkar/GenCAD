#include "ui/CadCommandLine.h"
#include <QKeyEvent>
#include <QScrollBar>

CadCommandLine::CadCommandLine(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
    setupCompleter();
}

void CadCommandLine::setupUi() {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setStyleSheet(QStringLiteral(
        "CadCommandLine { background-color: #1a1c23; border-top: 1px solid #2e3340; border-bottom: 1px solid #181a1f; }"
        "QTextEdit { background-color: #16181f; color: #abb2bf; border: none; font-family: 'Consolas', 'Courier New', monospace; font-size: 11px; padding: 4px; }"
        "QLabel { color: #00e5ff; font-weight: bold; font-family: 'Segoe UI', sans-serif; font-size: 11px; }"
        "QLineEdit { background-color: #21252b; color: #ffffff; border: 1px solid #3e4451; border-radius: 2px; padding: 3px 6px; font-family: 'Consolas', monospace; font-size: 11px; }"
        "QLineEdit:focus { border: 1px solid #007acc; background-color: #1e2227; }"
        "QToolButton { background-color: transparent; border: none; color: #828997; font-size: 9px; padding: 2px; }"
        "QToolButton:hover { color: #ffffff; background-color: #2c313a; }"
    ));

    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(6, 2, 6, 4);
    m_mainLayout->setSpacing(2);

    // Multi-line history transcript
    m_historyEdit = new QTextEdit(this);
    m_historyEdit->setReadOnly(true);
    m_historyEdit->setFixedHeight(48);
    m_historyEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_historyEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_historyEdit->append(QStringLiteral("<span style='color: #828997;'>GenCAD 4.0 AutoCAD Drafting Engine Ready. Type command or select tool.</span>"));
    m_mainLayout->addWidget(m_historyEdit);

    // Input row
    m_inputBar = new QWidget(this);
    auto* inputLayout = new QHBoxLayout(m_inputBar);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputLayout->setSpacing(6);

    m_btnExpandHistory = new QToolButton(m_inputBar);
    m_btnExpandHistory->setText(QStringLiteral("▲"));
    m_btnExpandHistory->setToolTip(tr("Expand / Collapse Command History"));
    connect(m_btnExpandHistory, &QToolButton::clicked, this, &CadCommandLine::toggleHistoryExpanded);
    inputLayout->addWidget(m_btnExpandHistory);

    m_promptLabel = new QLabel(tr("Command:"), m_inputBar);
    inputLayout->addWidget(m_promptLabel);

    m_commandEdit = new QLineEdit(m_inputBar);
    m_commandEdit->setPlaceholderText(tr("Type a command (LINE, CIRCLE, RECT, PLINE, DIM, TRIM, EXTEND, MOVE...)"));
    m_commandEdit->installEventFilter(this);
    connect(m_commandEdit, &QLineEdit::returnPressed, this, &CadCommandLine::onCommandReturnPressed);
    inputLayout->addWidget(m_commandEdit, 1);

    // Dynamic numeric fields
    m_label1 = new QLabel(m_inputBar);
    m_field1 = new QLineEdit(m_inputBar);
    m_field1->setFixedWidth(90);
    m_field1->installEventFilter(this);
    connect(m_field1, &QLineEdit::returnPressed, this, &CadCommandLine::onFieldReturnPressed);
    inputLayout->addWidget(m_label1);
    inputLayout->addWidget(m_field1);

    m_label2 = new QLabel(m_inputBar);
    m_field2 = new QLineEdit(m_inputBar);
    m_field2->setFixedWidth(90);
    m_field2->installEventFilter(this);
    connect(m_field2, &QLineEdit::returnPressed, this, &CadCommandLine::onFieldReturnPressed);
    inputLayout->addWidget(m_label2);
    inputLayout->addWidget(m_field2);

    m_label3 = new QLabel(m_inputBar);
    m_field3 = new QLineEdit(m_inputBar);
    m_field3->setFixedWidth(90);
    m_field3->installEventFilter(this);
    connect(m_field3, &QLineEdit::returnPressed, this, &CadCommandLine::onFieldReturnPressed);
    inputLayout->addWidget(m_label3);
    inputLayout->addWidget(m_field3);

    m_mainLayout->addWidget(m_inputBar);

    hideFields();
}

void CadCommandLine::setupCompleter() {
    QStringList commands = {
        QStringLiteral("LINE"), QStringLiteral("L"),
        QStringLiteral("CIRCLE"), QStringLiteral("C"),
        QStringLiteral("RECTANGLE"), QStringLiteral("REC"), QStringLiteral("RECT"),
        QStringLiteral("ARC"), QStringLiteral("A"),
        QStringLiteral("POLYLINE"), QStringLiteral("PL"), QStringLiteral("PLINE"),
        QStringLiteral("POLYGON"), QStringLiteral("POL"),
        QStringLiteral("ELLIPSE"), QStringLiteral("EL"),
        QStringLiteral("DIMENSION"), QStringLiteral("DIM"), QStringLiteral("DIMALIGNED"),
        QStringLiteral("TEXT"), QStringLiteral("T"), QStringLiteral("MTEXT"),
        QStringLiteral("MOVE"), QStringLiteral("M"),
        QStringLiteral("ROTATE"), QStringLiteral("RO"),
        QStringLiteral("MIRROR"), QStringLiteral("MI"),
        QStringLiteral("COPY"), QStringLiteral("CO"), QStringLiteral("CP"),
        QStringLiteral("OFFSET"), QStringLiteral("O"),
        QStringLiteral("TRIM"), QStringLiteral("TR"),
        QStringLiteral("EXTEND"), QStringLiteral("EX"),
        QStringLiteral("SELECT"), QStringLiteral("SEL"), QStringLiteral("ESC"),
        QStringLiteral("DELETE"), QStringLiteral("DEL"), QStringLiteral("ERASE"), QStringLiteral("E"),
        QStringLiteral("ZOOM"), QStringLiteral("Z"), QStringLiteral("FIT"),
        QStringLiteral("PAGESETUP"), QStringLiteral("LIMITS"),
        QStringLiteral("ORTHO"), QStringLiteral("ORTHOMODE"),
        QStringLiteral("POLAR"),
        QStringLiteral("SNAP"), QStringLiteral("OSNAP"),
        QStringLiteral("GRID"),
        QStringLiteral("UNDO"), QStringLiteral("U"),
        QStringLiteral("REDO"),
        QStringLiteral("ABOUT"), QStringLiteral("HELP")
    };

    m_completer = new QCompleter(commands, this);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setFilterMode(Qt::MatchStartsWith);
    m_commandEdit->setCompleter(m_completer);
}

void CadCommandLine::toggleHistoryExpanded() {
    m_historyExpanded = !m_historyExpanded;
    if (m_historyExpanded) {
        m_historyEdit->setFixedHeight(110);
        m_btnExpandHistory->setText(QStringLiteral("▼"));
    } else {
        m_historyEdit->setFixedHeight(48);
        m_btnExpandHistory->setText(QStringLiteral("▲"));
    }
}

void CadCommandLine::appendHistory(const QString& line) {
    if (!m_historyEdit) return;
    m_historyEdit->append(line);
    m_historyEdit->verticalScrollBar()->setValue(m_historyEdit->verticalScrollBar()->maximum());
}

void CadCommandLine::appendMessage(const QString& msg, const QString& prefix) {
    if (!m_historyEdit) return;
    QString formatted;
    if (!prefix.isEmpty()) {
        formatted = QStringLiteral("<span style='color: #00e5ff;'>%1</span> %2").arg(prefix, msg);
    } else {
        formatted = QStringLiteral("<span style='color: #abb2bf;'>%1</span>").arg(msg);
    }
    m_historyEdit->append(formatted);
    m_historyEdit->verticalScrollBar()->setValue(m_historyEdit->verticalScrollBar()->maximum());
}

void CadCommandLine::clearHistory() {
    if (m_historyEdit) m_historyEdit->clear();
}

void CadCommandLine::setPrompt(const QString& prompt) {
    m_promptLabel->setText(prompt);
}

void CadCommandLine::clearPrompt() {
    m_promptLabel->setText(tr("Command:"));
    m_commandEdit->clear();
    hideFields();
}

void CadCommandLine::showFields(const QString& label1, const QString& label2, const QString& label3) {
    m_fieldsActive = true;
    m_commandEdit->hide();

    m_label1->setText(label1);
    m_label1->setVisible(!label1.isEmpty());
    m_field1->setVisible(!label1.isEmpty());

    m_label2->setText(label2);
    m_label2->setVisible(!label2.isEmpty());
    m_field2->setVisible(!label2.isEmpty());

    m_label3->setText(label3);
    m_label3->setVisible(!label3.isEmpty());
    m_field3->setVisible(!label3.isEmpty());
}

void CadCommandLine::updateFieldValues(double val1, double val2, double val3, int precision) {
    if (!m_fieldsActive) return;

    if (!m_field1->hasFocus()) {
        m_field1->setText(QString::number(val1, 'f', precision));
    }
    if (!m_field2->hasFocus() && m_field2->isVisible()) {
        m_field2->setText(QString::number(val2, 'f', precision));
    }
    if (!m_field3->hasFocus() && m_field3->isVisible()) {
        m_field3->setText(QString::number(val3, 'f', precision));
    }
}

void CadCommandLine::hideFields() {
    m_fieldsActive = false;
    m_label1->hide();
    m_field1->hide();
    m_label2->hide();
    m_field2->hide();
    m_label3->hide();
    m_field3->hide();
    m_commandEdit->show();
}

double CadCommandLine::value1(bool* ok) const {
    return m_field1->text().toDouble(ok);
}

double CadCommandLine::value2(bool* ok) const {
    return m_field2->text().toDouble(ok);
}

double CadCommandLine::value3(bool* ok) const {
    return m_field3->text().toDouble(ok);
}

void CadCommandLine::focusFirstField() {
    if (m_fieldsActive && m_field1->isVisible()) {
        m_field1->setFocus();
        m_field1->selectAll();
    } else {
        m_commandEdit->setFocus();
    }
}

void CadCommandLine::setFocusToInput() {
    m_commandEdit->setFocus();
}

void CadCommandLine::onCommandReturnPressed() {
    QString cmd = m_commandEdit->text().trimmed();
    m_commandEdit->clear();
    if (!cmd.isEmpty()) {
        appendHistory(QStringLiteral("<span style='color: #00e5ff;'>Command:</span> <span style='color: #ffffff; font-weight: bold;'>%1</span>").arg(cmd));
        m_commandHistory.append(cmd);
        m_historyIndex = m_commandHistory.size();
        emit commandEntered(cmd);
    }
}

void CadCommandLine::onFieldReturnPressed() {
    bool ok1 = false, ok2 = false, ok3 = false;
    double v1 = value1(&ok1);
    double v2 = value2(&ok2);
    double v3 = value3(&ok3);
    emit valuesCommitted(v1, v2, v3);
}

bool CadCommandLine::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            appendHistory(QStringLiteral("<span style='color: #e06c75;'>*Cancel*</span>"));
            clearPrompt();
            emit cancelled();
            return true;
        } else if (keyEvent->key() == Qt::Key_Up) {
            if (obj == m_commandEdit && !m_commandHistory.isEmpty()) {
                if (m_historyIndex > 0) {
                    m_historyIndex--;
                    m_commandEdit->setText(m_commandHistory.at(m_historyIndex));
                    m_commandEdit->selectAll();
                }
                return true;
            }
        } else if (keyEvent->key() == Qt::Key_Down) {
            if (obj == m_commandEdit && !m_commandHistory.isEmpty()) {
                if (m_historyIndex < m_commandHistory.size() - 1) {
                    m_historyIndex++;
                    m_commandEdit->setText(m_commandHistory.at(m_historyIndex));
                    m_commandEdit->selectAll();
                } else {
                    m_historyIndex = m_commandHistory.size();
                    m_commandEdit->clear();
                }
                return true;
            }
        } else if (keyEvent->key() == Qt::Key_Tab) {
            if (obj == m_field1 && m_field2->isVisible()) {
                m_field2->setFocus();
                m_field2->selectAll();
                return true;
            } else if (obj == m_field2) {
                if (m_field3->isVisible()) {
                    m_field3->setFocus();
                    m_field3->selectAll();
                } else {
                    m_field1->setFocus();
                    m_field1->selectAll();
                }
                return true;
            } else if (obj == m_field3) {
                m_field1->setFocus();
                m_field1->selectAll();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}
