#include "ai/AiFloorPlanGenerator.h"
#include <QRegularExpression>
#include <cmath>
#include <algorithm>
#include <set>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

FloorPlanValidationResult AiFloorPlanGenerator::parsePrompt(const QString& prompt, FloorPlanConfig* outConfig) {
    FloorPlanValidationResult res;
    if (!outConfig) {
        res.isValid = false;
        res.errors.append(QStringLiteral("Null configuration pointer provided."));
        return res;
    }

    QString p = prompt.trimmed();
    QString lower = p.toLower();

    // 1. Dimensions Parsing (e.g. "10 m x 12 m", "10x12 m", "10000 x 12000 mm", "30ft x 40ft")
    QRegularExpression dimRe(QStringLiteral(R"((\d+(?:\.\d+)?)\s*(m|mm|ft|meter|meters)?\s*(?:x|by|\*|×)\s*(\d+(?:\.\d+)?)\s*(m|mm|ft|meter|meters)?)"),
                             QRegularExpression::CaseInsensitiveOption);
    auto dimMatch = dimRe.match(lower);

    if (!dimMatch.hasMatch()) {
        // Try labeled pattern: "width 10m ... depth 12m" or "10m wide ... 12m deep"
        QRegularExpression dimRe2(QStringLiteral(R"((?:width|wide)\s*(\d+(?:\.\d+)?)\s*(m|mm|ft)?.*?(?:depth|length|long)\s*(\d+(?:\.\d+)?)\s*(m|mm|ft)?)"),
                                  QRegularExpression::CaseInsensitiveOption);
        dimMatch = dimRe2.match(lower);
    }

    if (!dimMatch.hasMatch()) {
        // AMBIGUITY DETECTED: Never silently invent critical dimensions!
        res.isValid = false;
        res.needsClarification = true;
        res.clarificationQuestion = QStringLiteral(
            "Essential requirement missing: Overall building dimensions are not specified.\n"
            "Please specify building size (e.g. '10m x 12m') or select a standard footprint:"
        );
        res.clarificationOptions = {
            QStringLiteral("10m × 12m (Compact 3-BHK)"),
            QStringLiteral("12m × 15m (Spacious 3-BHK)"),
            QStringLiteral("8m × 10m (2-BHK)"),
            QStringLiteral("15m × 18m (Luxury Villa)")
        };
        return res;
    }

    double wVal = dimMatch.captured(1).toDouble();
    QString wUnit = dimMatch.captured(2).toLower();
    double dVal = dimMatch.captured(3).toDouble();
    QString dUnit = dimMatch.captured(4).toLower();

    // Convert to millimeters
    auto toMillimeters = [](double val, const QString& unit) -> double {
        if (unit == "m" || unit == "meter" || unit == "meters") return val * 1000.0;
        if (unit == "ft" || unit == "feet") return val * 304.8;
        if (unit == "mm") return val;
        // Default assumption: values <= 60 are meters, > 60 are millimeters
        return (val <= 60.0) ? (val * 1000.0) : val;
    };

    outConfig->width = toMillimeters(wVal, wUnit);
    outConfig->depth = toMillimeters(dVal, dUnit);

    // 2. Wall Style: Single-Line vs Double-Line
    if (lower.contains("single-line") || lower.contains("single line") || lower.contains("centerline")) {
        outConfig->wallStyle = FloorPlanWallStyle::SingleLine;
    } else {
        outConfig->wallStyle = FloorPlanWallStyle::DoubleLine;
    }

    // 3. Wall Thicknesses
    QRegularExpression extThickRe(QStringLiteral(R"((\d+(?:\.\d+)?)\s*(mm|m)?\s*(?:exterior|external|outer)\s*wall)"));
    auto extM = extThickRe.match(lower);
    if (extM.hasMatch()) {
        double v = extM.captured(1).toDouble();
        if (extM.captured(2) == "m") v *= 1000.0;
        outConfig->exteriorWallThickness = v;
    } else {
        outConfig->exteriorWallThickness = 200.0;
    }

    QRegularExpression intThickRe(QStringLiteral(R"((\d+(?:\.\d+)?)\s*(mm|m)?\s*(?:interior|internal|inner|partition)\s*wall)"));
    auto intM = intThickRe.match(lower);
    if (intM.hasMatch()) {
        double v = intM.captured(1).toDouble();
        if (intM.captured(2) == "m") v *= 1000.0;
        outConfig->interiorWallThickness = v;
    } else {
        outConfig->interiorWallThickness = 100.0;
    }

    // Generic wall thickness if specified without exterior/interior distinction
    QRegularExpression genThickRe(QStringLiteral(R"(wall\s*thickness\s*(\d+(?:\.\d+)?)\s*(mm|m)?)"));
    auto genM = genThickRe.match(lower);
    if (genM.hasMatch()) {
        double v = genM.captured(1).toDouble();
        if (genM.captured(2) == "m") v *= 1000.0;
        outConfig->exteriorWallThickness = v;
        outConfig->interiorWallThickness = std::max(50.0, v * 0.5);
    }

    // 4. Room Counts & Types
    // Bedrooms
    QRegularExpression bedRe(QStringLiteral(R"((\d+)\s*(?:bed|bedroom|bhk))"));
    auto bedM = bedRe.match(lower);
    if (bedM.hasMatch()) {
        outConfig->bedroomCount = bedM.captured(1).toInt();
    } else if (lower.contains("three bed") || lower.contains("3 bed") || lower.contains("3bhk")) {
        outConfig->bedroomCount = 3;
    } else if (lower.contains("two bed") || lower.contains("2 bed") || lower.contains("2bhk")) {
        outConfig->bedroomCount = 2;
    } else if (lower.contains("four bed") || lower.contains("4 bed") || lower.contains("4bhk")) {
        outConfig->bedroomCount = 4;
    } else if (lower.contains("one bed") || lower.contains("1 bed") || lower.contains("1bhk")) {
        outConfig->bedroomCount = 1;
    } else if (lower.contains("bedroom") || lower.contains("residential") || lower.contains("ground floor")) {
        outConfig->bedroomCount = 3; // Standard default
    }

    // Toilets / Bathrooms
    QRegularExpression toiletRe(QStringLiteral(R"((\d+)\s*(?:toilet|bath|bathroom|restroom|wc))"));
    auto toiletM = toiletRe.match(lower);
    if (toiletM.hasMatch()) {
        outConfig->toiletCount = toiletM.captured(1).toInt();
    } else if (lower.contains("two toilet") || lower.contains("2 toilet") || lower.contains("two bath") || lower.contains("2 bath")) {
        outConfig->toiletCount = 2;
    } else if (lower.contains("one toilet") || lower.contains("1 toilet") || lower.contains("one bath") || lower.contains("1 bath")) {
        outConfig->toiletCount = 1;
    } else if (lower.contains("toilet") || lower.contains("bath")) {
        outConfig->toiletCount = 2;
    }

    // Other functional spaces
    outConfig->hasLivingRoom = lower.contains("living") || lower.contains("hall") || lower.contains("residential") || lower.contains("ground floor");
    outConfig->hasKitchen = lower.contains("kitchen") || lower.contains("residential") || lower.contains("bhk") || lower.contains("ground floor");
    outConfig->hasDining = lower.contains("dining");
    outConfig->hasStairs = lower.contains("stair") || lower.contains("staircase") || lower.contains("steps");

    // Architectural features
    outConfig->hasDoors = !lower.contains("no door");
    outConfig->hasWindows = !lower.contains("no window");
    outConfig->hasDimensions = lower.contains("dimension") || !lower.contains("no dimension");
    outConfig->hasLabels = !lower.contains("no label");

    // Validate the parsed config
    return validateConfig(*outConfig);
}

