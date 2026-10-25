#include "SheetMetalConnectorMetadata.h"

#include <TCollection_AsciiString.hxx>
#include <TDF_Tool.hxx>
#include <Precision.hxx>
#include <gp_Vec.hxx>

#include <QMap>
#include <QStringList>

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
QString labelEntry(const TDF_Label& label)
{
    if (label.IsNull()) {
        return {};
    }

    TCollection_AsciiString entry;
    TDF_Tool::Entry(label, entry);
    return QString::fromLatin1(entry.ToCString());
}

QString connectionText(const SheetMetalConnectionEnd& connection)
{
    return QStringLiteral("%1,%2,%3,%4,%5,%6,%7,%8,%9,%10")
        .arg(connection.position.X(), 0, 'g', 12)
        .arg(connection.position.Y(), 0, 'g', 12)
        .arg(connection.position.Z(), 0, 'g', 12)
        .arg(connection.normal.X(), 0, 'g', 12)
        .arg(connection.normal.Y(), 0, 'g', 12)
        .arg(connection.normal.Z(), 0, 'g', 12)
        .arg(static_cast<int>(connection.jointType))
        .arg(connection.boltNut.specIndex)
        .arg(connection.weld.typeIndex)
        .arg(labelEntry(connection.targetLabel));
}

bool parseDouble(const QString& text, double& value)
{
    bool ok = false;
    value = text.toDouble(&ok);
    return ok && std::isfinite(value);
}

bool parseInt(const QString& text, int& value)
{
    bool ok = false;
    value = text.toInt(&ok);
    return ok;
}

bool parseDirection(const QStringList& values,
                    int offset,
                    gp_Dir& direction)
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    if (!parseDouble(values.value(offset), x)
        || !parseDouble(values.value(offset + 1), y)
        || !parseDouble(values.value(offset + 2), z)
        || x * x + y * y + z * z <= 1.0e-24) {
        return false;
    }
    direction = gp_Dir(x, y, z);
    return true;
}

bool parseConnection(const QString& text,
                     SheetMetalConnectionEnd& connection,
                     QString& targetEntry)
{
    const QStringList values = text.split(QLatin1Char(','), Qt::KeepEmptyParts);
    if (values.size() != 10) {
        return false;
    }

    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    int jointType = 0;
    if (!parseDouble(values[0], x)
        || !parseDouble(values[1], y)
        || !parseDouble(values[2], z)
        || !parseDirection(values, 3, connection.normal)
        || !parseInt(values[6], jointType)
        || !parseInt(values[7], connection.boltNut.specIndex)
        || !parseInt(values[8], connection.weld.typeIndex)
        || (jointType != 0 && jointType != 1)) {
        return false;
    }

    connection.position = gp_Pnt(x, y, z);
    connection.jointType = static_cast<SheetMetalJointType>(jointType);
    targetEntry = values[9];
    return true;
}

bool parseRoute(const QString& text,
                int version,
                std::vector<SheetMetalRoutePoint>& routePoints)
{
    routePoints.clear();
    if (text.isEmpty()) {
        return true;
    }

    const QStringList points = text.split(QLatin1Char('|'), Qt::KeepEmptyParts);
    routePoints.reserve(static_cast<std::size_t>(points.size()));
    for (const QString& pointText : points) {
        const QStringList values = pointText.split(QLatin1Char(','), Qt::KeepEmptyParts);
        SheetMetalRoutePoint point;
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
        const int expectedValueCount = version >= 5 ? 8 : 4;
        int transitionType = 0;
        if (values.size() != expectedValueCount
            || !parseDouble(values[0], x)
            || !parseDouble(values[1], y)
            || !parseDouble(values[2], z)
            || (version >= 5 && !parseInt(values[3], transitionType))
            || (version >= 5 && (transitionType < 0 || transitionType > 2))
            || !parseDouble(values[version >= 5 ? 4 : 3],
                            point.transition.bend.radius)
            || point.transition.bend.radius < 0.0
            || (version >= 5
                && (!parseInt(values[5], point.transition.weld.typeIndex)
                    || !parseInt(values[6], point.transition.boltNut.specIndex)
                    || !parseInt(values[7], point.transition.boltNut.holeCount)
                    || point.transition.boltNut.holeCount <= 0))) {
            return false;
        }
        point.position = gp_Pnt(x, y, z);
        point.transition.type = static_cast<SheetMetalRouteTransitionType>(
            transitionType);
        routePoints.push_back(point);
    }

    if (!routePoints.empty()) {
        routePoints.front().isEndpoint = true;
        routePoints.back().isEndpoint = true;
    }
    return true;
}

gp_Vec rotateAroundAxis(const gp_Vec& vector,
                        const gp_Vec& axis,
                        double angle)
{
    const double cosine = std::cos(angle);
    const double sine = std::sin(angle);
    return vector * cosine
        + axis.Crossed(vector) * sine
        + axis * (axis.Dot(vector) * (1.0 - cosine));
}

