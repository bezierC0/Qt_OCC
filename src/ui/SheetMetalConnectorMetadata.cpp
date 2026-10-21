#include "SheetMetalConnectorMetadata.h"

#include <TCollection_AsciiString.hxx>
#include <TDF_Tool.hxx>

#include <QMap>
#include <QStringList>

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
        if (values.size() != 4
            || !parseDouble(values[0], x)
            || !parseDouble(values[1], y)
            || !parseDouble(values[2], z)
            || !parseDouble(values[3], point.bend.radius)
            || point.bend.radius < 0.0) {
            return false;
        }
        point.position = gp_Pnt(x, y, z);
        routePoints.push_back(point);
    }

    if (!routePoints.empty()) {
        routePoints.front().isEndpoint = true;
        routePoints.back().isEndpoint = true;
    }
    return true;
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
        routePoints.push_back(QStringLiteral("%1,%2,%3,%4")
            .arg(point.position.X(), 0, 'g', 12)
            .arg(point.position.Y(), 0, 'g', 12)
            .arg(point.position.Z(), 0, 'g', 12)
            .arg(point.bend.radius, 0, 'g', 12));
    }

    return QStringLiteral(
        "version=2;width=%1;thickness=%2;clearance=%3;up=%4,%5,%6;start=%7;end=%8;route=%9")
        .arg(data.width, 0, 'g', 12)
        .arg(data.thickness, 0, 'g', 12)
        .arg(data.clearance, 0, 'g', 12)
        .arg(data.upDirection.X(), 0, 'g', 12)
        .arg(data.upDirection.Y(), 0, 'g', 12)
        .arg(data.upDirection.Z(), 0, 'g', 12)
        .arg(connectionText(data.start))
        .arg(connectionText(data.end))
        .arg(routePoints.join(QLatin1Char('|')));
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
    if (!parseInt(fields.value(QStringLiteral("version")), version)
        || (version != 1 && version != 2)
        || !parseDouble(fields.value(QStringLiteral("width")), parsed.data.width)
        || !parseDouble(fields.value(QStringLiteral("thickness")), parsed.data.thickness)
        || !parseDouble(fields.value(QStringLiteral("clearance")), parsed.data.clearance)
        || parsed.data.width <= 0.0
        || parsed.data.thickness <= 0.0
        || parsed.data.clearance < 0.0) {
        setError(errorMessage, QStringLiteral("Invalid connector dimensions."));
        return false;
    }

    if (version == 2) {
        const QStringList upValues = fields.value(QStringLiteral("up"))
            .split(QLatin1Char(','), Qt::KeepEmptyParts);
        if (upValues.size() != 3
            || !parseDirection(upValues, 0, parsed.data.upDirection)) {
            setError(errorMessage, QStringLiteral("Invalid section direction."));
            return false;
        }
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

    if (!parseRoute(fields.value(QStringLiteral("route")), parsed.data.routePoints)
        || (!parsed.data.routePoints.empty() && parsed.data.routePoints.size() < 2)) {
        setError(errorMessage, QStringLiteral("Invalid connector route."));
        return false;
    }

    result = std::move(parsed);
    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}