FloorPlanValidationResult AiFloorPlanGenerator::validateConfig(const FloorPlanConfig& config) {
    FloorPlanValidationResult res;

    if (config.width <= 0.0 || !std::isfinite(config.width)) {
        res.isValid = false;
        res.errors.append(QStringLiteral("Building width must be a positive finite dimension."));
    }
    if (config.depth <= 0.0 || !std::isfinite(config.depth)) {
        res.isValid = false;
        res.errors.append(QStringLiteral("Building depth must be a positive finite dimension."));
    }

    // Wall thickness checks
    if (config.wallStyle == FloorPlanWallStyle::DoubleLine) {
        if (config.exteriorWallThickness <= 0.0 || !std::isfinite(config.exteriorWallThickness)) {
            res.isValid = false;
            res.errors.append(QStringLiteral("Exterior wall thickness must be positive."));
        }
        if (config.interiorWallThickness <= 0.0 || !std::isfinite(config.interiorWallThickness)) {
            res.isValid = false;
            res.errors.append(QStringLiteral("Interior wall thickness must be positive."));
        }
        if (config.exteriorWallThickness * 2.0 >= config.width || config.exteriorWallThickness * 2.0 >= config.depth) {
            res.isValid = false;
            res.errors.append(QStringLiteral("Exterior wall thickness exceeds building dimensions."));
        }
    }

    // Area check for requested room count
    double totalAreaM2 = (config.width * config.depth) / 1e6;
    double minRequiredArea = 15.0; // Minimal 1-room shack
    int totalRooms = config.bedroomCount + (config.hasLivingRoom ? 1 : 0) + (config.hasKitchen ? 1 : 0) + config.toiletCount + (config.hasStairs ? 1 : 0);

    if (totalRooms >= 5) {
        minRequiredArea = 45.0; // At least 45 m² for 3 BHK / 2 toilet house
    } else if (totalRooms >= 3) {
        minRequiredArea = 25.0;
    }

    if (totalAreaM2 < minRequiredArea) {
        res.isValid = false;
        res.errors.append(QStringLiteral(
            "Building footprint (%1m × %2m = %3 m²) is too small for %4 rooms. "
            "Minimum recommended area is at least %5 m²."
        ).arg(config.width / 1000.0, 0, 'f', 1)
         .arg(config.depth / 1000.0, 0, 'f', 1)
         .arg(totalAreaM2, 0, 'f', 1)
         .arg(totalRooms)
         .arg(minRequiredArea, 0, 'f', 0));
    }

    return res;
}

