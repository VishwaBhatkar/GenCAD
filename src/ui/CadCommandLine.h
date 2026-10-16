#pragma once

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QCompleter>
#include <QStringList>
#include <QStringListModel>
#include <QToolButton>
#include <vector>

class CadCommandLine : public QWidget {
    Q_OBJECT

public:
    explicit CadCommandLine(QWidget* parent = nullptr);
    ~CadCommandLine() override = default;

    // Display command prompt or instructions
    void setPrompt(const QString& prompt);
    void clearPrompt();

    // Transcript / Log Output
    void appendHistory(const QString& line);
    void appendMessage(const QString& msg, const QString& prefix = QStringLiteral(""));
    void clearHistory();

    // Configure dynamic input fields
    void showFields(const QString& label1, const QString& label2 = QString(), const QString& label3 = QString());
    void updateFieldValues(double val1, double val2 = 0.0, double val3 = 0.0, int precision = 2);
    void hideFields();

    // Access current values
    double value1(bool* ok = nullptr) const;
    double value2(bool* ok = nullptr) const;
    double value3(bool* ok = nullptr) const;

    void focusFirstField();
    void setFocusToInput();

signals:
    void commandEntered(const QString& command);
    void valuesCommitted(double val1, double val2, double val3);
    void cancelled();

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onCommandReturnPressed();
    void onFieldReturnPressed();
    void toggleHistoryExpanded();

private:
    void setupUi();
    void setupCompleter();

    QVBoxLayout* m_mainLayout{nullptr};
    QTextEdit* m_historyEdit{nullptr};
    QWidget* m_inputBar{nullptr};
    QLabel* m_promptLabel{nullptr};
    QLineEdit* m_commandEdit{nullptr};

    // Dynamic numeric fields
    QLabel* m_label1{nullptr};
    QLineEdit* m_field1{nullptr};
    QLabel* m_label2{nullptr};
    QLineEdit* m_field2{nullptr};
    QLabel* m_label3{nullptr};
    QLineEdit* m_field3{nullptr};

    QToolButton* m_btnExpandHistory{nullptr};
    QCompleter* m_completer{nullptr};

    bool m_fieldsActive{false};
    bool m_historyExpanded{false};

    QStringList m_commandHistory;
    int m_historyIndex{-1};
};
