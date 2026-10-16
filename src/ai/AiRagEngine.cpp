#include "ai/AiRagEngine.h"
#include <algorithm>

QJsonObject RagQueryResult::toJson() const {
    QJsonObject obj;
    obj["title"] = title;
    obj["category"] = category;
    obj["content"] = content;
    obj["score"] = score;
    return obj;
}

AiRagEngine::AiRagEngine() {
    populateKnowledgeBase();
}

void AiRagEngine::populateKnowledgeBase() {
    m_knowledgeBase.push_back({
        QStringLiteral("GenCAD Drawing Tools Reference"),
        QStringLiteral("Tools"),
        QStringLiteral(
            "GenCAD provides professional 2D drawing tools:\n"
            "• Line (L): Click start point, then end point. Supports Ortho mode and exact coords.\n"
            "• Circle (C): Click center point, then click or type radius.\n"
            "• Rectangle (REC): Click first corner, then opposite corner.\n"
            "• Arc (A): 3-point or Center-Radius-Angles interactive arc creation.\n"
            "• Polyline (PL): Continuous connected series of line segments.\n"
            "• Text: Click position to place text with custom height and rotation.\n"
            "• Dimension (D): Click two points to measure distance, then position the dimension line."
        ),
        { QStringLiteral("line"), QStringLiteral("circle"), QStringLiteral("rectangle"), QStringLiteral("arc"),
          QStringLiteral("polyline"), QStringLiteral("text"), QStringLiteral("dimension"), QStringLiteral("tools"),
          QStringLiteral("draw") }
    });

    m_knowledgeBase.push_back({
        QStringLiteral("GenCAD Modify Tools & Operations"),
        QStringLiteral("Tools"),
        QStringLiteral(
            "Modify commands allow altering existing geometry:\n"
            "• Move (M): Select entities, specify base point and destination point.\n"
            "• Rotate (RO): Select entities, pick rotation center, specify angle in degrees.\n"
            "• Mirror (MI): Select entities, define mirror axis with two points. Can retain or copy.\n"
            "• Copy (CO): Duplicate selected entities to a new location.\n"
            "• Offset (O): Create parallel copies of lines/curves at a specified distance.\n"
            "• Trim (TR): Cut lines back to their intersection with boundary edges.\n"
            "• Extend (EX): Lengthen line segments until they meet a boundary edge."
        ),
        { QStringLiteral("move"), QStringLiteral("rotate"), QStringLiteral("mirror"), QStringLiteral("copy"),
          QStringLiteral("offset"), QStringLiteral("trim"), QStringLiteral("extend"), QStringLiteral("modify") }
    });

    m_knowledgeBase.push_back({
        QStringLiteral("Snapping & Precision Guidelines"),
        QStringLiteral("Drafting"),
        QStringLiteral(
            "Precision in CAD is achieved via Snapping and Ortho mode:\n"
            "• Endpoint Snap: Snaps to the ends of lines, arcs, and polylines.\n"
            "• Midpoint Snap: Snaps to exact geometric centers of line segments.\n"
            "• Center Snap: Snaps to circle and arc centers.\n"
            "• Intersection Snap: Snaps to crossing points of geometric elements.\n"
            "• Grid Snap: Constrains cursor to regular grid spacing (F7 to toggle grid).\n"
            "• Ortho Mode (F8): Constrains mouse motion strictly to horizontal or vertical axes."
        ),
        { QStringLiteral("snap"), QStringLiteral("endpoint"), QStringLiteral("midpoint"), QStringLiteral("intersection"),
          QStringLiteral("ortho"), QStringLiteral("grid"), QStringLiteral("precision") }
    });

    m_knowledgeBase.push_back({
        QStringLiteral("ISO 128 Technical Drawing Standards"),
        QStringLiteral("Standards"),
        QStringLiteral(
            "ISO 128 defines international standards for technical drawings:\n"
            "• Continuous Thick Line: Visible outlines, edges, and crests.\n"
            "• Continuous Thin Line: Dimension lines, projection lines, hatching, leaders.\n"
            "• Dashed Line: Hidden outlines and hidden edges.\n"
            "• Long Dashed Dotted Thin Line: Centerlines, axes of symmetry, trajectory paths.\n"
            "Standard paper sheet sizes follow ISO 216: A0 (841x1189mm), A1 (594x841mm), A2 (420x594mm), A3 (297x420mm), A4 (210x297mm)."
        ),
        { QStringLiteral("iso"), QStringLiteral("iso 128"), QStringLiteral("standards"), QStringLiteral("line weight"),
          QStringLiteral("centerline"), QStringLiteral("hidden"), QStringLiteral("sheet"), QStringLiteral("a4") }
    });

    m_knowledgeBase.push_back({
        QStringLiteral("ANSI / ASME Y14.5 Dimensioning Standards"),
        QStringLiteral("Standards"),
        QStringLiteral(
            "ANSI/ASME Y14.5 governs engineering drawings and tolerancing:\n"
            "• All dimensions must have an associated tolerance (expressed or general title block).\n"
            "• Dimension lines should not cross other dimension lines where possible.\n"
            "• Place dimensions outside the geometry outline with at least 10mm-15mm offset.\n"
            "• Dimension each feature completely once; do not duplicate dimensions on multiple views.\n"
            "• Extension lines should start 1-2mm away from object geometry."
        ),
        { QStringLiteral("ansi"), QStringLiteral("asme"), QStringLiteral("y14.5"), QStringLiteral("dimensioning"),
          QStringLiteral("tolerancing"), QStringLiteral("offset"), QStringLiteral("engineering") }
    });

    m_knowledgeBase.push_back({
        QStringLiteral("AIA CAD Layer Standards"),
        QStringLiteral("Standards"),
        QStringLiteral(
            "The American Institute of Architects (AIA) defines standard CAD layer naming:\n"
            "• A-WALL: Full height architectural interior and exterior walls.\n"
            "• A-DOOR: Doors, frames, hardware, and door swing arcs.\n"
            "• A-GLAZ: Windows, curtain walls, and glazed openings.\n"
            "• A-FLOR: Floor patterns, floor finishes, stairs.\n"
            "• A-ANNO-DIMS: Dimensions and geometric annotations.\n"
            "• A-ANNO-TEXT: Room names, titles, and descriptive text notes.\n"
            "• M-HVAC: Mechanical HVAC ductwork and diffusers.\n"
            "• E-POWR: Electrical power distribution, outlets, panels."
        ),
        { QStringLiteral("aia"), QStringLiteral("layer"), QStringLiteral("layers"), QStringLiteral("standards"),
          QStringLiteral("wall"), QStringLiteral("door"), QStringLiteral("window"), QStringLiteral("architectural") }
    });

    m_knowledgeBase.push_back({
        QStringLiteral("Keyboard Shortcuts & Acceleration"),
        QStringLiteral("Shortcuts"),
        QStringLiteral(
            "Quick access shortcuts in GenCAD:\n"
            "• L - Line Tool\n"
            "• C - Circle Tool\n"
            "• REC - Rectangle Tool\n"
            "• A - Arc Tool\n"
            "• PL - Polyline Tool\n"
            "• M - Move Tool\n"
            "• RO - Rotate Tool\n"
            "• MI - Mirror Tool\n"
            "• CO - Copy Tool\n"
            "• TR - Trim Tool\n"
            "• EX - Extend Tool\n"
            "• D - Dimension Tool\n"
            "• Ctrl+Z - Undo\n"
            "• Ctrl+Y - Redo\n"
            "• Ctrl+S - Save Drawing\n"
            "• F3 - Object Snap On/Off\n"
            "• F7 - Grid On/Off\n"
            "• F8 - Ortho Mode On/Off"
        ),
        { QStringLiteral("shortcuts"), QStringLiteral("keys"), QStringLiteral("hotkeys"), QStringLiteral("keyboard"),
          QStringLiteral("undo"), QStringLiteral("redo"), QStringLiteral("f3"), QStringLiteral("f8") }
    });

    m_knowledgeBase.push_back({
        QStringLiteral("GenCAD Cloud & Version Control"),
        QStringLiteral("Cloud"),
        QStringLiteral(
            "GenCAD Cloud Workspace features:\n"
            "• Secure JWT-based cloud authentication.\n"
            "• Project workspace grouping and multi-drawing collaboration.\n"
            "• Automated version history and commit tracking.\n"
            "• One-click version rollback and drawing restoration.\n"
            "• Cloud backup in native .gencad and industry-standard .dxf formats."
        ),
        { QStringLiteral("cloud"), QStringLiteral("version"), QStringLiteral("backup"), QStringLiteral("sync"),
          QStringLiteral("revert"), QStringLiteral("project"), QStringLiteral("jwt") }
    });
}