FloorPlanValidationResult AiFloorPlanGenerator::validateLayout(const FloorPlanLayout& layout) {
    FloorPlanValidationResult res = validateConfig(layout.config);
    if (!res.isValid) return res;

    // 1. Room containment within footprint
    QRectF buildingBounds(0, 0, layout.config.width, layout.config.depth);
    for (const auto& room : layout.rooms) {
        if (room.rect.width() <= 0 || room.rect.height() <= 0) {
            res.isValid = false;
            res.errors.append(QStringLiteral("Room '%1' has degenerate non-positive dimensions.").arg(room.name));
        }
        if (!buildingBounds.contains(room.rect.adjusted(1.0, 1.0, -1.0, -1.0))) {
            res.isValid = false;
            res.errors.append(QStringLiteral("Room '%1' extends outside building boundary.").arg(room.name));
        }
    }

    // 2. Room overlap validation
    for (size_t i = 0; i < layout.rooms.size(); ++i) {
        for (size_t j = i + 1; j < layout.rooms.size(); ++j) {
            QRectF r1 = layout.rooms[i].rect.adjusted(1.0, 1.0, -1.0, -1.0);
            QRectF r2 = layout.rooms[j].rect.adjusted(1.0, 1.0, -1.0, -1.0);
            if (r1.intersects(r2)) {
                res.isValid = false;
                res.errors.append(QStringLiteral("Geometric overlap detected between room '%1' and '%2'.")
                                  .arg(layout.rooms[i].name, layout.rooms[j].name));
            }
        }
    }

    // 3. Wall segment coordinates
    for (const auto& w : layout.walls) {
        if (!std::isfinite(w.p1.x()) || !std::isfinite(w.p1.y()) ||
            !std::isfinite(w.p2.x()) || !std::isfinite(w.p2.y())) {
            res.isValid = false;
            res.errors.append(QStringLiteral("Wall segment contains invalid NaN or infinite coordinates."));
            break;
        }
    }

    // 4. Doors validation
    for (const auto& d : layout.doors) {
        if (d.width <= 0.0 || !std::isfinite(d.width)) {
            res.isValid = false;
            res.errors.append(QStringLiteral("Door '%1' has invalid non-positive width.").arg(d.id));
        }
        if (!buildingBounds.adjusted(-100, -100, 100, 100).contains(d.hinge)) {
            res.isValid = false;
            res.errors.append(QStringLiteral("Door '%1' hinge lies outside building boundaries.").arg(d.id));
        }
    }

    // 5. Windows validation
    for (const auto& win : layout.windows) {
        if (win.width <= 0.0 || !std::isfinite(win.width)) {
            res.isValid = false;
            res.errors.append(QStringLiteral("Window '%1' has invalid non-positive width.").arg(win.id));
        }
    }

    return res;
}

