#include "ai/AiFloorPlanProvider.h"
#include <QProcessEnvironment>
#include <QSettings>
#include <QEventLoop>
#include <QTimer>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// ============================================================================
// DeterministicFloorPlanProvider
// ============================================================================

FloorPlanProviderResult DeterministicFloorPlanProvider::generatePlan(const QString& prompt, const FloorPlanConfig* baseConfig) {
    FloorPlanProviderResult result;
    result.providerUsed = displayName();

    FloorPlanLayout layout;
    FloorPlanValidationResult valRes;

    bool ok = AiFloorPlanGenerator::generateFromPrompt(prompt, baseConfig, &layout, &valRes);

    if (valRes.needsClarification) {
        result.success = false;
        result.needsClarification = true;
        result.clarificationQuestion = valRes.clarificationQuestion;
        result.clarificationOptions = valRes.clarificationOptions;
        result.errorMessage = valRes.errorString();
        return result;
    }

    if (!ok || !valRes.isValid) {
        result.success = false;
        result.errorMessage = valRes.errorString();
        return result;
    }

    result.success = true;
    result.layout = layout;
    result.proposal = layout.toAiProposal();
    return result;
}

// ============================================================================
// CloudAiFloorPlanProvider
// ============================================================================

CloudAiFloorPlanProvider::CloudAiFloorPlanProvider(QObject* parent)
    : QObject(parent)
{
}

QString CloudAiFloorPlanProvider::resolveApiKey() {
    // 1. Check environment variables
    QString key = qEnvironmentVariable("GENCAD_AI_API_KEY");
    if (!key.isEmpty()) return key.trimmed();

    key = qEnvironmentVariable("OPENAI_API_KEY");
    if (!key.isEmpty()) return key.trimmed();

    key = qEnvironmentVariable("ANTHROPIC_API_KEY");
    if (!key.isEmpty()) return key.trimmed();

    key = qEnvironmentVariable("GEMINI_API_KEY");
    if (!key.isEmpty()) return key.trimmed();

    // 2. Check local application settings (secure non-hardcoded user preference)
    QSettings settings(QStringLiteral("GenCAD"), QStringLiteral("GenCAD"));
    return settings.value(QStringLiteral("Ai/ApiKey")).toString().trimmed();
}

QString CloudAiFloorPlanProvider::resolveEndpoint() {
    QString endpoint = qEnvironmentVariable("GENCAD_AI_ENDPOINT");
    if (!endpoint.isEmpty()) return endpoint.trimmed();

    QSettings settings(QStringLiteral("GenCAD"), QStringLiteral("GenCAD"));
    return settings.value(QStringLiteral("Ai/Endpoint"), QStringLiteral("http://127.0.0.1:8000/api/v1/ai/floorplan")).toString().trimmed();
}

void CloudAiFloorPlanProvider::setSavedApiKey(const QString& key) {
    QSettings settings(QStringLiteral("GenCAD"), QStringLiteral("GenCAD"));
    settings.setValue(QStringLiteral("Ai/ApiKey"), key.trimmed());
}

void CloudAiFloorPlanProvider::setSavedEndpoint(const QString& endpoint) {
    QSettings settings(QStringLiteral("GenCAD"), QStringLiteral("GenCAD"));
    settings.setValue(QStringLiteral("Ai/Endpoint"), endpoint.trimmed());
}

bool CloudAiFloorPlanProvider::isConfigured() const {
    return !resolveApiKey().isEmpty();
}

QString CloudAiFloorPlanProvider::configurationStatus() const {
    if (!qEnvironmentVariable("GENCAD_AI_API_KEY").isEmpty()) {
        return QStringLiteral("Configured via GENCAD_AI_API_KEY env var");
    }
    if (!qEnvironmentVariable("OPENAI_API_KEY").isEmpty()) {
        return QStringLiteral("Configured via OPENAI_API_KEY env var");
    }
    QSettings settings(QStringLiteral("GenCAD"), QStringLiteral("GenCAD"));
    if (!settings.value(QStringLiteral("Ai/ApiKey")).toString().trimmed().isEmpty()) {
        return QStringLiteral("Configured in Application Settings");
    }
    return QStringLiteral("Missing API key (set GENCAD_AI_API_KEY or OPENAI_API_KEY in environment)");
}

