#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <memory>
#include "ai/AiDsl.h"
#include "ai/AiGeometryAnalyzer.h"
#include "ai/AiRagEngine.h"
#include "ai/AiLocalEngine.h"

class CadDocument;

class AiClient : public QObject {
    Q_OBJECT

public:
    explicit AiClient(QObject* parent = nullptr);
    ~AiClient() override = default;

    QString serverUrl() const { return m_serverUrl; }
    void setServerUrl(const QString& url);

    bool isOfflineMode() const { return m_offlineMode; }
    void setOfflineMode(bool offline) { m_offlineMode = offline; }

    // Core AI Operations
    AiProposal generate(const QString& prompt, const CadDocument* doc = nullptr, QString* errorMessage = nullptr);
    AiAnalysisReport analyze(const CadDocument* doc, QString* errorMessage = nullptr);
    AiProposal cleanup(const CadDocument* doc, QString* errorMessage = nullptr);
    AiProposal dimension(const CadDocument* doc, const std::vector<QUuid>& targetIds = {}, QString* errorMessage = nullptr);
    std::vector<RagQueryResult> queryRag(const QString& question, QString* errorMessage = nullptr);
    QString askRag(const QString& question);

    AiLocalEngine* localEngine() { return &m_localEngine; }

signals:
    void generationFinished(const AiProposal& proposal);
    void analysisFinished(const AiAnalysisReport& report);
    void errorOccurred(const QString& message);

private:
    QByteArray executeSyncPost(const QString& endpoint, const QByteArray& data, int* statusCode, QString* errorMessage);

    QString m_serverUrl{QStringLiteral("http://127.0.0.1:8000")};
    bool m_offlineMode{false};
    QNetworkAccessManager m_netManager;
    AiLocalEngine m_localEngine;
};