FloorPlanLayout AiFloorPlanGenerator::generate(const FloorPlanConfig& config) {
    FloorPlanLayout layout;
    layout.config = config;
    layout.totalAreaSqM = (config.width * config.depth) / 1e6;

    double W = config.width;
    double H = config.depth;

    double extThick = (config.wallStyle == FloorPlanWallStyle::DoubleLine) ? config.exteriorWallThickness : 0.0;
    double xMin = extThick;
    double yMin = extThick;
    double xMax = W - extThick;
    double yMax = H - extThick;

    double usableW = xMax - xMin;
    double usableH = yMax - yMin;

    // Architectural zoning:
    // Split into Front Zone (Public: Living, Kitchen, Stairs) and Rear Zone (Private: Bedrooms, Bathrooms)
    double ySplit = yMin + usableH * 0.48;

    std::vector<FloorPlanRoomSpec>& rooms = layout.rooms;

    // ----------------------------------------------------
    // FRONT ZONE (Y: yMin to ySplit)
    // ----------------------------------------------------
    double frontH = ySplit - yMin;

    if (config.hasStairs) {
        double livingW = usableW * 0.58;
        double sideW = usableW - livingW;

        // 1. Living Room
        FloorPlanRoomSpec living;
        living.id = QStringLiteral("room_living");
        living.name = QStringLiteral("LIVING ROOM");
        living.type = FloorPlanRoomType::LivingRoom;
        living.rect = QRectF(xMin, yMin, livingW, frontH);
        living.areaSqM = (living.rect.width() * living.rect.height()) / 1e6;
        rooms.push_back(living);

        // 2. Staircase (Bottom right front)
        double stairsH = frontH * 0.52;
        FloorPlanRoomSpec stairs;
        stairs.id = QStringLiteral("room_stairs");
        stairs.name = QStringLiteral("STAIRCASE");
        stairs.type = FloorPlanRoomType::Staircase;
        stairs.rect = QRectF(xMin + livingW, yMin, sideW, stairsH);
        stairs.areaSqM = (stairs.rect.width() * stairs.rect.height()) / 1e6;
        rooms.push_back(stairs);

        // 3. Kitchen (Top right front)
        double kitchenH = frontH - stairsH;
        FloorPlanRoomSpec kitchen;
        kitchen.id = QStringLiteral("room_kitchen");
        kitchen.name = QStringLiteral("KITCHEN");
        kitchen.type = FloorPlanRoomType::Kitchen;
        kitchen.rect = QRectF(xMin + livingW, yMin + stairsH, sideW, kitchenH);
        kitchen.areaSqM = (kitchen.rect.width() * kitchen.rect.height()) / 1e6;
        rooms.push_back(kitchen);
    } else {
        double livingW = usableW * 0.60;
        double kitchenW = usableW - livingW;

        FloorPlanRoomSpec living;
        living.id = QStringLiteral("room_living");
        living.name = QStringLiteral("LIVING ROOM");
        living.type = FloorPlanRoomType::LivingRoom;
        living.rect = QRectF(xMin, yMin, livingW, frontH);
        living.areaSqM = (living.rect.width() * living.rect.height()) / 1e6;
        rooms.push_back(living);

        FloorPlanRoomSpec kitchen;
        kitchen.id = QStringLiteral("room_kitchen");
        kitchen.name = QStringLiteral("KITCHEN & DINING");
        kitchen.type = FloorPlanRoomType::Kitchen;
        kitchen.rect = QRectF(xMin + livingW, yMin, kitchenW, frontH);
        kitchen.areaSqM = (kitchen.rect.width() * kitchen.rect.height()) / 1e6;
        rooms.push_back(kitchen);
    }

    // ----------------------------------------------------
    // REAR ZONE (Y: ySplit to yMax)
    // ----------------------------------------------------
    double rearH = yMax - ySplit;

    if (config.bedroomCount >= 3) {
        // 3 Bedrooms + 2 Toilets configuration
        double bay1W = usableW * 0.38;
        double bay2W = usableW * 0.28;
        double bay3W = usableW - bay1W - bay2W;

        double toiletH = std::min(rearH * 0.38, 2200.0);
        double bedH = rearH - toiletH;

        // Bay 1: Master Bedroom (rear) + Attached Toilet (front of rear zone)
        FloorPlanRoomSpec toilet1;
        toilet1.id = QStringLiteral("room_toilet1");
        toilet1.name = QStringLiteral("TOILET 1");
        toilet1.type = FloorPlanRoomType::Toilet;
        toilet1.rect = QRectF(xMin, ySplit, bay1W * 0.60, toiletH);
        toilet1.areaSqM = (toilet1.rect.width() * toilet1.rect.height()) / 1e6;
        rooms.push_back(toilet1);

        FloorPlanRoomSpec corridor1;
        corridor1.id = QStringLiteral("room_corridor");
        corridor1.name = QStringLiteral("LOBBY / PASSAGE");
        corridor1.type = FloorPlanRoomType::Corridor;
        corridor1.rect = QRectF(xMin + bay1W * 0.60, ySplit, bay1W * 0.40, toiletH);
        corridor1.areaSqM = (corridor1.rect.width() * corridor1.rect.height()) / 1e6;
        rooms.push_back(corridor1);

        FloorPlanRoomSpec masterBed;
        masterBed.id = QStringLiteral("room_master_bed");
        masterBed.name = QStringLiteral("MASTER BEDROOM");
        masterBed.type = FloorPlanRoomType::MasterBedroom;
        masterBed.rect = QRectF(xMin, ySplit + toiletH, bay1W, bedH);
        masterBed.areaSqM = (masterBed.rect.width() * masterBed.rect.height()) / 1e6;
        rooms.push_back(masterBed);

        // Bay 2: Common Toilet (front) + Bedroom 3 (rear)
        FloorPlanRoomSpec toilet2;
        toilet2.id = QStringLiteral("room_toilet2");
        toilet2.name = QStringLiteral("TOILET 2");
        toilet2.type = FloorPlanRoomType::Toilet;
        toilet2.rect = QRectF(xMin + bay1W, ySplit, bay2W, toiletH);
        toilet2.areaSqM = (toilet2.rect.width() * toilet2.rect.height()) / 1e6;
        rooms.push_back(toilet2);

        FloorPlanRoomSpec bed3;
        bed3.id = QStringLiteral("room_bed3");
        bed3.name = QStringLiteral("BEDROOM 3");
        bed3.type = FloorPlanRoomType::Bedroom;
        bed3.rect = QRectF(xMin + bay1W, ySplit + toiletH, bay2W, bedH);
        bed3.areaSqM = (bed3.rect.width() * bed3.rect.height()) / 1e6;
        rooms.push_back(bed3);

        // Bay 3: Bedroom 2 (spacious full-depth bedroom)
        FloorPlanRoomSpec bed2;
        bed2.id = QStringLiteral("room_bed2");
        bed2.name = QStringLiteral("BEDROOM 2");
        bed2.type = FloorPlanRoomType::Bedroom;
        bed2.rect = QRectF(xMin + bay1W + bay2W, ySplit, bay3W, rearH);
        bed2.areaSqM = (bed2.rect.width() * bed2.rect.height()) / 1e6;
        rooms.push_back(bed2);
    } else if (config.bedroomCount == 2) {
        // 2 Bedrooms + 2 Toilets configuration
        double bay1W = usableW * 0.50;
        double bay2W = usableW - bay1W;
        double toiletH = std::min(rearH * 0.38, 2000.0);
        double bedH = rearH - toiletH;

        // Master Bedroom + Toilet 1
        FloorPlanRoomSpec toilet1;
        toilet1.id = QStringLiteral("room_toilet1");
        toilet1.name = QStringLiteral("TOILET 1");
        toilet1.type = FloorPlanRoomType::Toilet;
        toilet1.rect = QRectF(xMin, ySplit, bay1W * 0.50, toiletH);
        toilet1.areaSqM = (toilet1.rect.width() * toilet1.rect.height()) / 1e6;
        rooms.push_back(toilet1);

        FloorPlanRoomSpec masterBed;
        masterBed.id = QStringLiteral("room_master_bed");
        masterBed.name = QStringLiteral("MASTER BEDROOM");
        masterBed.type = FloorPlanRoomType::MasterBedroom;
        masterBed.rect = QRectF(xMin, ySplit + toiletH, bay1W, bedH);
        masterBed.areaSqM = (masterBed.rect.width() * masterBed.rect.height()) / 1e6;
        rooms.push_back(masterBed);

        // Bedroom 2 + Common Toilet 2
        FloorPlanRoomSpec toilet2;
        toilet2.id = QStringLiteral("room_toilet2");
        toilet2.name = QStringLiteral("TOILET 2");
        toilet2.type = FloorPlanRoomType::Toilet;
        toilet2.rect = QRectF(xMin + bay1W, ySplit, bay2W * 0.50, toiletH);
        toilet2.areaSqM = (toilet2.rect.width() * toilet2.rect.height()) / 1e6;
        rooms.push_back(toilet2);

        FloorPlanRoomSpec bed2;
        bed2.id = QStringLiteral("room_bed2");
        bed2.name = QStringLiteral("BEDROOM 2");
        bed2.type = FloorPlanRoomType::Bedroom;
        bed2.rect = QRectF(xMin + bay1W, ySplit + toiletH, bay2W, bedH);
        bed2.areaSqM = (bed2.rect.width() * bed2.rect.height()) / 1e6;
        rooms.push_back(bed2);
    } else {
        // 1 Bedroom + Toilet
        double toiletW = std::min(usableW * 0.35, 2200.0);
        double bedW = usableW - toiletW;

        FloorPlanRoomSpec toilet1;
        toilet1.id = QStringLiteral("room_toilet1");
        toilet1.name = QStringLiteral("BATHROOM");
        toilet1.type = FloorPlanRoomType::Toilet;
        toilet1.rect = QRectF(xMin, ySplit, toiletW, rearH);
        toilet1.areaSqM = (toilet1.rect.width() * toilet1.rect.height()) / 1e6;
        rooms.push_back(toilet1);

        FloorPlanRoomSpec bed1;
        bed1.id = QStringLiteral("room_bed1");
        bed1.name = QStringLiteral("BEDROOM");
        bed1.type = FloorPlanRoomType::Bedroom;
        bed1.rect = QRectF(xMin + toiletW, ySplit, bedW, rearH);
        bed1.areaSqM = (bed1.rect.width() * bed1.rect.height()) / 1e6;
        rooms.push_back(bed1);
    }

    // ----------------------------------------------------
    // WALL GENERATION
    // ----------------------------------------------------
    if (config.wallStyle == FloorPlanWallStyle::SingleLine) {
        generateSingleLineWalls(config, rooms, layout.walls);
    } else {
        generateDoubleLineWalls(config, rooms, layout.walls);
    }

    // ----------------------------------------------------
    // DOORS GENERATION (Doors with 90° Swing Arcs)
    // ----------------------------------------------------
    if (config.hasDoors) {
        generateDoors(config, rooms, layout.doors);
    }

    // ----------------------------------------------------
    // WINDOWS GENERATION
    // ----------------------------------------------------
    if (config.hasWindows) {
        generateWindows(config, rooms, layout.windows);
    }

    // ----------------------------------------------------
    // STAIRS GENERATION
    // ----------------------------------------------------
    if (config.hasStairs) {
        for (const auto& r : rooms) {
            if (r.type == FloorPlanRoomType::Staircase) {
                generateStairs(config, r, layout.stairs);
                break;
            }
        }
    }

    // ----------------------------------------------------
    // ROOM LABELS GENERATION
    // ----------------------------------------------------
    if (config.hasLabels) {
        generateLabels(config, rooms, layout.labels);
    }

    // ----------------------------------------------------
    // DIMENSIONS GENERATION
    // ----------------------------------------------------
    if (config.hasDimensions) {
        generateDimensions(config, rooms, layout.dimensions);
    }

    return layout;
}