void migrateLegacyTransitions(SheetMetalConnectorData& data,
                              const SheetMetalRouteTransition& legacyTransition)
{
    if (data.routePoints.size() < 3) {
        return;
    }

    const auto originalPoints = data.routePoints;
    gp_Vec firstDirection(originalPoints[0].position,
                          originalPoints[1].position);
    if (firstDirection.SquareMagnitude() <= Precision::SquareConfusion()) {
        return;
    }
    firstDirection.Normalize();

    gp_Vec reference(data.upDirection);
    if (std::abs(reference.Dot(firstDirection)) > 0.999) {
        reference = std::abs(firstDirection.Y()) < 0.999
            ? gp_Vec(0.0, 1.0, 0.0)
            : gp_Vec(1.0, 0.0, 0.0);
    }
    gp_Vec widthDirection = reference.Crossed(firstDirection);
    widthDirection.Normalize();
    gp_Vec thicknessDirection = firstDirection.Crossed(widthDirection);
    thicknessDirection.Normalize();

    std::vector<SheetMetalRoutePoint> migratedPoints;
    migratedPoints.reserve(originalPoints.size() + 2);
    migratedPoints.push_back(originalPoints.front());
    int bendCount = 0;
    for (std::size_t index = 1; index + 1 < originalPoints.size(); ++index) {
        auto point = originalPoints[index];
        gp_Vec incoming(originalPoints[index - 1].position,
                        point.position);
        gp_Vec outgoing(point.position,
                        originalPoints[index + 1].position);
        if (incoming.SquareMagnitude() <= Precision::SquareConfusion()
            || outgoing.SquareMagnitude() <= Precision::SquareConfusion()) {
            migratedPoints.push_back(point);
            continue;
        }
        incoming.Normalize();
        outgoing.Normalize();
        const double angle = std::acos(std::clamp(incoming.Dot(outgoing), -1.0, 1.0));
        if (angle <= Precision::Angular()) {
            migratedPoints.push_back(point);
            continue;
        }

        gp_Vec bendAxis = incoming.Crossed(outgoing);
        if (bendAxis.SquareMagnitude() <= Precision::SquareConfusion()) {
            migratedPoints.push_back(point);
            continue;
        }
        bendAxis.Normalize();
        const bool sideBend = std::abs(bendAxis.Dot(thicknessDirection)) > 0.999;
        const bool cornerSplit = (!data.allowSideBends && sideBend)
            || data.maximumBendCount == 0;
        if (cornerSplit) {
            point.transition = legacyTransition;
            migratedPoints.push_back(point);
            bendCount = 0;
        } else if (bendCount >= data.maximumBendCount) {
            SheetMetalRoutePoint splitPoint;
            splitPoint.position = gp_Pnt(
                (originalPoints[index - 1].position.X() + point.position.X()) * 0.5,
                (originalPoints[index - 1].position.Y() + point.position.Y()) * 0.5,
                (originalPoints[index - 1].position.Z() + point.position.Z()) * 0.5);
            splitPoint.transition = legacyTransition;
            migratedPoints.push_back(splitPoint);
            migratedPoints.push_back(point);
            bendCount = 1;
        } else {
            migratedPoints.push_back(point);
            ++bendCount;
        }

        widthDirection = rotateAroundAxis(widthDirection, bendAxis, angle);
        thicknessDirection = rotateAroundAxis(
            thicknessDirection, bendAxis, angle);
        widthDirection.Normalize();
        thicknessDirection.Normalize();
    }
    migratedPoints.push_back(originalPoints.back());
    data.routePoints = std::move(migratedPoints);
}

void setError(QString* errorMessage, const QString& message)
{
    if (errorMessage) {
        *errorMessage = message;
    }
}
}

QString SheetMetalConnectorMetadata::serialize(const SheetMetalConnectorData& data)
{
    QStringList routePoints;
    routePoints.reserve(static_cast<int>(data.routePoints.size()));
    for (const auto& point : data.routePoints) {
        routePoints.push_back(QStringLiteral("%1,%2,%3,%4,%5,%6,%7,%8")
            .arg(point.position.X(), 0, 'g', 12)
            .arg(point.position.Y(), 0, 'g', 12)
            .arg(point.position.Z(), 0, 'g', 12)
            .arg(static_cast<int>(point.transition.type))
            .arg(point.transition.bend.radius, 0, 'g', 12)
            .arg(point.transition.weld.typeIndex)
            .arg(point.transition.boltNut.specIndex)
            .arg(point.transition.boltNut.holeCount));
    }

    return QStringList{
        QStringLiteral("version=5"),
        QStringLiteral("width=%1").arg(data.width, 0, 'g', 12),
        QStringLiteral("thickness=%1").arg(data.thickness, 0, 'g', 12),
        QStringLiteral("clearance=%1").arg(data.clearance, 0, 'g', 12),
        QStringLiteral("up=%1,%2,%3")
            .arg(data.upDirection.X(), 0, 'g', 12)
            .arg(data.upDirection.Y(), 0, 'g', 12)
            .arg(data.upDirection.Z(), 0, 'g', 12),
        QStringLiteral("maximumBendCount=%1").arg(data.maximumBendCount),
        QStringLiteral("allowSideBends=%1").arg(data.allowSideBends ? 1 : 0),
        QStringLiteral("start=%1").arg(connectionText(data.start)),
        QStringLiteral("end=%1").arg(connectionText(data.end)),
        QStringLiteral("route=%1").arg(routePoints.join(QLatin1Char('|')))
    }.join(QLatin1Char(';'));
}