QByteArray CloudAiFloorPlanProvider::executeSyncRequest(const QString& urlStr, const QByteArray& postData, const QString& apiKey, int* outStatusCode, QString* outError) {
    QUrl url(urlStr);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!apiKey.isEmpty()) {
        request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());
    }

    QNetworkReply* reply = m_netManager.post(request, postData);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, [&]() {
        if (reply && reply->isRunning()) {
            reply->abort();
        }
        loop.quit();
    });
    timer.start(3500); // 3.5 second timeout

    loop.exec();

    int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (outStatusCode) *outStatusCode = code;

    QByteArray respData = reply->readAll();

    if (reply->error() != QNetworkReply::NoError) {
        if (outError) {
            *outError = QStringLiteral("Network Error (%1): %2").arg(code).arg(reply->errorString());
        }
        reply->deleteLater();
        return QByteArray();
    }

    reply->deleteLater();
    return respData;
}

FloorPlanProviderResult CloudAiFloorPlanProvider::generatePlan(const QString& prompt, const FloorPlanConfig* baseConfig) {
    QString apiKey = resolveApiKey();
    QString endpoint = resolveEndpoint();

    if (apiKey.isEmpty()) {
        // Missing credentials handled gracefully: fall back to deterministic local engine
        FloorPlanProviderResult res = m_fallbackProvider.generatePlan(prompt, baseConfig);
        res.providerUsed = QStringLiteral("Deterministic Engine (Cloud API key missing)");
        return res;
    }

    // Build structured JSON schema generation request
    QJsonObject reqObj;
    reqObj["prompt"] = prompt;
    if (baseConfig) {
        reqObj["config"] = baseConfig->toJson();
    }
    reqObj["target_schema"] = QStringLiteral("https://gencad.org/schemas/v1/floorplan.json");

    int statusCode = 0;
    QString netErr;
    QByteArray resp = executeSyncRequest(endpoint, QJsonDocument(reqObj).toJson(QJsonDocument::Compact), apiKey, &statusCode, &netErr);

    if (statusCode == 200 && !resp.isEmpty()) {
        FloorPlanLayout layout;
        QString parseErr;
        if (FloorPlanLayout::fromJsonString(QString::fromUtf8(resp), &layout, &parseErr)) {
            FloorPlanValidationResult valRes = AiFloorPlanGenerator::validateLayout(layout);
            if (valRes.isValid) {
                FloorPlanProviderResult result;
                result.success = true;
                result.providerUsed = QStringLiteral("Cloud AI Gateway");
                result.layout = layout;
                result.proposal = layout.toAiProposal();
                return result;
            }
        }
    }

    // Network error or schema failure handled gracefully: fall back to deterministic generator
    FloorPlanProviderResult fallbackRes = m_fallbackProvider.generatePlan(prompt, baseConfig);
    fallbackRes.providerUsed = QStringLiteral("Deterministic Engine (Cloud network fallback: %1)").arg(netErr.isEmpty() ? QStringLiteral("HTTP %1").arg(statusCode) : netErr);
    return fallbackRes;
}

// ============================================================================
// AiFloorPlanEngine
// ============================================================================

AiFloorPlanEngine::AiFloorPlanEngine() {
    m_providers.push_back(std::make_shared<DeterministicFloorPlanProvider>());
    m_providers.push_back(std::make_shared<CloudAiFloorPlanProvider>(this));
    m_activeProviderId = QStringLiteral("deterministic");
}

AiFloorPlanEngine& AiFloorPlanEngine::instance() {
    static AiFloorPlanEngine s_instance;
    return s_instance;
}

IAiFloorPlanProvider* AiFloorPlanEngine::activeProvider() const {
    for (const auto& p : m_providers) {
        if (p->id() == m_activeProviderId) {
            return p.get();
        }
    }
    return m_providers.empty() ? nullptr : m_providers.front().get();
}

void AiFloorPlanEngine::setActiveProvider(const QString& id) {
    if (m_activeProviderId != id) {
        m_activeProviderId = id;
        emit activeProviderChanged(id);
    }
}

FloorPlanProviderResult AiFloorPlanEngine::generatePlan(const QString& prompt, const FloorPlanConfig* baseConfig) {
    auto* p = activeProvider();
    if (!p) {
        FloorPlanProviderResult r;
        r.success = false;
        r.errorMessage = QStringLiteral("No AI floor plan provider registered.");
        return r;
    }
    return p->generatePlan(prompt, baseConfig);
}

QString AiFloorPlanEngine::apiKeyStatus() {
    return CloudAiFloorPlanProvider::resolveApiKey().isEmpty()
        ? QStringLiteral("Cloud Key: Missing (Set GENCAD_AI_API_KEY)")
        : QStringLiteral("Cloud Key: Configured");
}

bool AiFloorPlanEngine::hasConfiguredCloudKey() {
    return !CloudAiFloorPlanProvider::resolveApiKey().isEmpty();
}