void AiFloorPlanGenerator::generateSingleLineWalls(const FloorPlanConfig& config,
                                                   const std::vector<FloorPlanRoomSpec>& rooms,
                                                   std::vector<FloorPlanWallSegment>& outWalls) {
    double W = config.width;
    double H = config.depth;

    // 1. Exterior Perimeter Walls
    outWalls.push_back({ QPointF(0, 0), QPointF(W, 0), 0.0, true, QStringLiteral("A-WALL") });
    outWalls.push_back({ QPointF(W, 0), QPointF(W, H), 0.0, true, QStringLiteral("A-WALL") });
    outWalls.push_back({ QPointF(W, H), QPointF(0, H), 0.0, true, QStringLiteral("A-WALL") });
    outWalls.push_back({ QPointF(0, H), QPointF(0, 0), 0.0, true, QStringLiteral("A-WALL") });

    // 2. Interior Walls along room edges (deduplicated)
    struct SegKey {
        int x1, y1, x2, y2;
        bool operator<(const SegKey& o) const {
            if (x1 != o.x1) return x1 < o.x1;
            if (y1 != o.y1) return y1 < o.y1;
            if (x2 != o.x2) return x2 < o.x2;
            return y2 < o.y2;
        }
    };
    std::set<SegKey> seen;

    auto addInteriorSeg = [&](const QPointF& p1, const QPointF& p2) {
        int ix1 = static_cast<int>(std::round(p1.x()));
        int iy1 = static_cast<int>(std::round(p1.y()));
        int ix2 = static_cast<int>(std::round(p2.x()));
        int iy2 = static_cast<int>(std::round(p2.y()));

        if (ix1 > ix2 || (ix1 == ix2 && iy1 > iy2)) {
            std::swap(ix1, ix2);
            std::swap(iy1, iy2);
        }

        // Ignore segments lying on exterior perimeter
        bool onLeft = (ix1 == 0 && ix2 == 0);
        bool onRight = (ix1 == static_cast<int>(W) && ix2 == static_cast<int>(W));
        bool onBottom = (iy1 == 0 && iy2 == 0);
        bool onTop = (iy1 == static_cast<int>(H) && iy2 == static_cast<int>(H));

        if (onLeft || onRight || onBottom || onTop) return;

        SegKey key{ix1, iy1, ix2, iy2};
        if (seen.find(key) == seen.end()) {
            seen.insert(key);
            outWalls.push_back({ p1, p2, 0.0, false, QStringLiteral("A-WALL") });
        }
    };

    for (const auto& r : rooms) {
        addInteriorSeg(r.rect.topLeft(), r.rect.topRight());
        addInteriorSeg(r.rect.topRight(), r.rect.bottomRight());
        addInteriorSeg(r.rect.bottomRight(), r.rect.bottomLeft());
        addInteriorSeg(r.rect.bottomLeft(), r.rect.topLeft());
    }
}

