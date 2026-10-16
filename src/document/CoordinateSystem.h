#pragma once

#include <QPointF>
#include <QRectF>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class CoordinateSystem {
public:
    static double distance(const QPointF& p1, const QPointF& p2) {
        return std::hypot(p2.x() - p1.x(), p2.y() - p1.y());
    }

    static double angleDeg(const QPointF& from, const QPointF& to) {
        double dx = to.x() - from.x();
        double dy = to.y() - from.y();
        double deg = std::atan2(dy, dx) * 180.0 / M_PI;
        if (deg < 0.0) deg += 360.0;
        return deg;
    }

    static double angleRad(const QPointF& from, const QPointF& to) {
        double dx = to.x() - from.x();
        double dy = to.y() - from.y();
        double rad = std::atan2(dy, dx);
        if (rad < 0.0) rad += 2.0 * M_PI;
        return rad;
    }

    static QPointF polarPoint(const QPointF& origin, double distance, double angleDeg) {
        double rad = angleDeg * M_PI / 180.0;
        return QPointF(origin.x() + distance * std::cos(rad),
                       origin.y() + distance * std::sin(rad));
    }

    static double normalizeAngleDeg(double deg) {
        double d = std::fmod(deg, 360.0);
        if (d < 0.0) d += 360.0;
        return d;
    }

    static bool isOrthoHorizontal(const QPointF& base, const QPointF& current) {
        double dx = current.x() - base.x();
        double dy = current.y() - base.y();
        return std::abs(dx) >= std::abs(dy);
    }

    static double orthoAngleDeg(const QPointF& base, const QPointF& current) {
        double dx = current.x() - base.x();
        double dy = current.y() - base.y();
        if (std::abs(dx) >= std::abs(dy)) {
            return (dx >= 0.0) ? 0.0 : 180.0;
        } else {
            return (dy >= 0.0) ? 90.0 : 270.0;
        }
    }

    static double constrainToOrthoAngle(double angleDeg) {
        double norm = normalizeAngleDeg(angleDeg);
        double nearest = std::round(norm / 90.0) * 90.0;
        if (nearest >= 360.0) nearest -= 360.0;
        return nearest;
    }

    static QPointF snapToOrtho(const QPointF& base, const QPointF& current) {
        double dx = current.x() - base.x();
        double dy = current.y() - base.y();
        if (std::abs(dx) >= std::abs(dy)) {
            return QPointF(current.x(), base.y());
        } else {
            return QPointF(base.x(), current.y());
        }
    }

    static QPointF snapToPolar(const QPointF& base, const QPointF& current, double angleIncrementDeg, double toleranceDeg, bool* snapped = nullptr) {
        if (snapped) *snapped = false;
        if (angleIncrementDeg <= 0.01) return current;

        double dist = distance(base, current);
        if (dist < 1e-4) return current;

        double currentAngle = angleDeg(base, current);
        double nearestMultiple = std::round(currentAngle / angleIncrementDeg) * angleIncrementDeg;
        while (nearestMultiple >= 360.0) nearestMultiple -= 360.0;
        while (nearestMultiple < 0.0) nearestMultiple += 360.0;

        double diff = std::abs(currentAngle - nearestMultiple);
        if (diff > 180.0) diff = 360.0 - diff;

        if (diff <= toleranceDeg) {
            if (snapped) *snapped = true;
            return polarPoint(base, dist, nearestMultiple);
        }

        return current;
    }
};
