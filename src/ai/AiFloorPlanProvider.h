#pragma once

#include <QString>
#include <QStringList>
#include <memory>
#include <vector>
#include <QObject>
#include <QNetworkAccessManager>
#include "ai/AiFloorPlanSpec.h"
#include "ai/AiFloorPlanGenerator.h"

struct FloorPlanProviderResult {
    bool success{false};
    bool needsClarification{false};
    QString clarificationQuestion;
    QStringList clarificationOptions;
    QString errorMessage;
    QString providerUsed;
    FloorPlanLayout layout;
    AiProposal proposal;
};

class IAiFloorPlanProvider {
public:
    virtual ~IAiFloorPlanProvider() = default;

    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual bool isConfigured() const = 0;
    virtual QString configurationStatus() const = 0;
    virtual FloorPlanProviderResult generatePlan(const QString& prompt, const FloorPlanConfig* baseConfig = nullptr) = 0;
};

class DeterministicFloorPlanProvider : public IAiFloorPlanProvider {
public:
    DeterministicFloorPlanProvider() = default;
    ~DeterministicFloorPlanProvider() override = default;

    QString id() const override { return QStringLiteral("deterministic"); }
    QString displayName() const override { return QStringLiteral("Built-in Deterministic Engine (Offline / Instant)"); }
    bool isConfigured() const override { return true; }
    QString configurationStatus() const override { return QStringLiteral("Ready (Built-in offline engine)"); }

    FloorPlanProviderResult generatePlan(const QString& prompt, const FloorPlanConfig* baseConfig = nullptr) override;
};

class CloudAiFloorPlanProvider : public QObject, public IAiFloorPlanProvider {
    Q_OBJECT

public:
    explicit CloudAiFloorPlanProvider(QObject* parent = nullptr);
    ~CloudAiFloorPlanProvider() override = default;

    QString id() const override { return QStringLiteral("cloud_ai"); }
    QString displayName() const override { return QStringLiteral("Cloud AI Provider (OpenAI / Custom Gateway)"); }
    bool isConfigured() const override;
    QString configurationStatus() const override;

    FloorPlanProviderResult generatePlan(const QString& prompt, const FloorPlanConfig* baseConfig = nullptr) override;

    // Secure configuration helpers
    static QString resolveApiKey();
    static QString resolveEndpoint();
    static void setSavedApiKey(const QString& key);
    static void setSavedEndpoint(const QString& endpoint);

private:
    QByteArray executeSyncRequest(const QString& url, const QByteArray& postData, const QString& apiKey, int* outStatusCode, QString* outError);

    QNetworkAccessManager m_netManager;
    DeterministicFloorPlanProvider m_fallbackProvider;
};

class AiFloorPlanEngine : public QObject {
    Q_OBJECT

public:
    static AiFloorPlanEngine& instance();

    const std::vector<std::shared_ptr<IAiFloorPlanProvider>>& providers() const { return m_providers; }
    IAiFloorPlanProvider* activeProvider() const;
    void setActiveProvider(const QString& id);

    FloorPlanProviderResult generatePlan(const QString& prompt, const FloorPlanConfig* baseConfig = nullptr);

    // Helpers
    static QString apiKeyStatus();
    static bool hasConfiguredCloudKey();

signals:
    void activeProviderChanged(const QString& id);

private:
    AiFloorPlanEngine();
    ~AiFloorPlanEngine() override = default;

    std::vector<std::shared_ptr<IAiFloorPlanProvider>> m_providers;
    QString m_activeProviderId{QStringLiteral("deterministic")};
};