void AiFloorPlanGenerator::generateDoubleLineWalls(const FloorPlanConfig& config,
                                                   const std::vector<FloorPlanRoomSpec>& rooms,
                                                   std::vector<FloorPlanWallSegment>& outWalls) {
    double W = config.width;
    double H = config.depth;
    double extT = config.exteriorWallThickness;
    double intT = config.interiorWallThickness;

    // 1. Exterior Outer Perimeter
    outWalls.push_back({ QPointF(0, 0), QPointF(W, 0), extT, true, QStringLiteral("A-WALL") });
    outWalls.push_back({ QPointF(W, 0), QPointF(W, H), extT, true, QStringLiteral("A-WALL") });
    outWalls.push_back({ QPointF(W, H), QPointF(0, H), extT, true, QStringLiteral("A-WALL") });
    outWalls.push_back({ QPointF(0, H), QPointF(0, 0), extT, true, QStringLiteral("A-WALL") });

    // 2. Exterior Inner Perimeter
    outWalls.push_back({ QPointF(extT, extT), QPointF(W - extT, extT), extT, true, QStringLiteral("A-WALL") });
    outWalls.push_back({ QPointF(W - extT, extT), QPointF(W - extT, H - extT), extT, true, QStringLiteral("A-WALL") });
    outWalls.push_back({ QPointF(W - extT, H - extT), QPointF(extT, H - extT), extT, true, QStringLiteral("A-WALL") });
    outWalls.push_back({ QPointF(extT, H - extT), QPointF(extT, extT), extT, true, QStringLiteral("A-WALL") });

    // 3. Interior Walls: generated from room boundaries
    struct SegKey {
        int x1, y1, x2, y2;
        bool operator<(const SegKey& o) const {
            if (x1 != o.x1) return x1 < o.x1;
            if (y1 != o.y1) return y1 < o.y1;
            if (x2 != o.x2) return x2 < o.x2;
            return y2 < o.y2;
        }
    };
    std::set<SegKey> seen;

    auto addInteriorWall = [&](const QPointF& p1, const QPointF& p2, bool isHorizontal) {
        int ix1 = static_cast<int>(std::round(p1.x()));
        int iy1 = static_cast<int>(std::round(p1.y()));
        int ix2 = static_cast<int>(std::round(p2.x()));
        int iy2 = static_cast<int>(std::round(p2.y()));

        if (ix1 > ix2 || (ix1 == ix2 && iy1 > iy2)) {
            std::swap(ix1, ix2);
            std::swap(iy1, iy2);
        }

        // Exclude segments on inner exterior perimeter
        int eT = static_cast<int>(std::round(extT));
        int eW = static_cast<int>(std::round(W - extT));
        int eH = static_cast<int>(std::round(H - extT));

        if (isHorizontal) {
            if (iy1 <= eT || iy1 >= eH) return;
        } else {
            if (ix1 <= eT || ix1 >= eW) return;
        }

        SegKey key{ix1, iy1, ix2, iy2};
        if (seen.find(key) == seen.end()) {
            seen.insert(key);

            double halfT = intT * 0.5;
            if (isHorizontal) {
                // Two parallel horizontal lines
                outWalls.push_back({ QPointF(p1.x(), p1.y() - halfT), QPointF(p2.x(), p2.y() - halfT), intT, false, QStringLiteral("A-WALL") });
                outWalls.push_back({ QPointF(p1.x(), p1.y() + halfT), QPointF(p2.x(), p2.y() + halfT), intT, false, QStringLiteral("A-WALL") });
            } else {
                // Two parallel vertical lines
                outWalls.push_back({ QPointF(p1.x() - halfT, p1.y()), QPointF(p2.x() - halfT, p2.y()), intT, false, QStringLiteral("A-WALL") });
                outWalls.push_back({ QPointF(p1.x() + halfT, p1.y()), QPointF(p2.x() + halfT, p2.y()), intT, false, QStringLiteral("A-WALL") });
            }
        }
    };

    for (const auto& r : rooms) {
        addInteriorWall(r.rect.topLeft(), r.rect.topRight(), true);
        addInteriorWall(r.rect.topRight(), r.rect.bottomRight(), false);
        addInteriorWall(r.rect.bottomLeft(), r.rect.bottomRight(), true);
        addInteriorWall(r.rect.topLeft(), r.rect.bottomLeft(), false);
    }
}