bool SheetMetalConnectorMetadata::deserialize(
    const QString& text,
    SheetMetalConnectorMetadataResult& result,
    QString* errorMessage)
{
    QMap<QString, QString> fields;
    const QStringList sections = text.split(QLatin1Char(';'), Qt::KeepEmptyParts);
    for (const QString& section : sections) {
        const int separator = section.indexOf(QLatin1Char('='));
        if (separator <= 0) {
            setError(errorMessage, QStringLiteral("Invalid metadata field."));
            return false;
        }
        fields.insert(section.left(separator), section.mid(separator + 1));
    }

    int version = 0;
    SheetMetalConnectorMetadataResult parsed;
    SheetMetalRouteTransition legacyTransition;
    legacyTransition.type = SheetMetalRouteTransitionType::Weld;
    if (!parseInt(fields.value(QStringLiteral("version")), version)
        || (version < 1 || version > 5)
        || !parseDouble(fields.value(QStringLiteral("width")), parsed.data.width)
        || !parseDouble(fields.value(QStringLiteral("thickness")), parsed.data.thickness)
        || !parseDouble(fields.value(QStringLiteral("clearance")), parsed.data.clearance)
        || parsed.data.width <= 0.0
        || parsed.data.thickness <= 0.0
        || parsed.data.clearance < 0.0) {
        setError(errorMessage, QStringLiteral("Invalid connector dimensions."));
        return false;
    }

    if (version >= 2) {
        const QStringList upValues = fields.value(QStringLiteral("up"))
            .split(QLatin1Char(','), Qt::KeepEmptyParts);
        if (upValues.size() != 3
            || !parseDirection(upValues, 0, parsed.data.upDirection)) {
            setError(errorMessage, QStringLiteral("Invalid section direction."));
            return false;
        }
    }

    if (version >= 3) {
        int allowSideBends = 0;
        if (!parseInt(fields.value(QStringLiteral("maximumBendCount")),
                      parsed.data.maximumBendCount)
            || parsed.data.maximumBendCount < 0
            || !parseInt(fields.value(QStringLiteral("allowSideBends")),
                         allowSideBends)
            || (allowSideBends != 0 && allowSideBends != 1)) {
            setError(errorMessage, QStringLiteral("Invalid manufacturing constraints."));
            return false;
        }
        parsed.data.allowSideBends = allowSideBends != 0;
    }

    if (version == 4) {
        int jointType = 0;
        if (!parseInt(fields.value(QStringLiteral("intermediateJointType")),
                      jointType)
            || (jointType != 0 && jointType != 1)
            || !parseInt(fields.value(QStringLiteral("intermediateWeldType")),
                         legacyTransition.weld.typeIndex)
            || !parseInt(fields.value(QStringLiteral("intermediateBoltSpec")),
                         legacyTransition.boltNut.specIndex)) {
            setError(errorMessage, QStringLiteral("Invalid intermediate joint."));
            return false;
        }
        legacyTransition.type = jointType == 0
            ? SheetMetalRouteTransitionType::Weld
            : SheetMetalRouteTransitionType::BoltNut;
    }

    if (!parseConnection(fields.value(QStringLiteral("start")),
                         parsed.data.start,
                         parsed.startTargetEntry)
        || !parseConnection(fields.value(QStringLiteral("end")),
                            parsed.data.end,
                            parsed.endTargetEntry)) {
        setError(errorMessage, QStringLiteral("Invalid connection endpoint."));
        return false;
    }

    if (!parseRoute(fields.value(QStringLiteral("route")),
                    version,
                    parsed.data.routePoints)
        || (!parsed.data.routePoints.empty() && parsed.data.routePoints.size() < 2)) {
        setError(errorMessage, QStringLiteral("Invalid connector route."));
        return false;
    }

    if (version < 5) {
        migrateLegacyTransitions(parsed.data, legacyTransition);
    }

    result = std::move(parsed);
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}