double AiRagEngine::calculateScore(const QStringList& queryTokens, const RagArticle& article) const {
    double score = 0.0;
    QString lowerTitle = article.title.toLower();
    QString lowerContent = article.content.toLower();

    for (const auto& token : queryTokens) {
        if (token.length() < 2) continue;

        // Keyword exact match
        for (const auto& kw : article.keywords) {
            if (kw.compare(token, Qt::CaseInsensitive) == 0) {
                score += 5.0;
            } else if (kw.contains(token, Qt::CaseInsensitive)) {
                score += 2.0;
            }
        }

        // Title match
        if (lowerTitle.contains(token)) {
            score += 4.0;
        }

        // Content match
        if (lowerContent.contains(token)) {
            score += 1.0;
        }
    }

    return score;
}

std::vector<RagQueryResult> AiRagEngine::query(const QString& question, int maxResults) const {
    QStringList tokens = question.toLower().split(QRegularExpression(QStringLiteral("[\\s,;?.!/\\-]+")), Qt::SkipEmptyParts);
    std::vector<RagQueryResult> scoredResults;

    for (const auto& article : m_knowledgeBase) {
        double score = calculateScore(tokens, article);
        if (score > 0.0) {
            RagQueryResult res;
            res.title = article.title;
            res.category = article.category;
            res.content = article.content;
            res.score = score;
            scoredResults.push_back(res);
        }
    }

    std::sort(scoredResults.begin(), scoredResults.end(), [](const RagQueryResult& a, const RagQueryResult& b) {
        return a.score > b.score;
    });

    if (static_cast<int>(scoredResults.size()) > maxResults) {
        scoredResults.resize(maxResults);
    }

    return scoredResults;
}

QString AiRagEngine::answerQuestion(const QString& question) const {
    auto results = query(question, 2);
    if (results.empty()) {
        return QStringLiteral(
            "I could not find a specific documentation article matching '%1'.\n"
            "Try asking about drawing tools (Line, Circle, Rectangle, Arc), modify tools (Trim, Extend, Offset), "
            "shortcuts, or standards (ISO 128, ANSI Y14.5, AIA Layers)."
        ).arg(question);
    }

    QString answer;
    answer += QStringLiteral("### %1\n\n%2\n").arg(results[0].title, results[0].content);
    if (results.size() > 1 && results[1].score > 3.0) {
        answer += QStringLiteral("\n\n---\n**Related: %1**\n%2").arg(results[1].title, results[1].content);
    }
    return answer;
}