void AiFloorPlanGenerator::generateDoors(const FloorPlanConfig& config,
                                        const std::vector<FloorPlanRoomSpec>& rooms,
                                        std::vector<FloorPlanDoorSpec>& outDoors) {
    int doorIdx = 1;
    double dw = config.doorWidth;

    for (const auto& r : rooms) {
        if (r.type == FloorPlanRoomType::Corridor || r.type == FloorPlanRoomType::Staircase) {
            continue; // Circulation rooms don't need entry doors
        }

        FloorPlanDoorSpec door;
        door.id = QStringLiteral("door_%1").arg(doorIdx++);
        door.targetRoom = r.name;
        door.layer = QStringLiteral("A-DOOR");

        double effectiveW = (r.type == FloorPlanRoomType::Toilet) ? std::min(750.0, dw) : dw;
        door.width = effectiveW;

        if (r.type == FloorPlanRoomType::LivingRoom) {
            // Main Entrance Door (South exterior wall, opening inward)
            door.hinge = QPointF(r.rect.left() + 800.0, r.rect.bottom());
            door.strike = QPointF(door.hinge.x() + effectiveW, door.hinge.y());
            door.swingCenter = door.hinge;
            door.swingRadius = effectiveW;
            door.startAngleDeg = 0.0;
            door.sweepAngleDeg = 90.0;
        } else if (r.type == FloorPlanRoomType::Kitchen) {
            // Kitchen Door (West wall shared with Living Room)
            door.hinge = QPointF(r.rect.left(), r.rect.bottom() - 200.0);
            door.strike = QPointF(door.hinge.x() + effectiveW, door.hinge.y());
            door.swingCenter = door.hinge;
            door.swingRadius = effectiveW;
            door.startAngleDeg = 0.0;
            door.sweepAngleDeg = 90.0;
        } else {
            // Bedrooms & Toilets: Access door along bottom horizontal wall (facing circulation lobby)
            door.hinge = QPointF(r.rect.left() + 150.0, r.rect.top());
            door.strike = QPointF(door.hinge.x(), door.hinge.y() + effectiveW);
            door.swingCenter = door.hinge;
            door.swingRadius = effectiveW;
            door.startAngleDeg = 270.0;
            door.sweepAngleDeg = 90.0;
        }

        outDoors.push_back(door);
    }
}

void AiFloorPlanGenerator::generateWindows(const FloorPlanConfig& config,
                                          const std::vector<FloorPlanRoomSpec>& rooms,
                                          std::vector<FloorPlanWindowSpec>& outWindows) {
    int winIdx = 1;
    double ww = config.windowWidth;

    for (const auto& r : rooms) {
        if (r.type == FloorPlanRoomType::Toilet || r.type == FloorPlanRoomType::Corridor) {
            continue;
        }

        FloorPlanWindowSpec win;
        win.id = QStringLiteral("win_%1").arg(winIdx++);
        win.width = ww;
        win.layer = QStringLiteral("A-GLAZ");

        if (r.type == FloorPlanRoomType::LivingRoom) {
            // Large window on front exterior wall
            double cx = r.rect.center().x();
            win.p1 = QPointF(cx - ww * 0.5, 0.0);
            win.p2 = QPointF(cx + ww * 0.5, 0.0);
            win.wallOrientation = QStringLiteral("south");
            outWindows.push_back(win);
        } else if (r.type == FloorPlanRoomType::Kitchen) {
            // Kitchen window on right exterior wall
            double cy = r.rect.center().y();
            win.p1 = QPointF(config.width, cy - ww * 0.5);
            win.p2 = QPointF(config.width, cy + ww * 0.5);
            win.wallOrientation = QStringLiteral("east");
            outWindows.push_back(win);
        } else if (r.type == FloorPlanRoomType::MasterBedroom) {
            // Master bedroom window on top rear exterior wall
            double cx = r.rect.center().x();
            win.p1 = QPointF(cx - ww * 0.5, config.depth);
            win.p2 = QPointF(cx + ww * 0.5, config.depth);
            win.wallOrientation = QStringLiteral("north");
            outWindows.push_back(win);
        } else if (r.type == FloorPlanRoomType::Bedroom) {
            // Bedroom window on rear or side exterior wall
            if (std::abs(r.rect.right() - (config.width - config.exteriorWallThickness)) < 50.0) {
                double cy = r.rect.center().y();
                win.p1 = QPointF(config.width, cy - ww * 0.5);
                win.p2 = QPointF(config.width, cy + ww * 0.5);
                win.wallOrientation = QStringLiteral("east");
            } else {
                double cx = r.rect.center().x();
                win.p1 = QPointF(cx - ww * 0.5, config.depth);
                win.p2 = QPointF(cx + ww * 0.5, config.depth);
                win.wallOrientation = QStringLiteral("north");
            }
            outWindows.push_back(win);
        }
    }
}

void AiFloorPlanGenerator::generateStairs(const FloorPlanConfig& /*config*/,
                                         const FloorPlanRoomSpec& stairRoom,
                                         std::vector<FloorPlanStairSpec>& outStairs) {
    FloorPlanStairSpec st;
    st.id = QStringLiteral("stair_flight_1");
    // Inset stairs inside room slightly for stringer clearance
    st.bounds = stairRoom.rect.adjusted(100.0, 100.0, -100.0, -100.0);
    st.stepCount = 14;
    st.direction = QStringLiteral("UP");
    st.layer = QStringLiteral("A-FLOR-STRS");
    outStairs.push_back(st);
}

void AiFloorPlanGenerator::generateLabels(const FloorPlanConfig& /*config*/,
                                         const std::vector<FloorPlanRoomSpec>& rooms,
                                         std::vector<FloorPlanLabelSpec>& outLabels) {
    for (const auto& r : rooms) {
        FloorPlanLabelSpec lbl;
        lbl.layer = QStringLiteral("A-ANNO-TEXT");
        lbl.height = (r.rect.width() < 2500.0 || r.rect.height() < 2500.0) ? 140.0 : 180.0;

        QString text = QStringLiteral("%1\n%2m x %3m\n%4 m²")
                       .arg(r.name)
                       .arg(r.rect.width() / 1000.0, 0, 'f', 2)
                       .arg(r.rect.height() / 1000.0, 0, 'f', 2)
                       .arg(r.areaSqM, 0, 'f', 1);

        lbl.text = text;
        // Position centered inside the room
        lbl.position = QPointF(r.rect.center().x() - 500.0, r.rect.center().y());
        outLabels.push_back(lbl);
    }
}

void AiFloorPlanGenerator::generateDimensions(const FloorPlanConfig& config,
                                             const std::vector<FloorPlanRoomSpec>& rooms,
                                             std::vector<FloorPlanDimensionSpec>& outDimensions) {
    double W = config.width;
    double H = config.depth;
    double offset = 700.0; // 700 mm offset for dimension line

    // 1. Overall Exterior Width (Bottom)
    FloorPlanDimensionSpec dimWBottom;
    dimWBottom.p1 = QPointF(0, 0);
    dimWBottom.p2 = QPointF(W, 0);
    dimWBottom.dimPoint = QPointF(W * 0.5, -offset);
    dimWBottom.label = QStringLiteral("%1 m").arg(W / 1000.0, 0, 'f', 2);
    dimWBottom.layer = QStringLiteral("A-ANNO-DIMS");
    outDimensions.push_back(dimWBottom);

    // 2. Overall Exterior Depth (Left)
    FloorPlanDimensionSpec dimDLeft;
    dimDLeft.p1 = QPointF(0, 0);
    dimDLeft.p2 = QPointF(0, H);
    dimDLeft.dimPoint = QPointF(-offset, H * 0.5);
    dimDLeft.label = QStringLiteral("%1 m").arg(H / 1000.0, 0, 'f', 2);
    dimDLeft.layer = QStringLiteral("A-ANNO-DIMS");
    outDimensions.push_back(dimDLeft);

    // 3. Top Width Dimension (Top)
    FloorPlanDimensionSpec dimWTop;
    dimWTop.p1 = QPointF(0, H);
    dimWTop.p2 = QPointF(W, H);
    dimWTop.dimPoint = QPointF(W * 0.5, H + offset);
    dimWTop.label = QStringLiteral("%1 m").arg(W / 1000.0, 0, 'f', 2);
    dimWTop.layer = QStringLiteral("A-ANNO-DIMS");
    outDimensions.push_back(dimWTop);

    // 4. Right Depth Dimension (Right)
    FloorPlanDimensionSpec dimDRight;
    dimDRight.p1 = QPointF(W, 0);
    dimDRight.p2 = QPointF(W, H);
    dimDRight.dimPoint = QPointF(W + offset, H * 0.5);
    dimDRight.label = QStringLiteral("%1 m").arg(H / 1000.0, 0, 'f', 2);
    dimDRight.layer = QStringLiteral("A-ANNO-DIMS");
    outDimensions.push_back(dimDRight);

    // 5. Living Room Interior Dimensions
    for (const auto& r : rooms) {
        if (r.type == FloorPlanRoomType::LivingRoom) {
            FloorPlanDimensionSpec dimLivingW;
            dimLivingW.p1 = r.rect.bottomLeft();
            dimLivingW.p2 = r.rect.bottomRight();
            dimLivingW.dimPoint = QPointF(r.rect.center().x(), r.rect.bottom() + 300.0);
            dimLivingW.label = QStringLiteral("%1 m").arg(r.rect.width() / 1000.0, 0, 'f', 2);
            dimLivingW.layer = QStringLiteral("A-ANNO-DIMS");
            outDimensions.push_back(dimLivingW);
            break;
        }
    }
}

bool AiFloorPlanGenerator::generateFromPrompt(const QString& prompt,
                                             const FloorPlanConfig* baseConfig,
                                             FloorPlanLayout* outLayout,
                                             FloorPlanValidationResult* outValidation) {
    if (!outLayout) return false;

    FloorPlanConfig config;
    if (baseConfig) {
        config = *baseConfig;
    }

    FloorPlanValidationResult parseRes = parsePrompt(prompt, &config);
    if (outValidation) {
        *outValidation = parseRes;
    }

    if (!parseRes.isValid) {
        return false;
    }

    FloorPlanLayout layout = generate(config);
    FloorPlanValidationResult valRes = validateLayout(layout);
    if (outValidation) {
        *outValidation = valRes;
    }

    if (!valRes.isValid) {
        return false;
    }

    *outLayout = layout;
    return true;
}
