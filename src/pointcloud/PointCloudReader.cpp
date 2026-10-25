#include "PointCloudReader.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QtEndian>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>

namespace PointCloud
{
namespace
{

constexpr Color DefaultColor{200, 200, 200};

QString readerText(const char* text)
{
    return QCoreApplication::translate("PointCloudReader", text);
}

std::size_t safeReserveCount(std::uint64_t declaredCount, qint64 fileSize)
{
    const auto estimatedMaximum = static_cast<std::uint64_t>(
        std::max<qint64>(fileSize / 6, 0));
    return static_cast<std::size_t>(std::min({
        declaredCount,
        estimatedMaximum,
        static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())}));
}

QList<QByteArray> splitFields(const QByteArray& line)
{
    return line.simplified().split(' ');
}

bool isCancelled(const ReadOptions& options)
{
    return options.isCancelled && options.isCancelled();
}

void reportProgress(const QFile& file, const ReadOptions& options)
{
    if (!options.reportProgress || file.size() <= 0) {
        return;
    }

    const auto percent = static_cast<int>((file.pos() * 100) / file.size());
    options.reportProgress(std::clamp(percent, 0, 100));
}

bool parseFiniteDouble(const QByteArray& text, double& value)
{
    bool ok = false;
    value = text.toDouble(&ok);
    return ok && std::isfinite(value);
}

bool parseColor(const QByteArray& text, std::uint8_t& value)
{
    bool ok = false;
    const int component = text.toInt(&ok);
    if (!ok || component < 0 || component > 255) {
        return false;
    }
    value = static_cast<std::uint8_t>(component);
    return true;
}

bool parsePoint(const QList<QByteArray>& fields, int xIndex, int yIndex, int zIndex, Point& point)
{
    const int requiredIndex = std::max({xIndex, yIndex, zIndex});
    if (requiredIndex < 0 || fields.size() <= requiredIndex) {
        return false;
    }

    return parseFiniteDouble(fields[xIndex], point.x)
        && parseFiniteDouble(fields[yIndex], point.y)
        && parseFiniteDouble(fields[zIndex], point.z);
}

bool parseRgb(const QList<QByteArray>& fields, int redIndex, int greenIndex, int blueIndex, Color& color)
{
    const int requiredIndex = std::max({redIndex, greenIndex, blueIndex});
    if (redIndex < 0 || greenIndex < 0 || blueIndex < 0 || fields.size() <= requiredIndex) {
        return false;
    }

    return parseColor(fields[redIndex], color.red)
        && parseColor(fields[greenIndex], color.green)
        && parseColor(fields[blueIndex], color.blue);
}

void appendPoint(Data& data, const Point& point, const Color* color)
{
    if (color) {
        if (data.colors.empty() && !data.points.empty()) {
            data.colors.resize(data.points.size(), DefaultColor);
        }
        data.colors.push_back(*color);
    } else if (!data.colors.empty()) {
        data.colors.push_back(DefaultColor);
    }

    data.points.push_back(point);
    data.bounds.add(point);
}

ReadResult readTextPoints(QFile& file, const QString& filePath, const ReadOptions& options)
{
    auto data = std::make_shared<Data>();
    data->sourcePath = QFileInfo(filePath).absoluteFilePath();
    data->fileSize = static_cast<std::uint64_t>(std::max<qint64>(file.size(), 0));

    const bool isPts = QFileInfo(filePath).suffix().compare(QStringLiteral("pts"), Qt::CaseInsensitive) == 0;
    bool firstRecord = true;
    std::uint64_t lineNumber = 0;

    while (!file.atEnd()) {
        if (isCancelled(options)) {
            return {data, {}, {}, true};
        }

        const QByteArray line = file.readLine().trimmed();
        ++lineNumber;
        if ((lineNumber & 0x0FFFu) == 0u) {
            reportProgress(file, options);
        }
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        const QList<QByteArray> fields = splitFields(line);
        if (firstRecord && isPts && fields.size() == 1) {
            bool ok = false;
            const qulonglong declaredCount = fields.front().toULongLong(&ok);
            if (ok) {
                data->declaredPointCount = declaredCount;
                data->points.reserve(safeReserveCount(declaredCount, file.size()));
                firstRecord = false;
                continue;
            }
        }
        firstRecord = false;
        ++data->sourceRecordCount;

        Point point;
        if (!parsePoint(fields, 0, 1, 2, point)) {
            ++data->invalidRecordCount;
            continue;
        }

        Color color;
        const bool hasRgbFields = fields.size() == 6 || (isPts && fields.size() >= 7);
        const int colorOffset = isPts && fields.size() >= 7 ? 4 : 3;
        if (hasRgbFields) {
            if (!parseRgb(fields, colorOffset, colorOffset + 1, colorOffset + 2, color)) {
                ++data->invalidRecordCount;
                continue;
            }
            appendPoint(*data, point, &color);
        } else if (fields.size() == 3 || (isPts && fields.size() == 4)) {
            appendPoint(*data, point, nullptr);
        } else {
            ++data->invalidRecordCount;
        }
    }

    reportProgress(file, options);
    ReadResult result;
    result.data = std::move(data);
    if (result.data->points.empty()) {
        result.errorMessage = readerText("The file does not contain any valid points.");
    } else if (result.data->declaredPointCount != 0
               && result.data->declaredPointCount != result.data->points.size()) {
        result.warningMessage = readerText("The declared point count does not match the number of valid points.");
    }
    return result;
}

enum class PlyFormat
{
    Unknown,
    Ascii,
    BinaryLittleEndian,
    BinaryBigEndian
};

enum class PlyScalarType
{
    Invalid,
    Int8,
    UInt8,
    Int16,
    UInt16,
    Int32,
    UInt32,
    Float32,
    Float64
};

struct PlyProperty
{
    QByteArray name;
    PlyScalarType valueType{PlyScalarType::Invalid};
    PlyScalarType countType{PlyScalarType::Invalid};
    bool isList{false};
};

struct PlyElement
{
    QByteArray name;
    std::uint64_t count{};
    std::vector<PlyProperty> properties;
};

struct PlyHeader
{
    PlyFormat format{PlyFormat::Unknown};
    std::vector<PlyElement> elements;
};

PlyScalarType plyScalarType(const QByteArray& name)
{
    const QByteArray type = name.toLower();
    if (type == "char" || type == "int8") return PlyScalarType::Int8;
    if (type == "uchar" || type == "uint8") return PlyScalarType::UInt8;
    if (type == "short" || type == "int16") return PlyScalarType::Int16;
    if (type == "ushort" || type == "uint16") return PlyScalarType::UInt16;
    if (type == "int" || type == "int32") return PlyScalarType::Int32;
    if (type == "uint" || type == "uint32") return PlyScalarType::UInt32;
    if (type == "float" || type == "float32") return PlyScalarType::Float32;
    if (type == "double" || type == "float64") return PlyScalarType::Float64;
    return PlyScalarType::Invalid;
}

int plyScalarSize(PlyScalarType type)
{
    switch (type) {
    case PlyScalarType::Int8:
    case PlyScalarType::UInt8:
        return 1;
    case PlyScalarType::Int16:
    case PlyScalarType::UInt16:
        return 2;
    case PlyScalarType::Int32:
    case PlyScalarType::UInt32:
    case PlyScalarType::Float32:
        return 4;
    case PlyScalarType::Float64:
        return 8;
    default:
        return 0;
    }
}

bool isFloatingPlyType(PlyScalarType type)
{
    return type == PlyScalarType::Float32 || type == PlyScalarType::Float64;
}

bool isIntegerPlyType(PlyScalarType type)
{
    return type != PlyScalarType::Invalid && !isFloatingPlyType(type);
}

bool isColorName(const QByteArray& name, const char* fullName, const char* shortName)
{
    return name == fullName || name == shortName;
}

bool parsePlyHeader(QFile& file, PlyHeader& header, QString& errorMessage, const ReadOptions& options)
{
    if (file.readLine().trimmed() != QByteArrayLiteral("ply")) {
        errorMessage = readerText("Invalid PLY header.");
        return false;
    }

    PlyElement* currentElement = nullptr;
    while (!file.atEnd()) {
        if (isCancelled(options)) {
            return false;
        }

        const QByteArray line = file.readLine().trimmed();
        if (line.isEmpty() || line.startsWith("comment ") || line.startsWith("obj_info ")) {
            continue;
        }
        if (line.startsWith("format ")) {
            const QList<QByteArray> fields = splitFields(line);
            if (fields.size() != 3 || fields[2] != QByteArrayLiteral("1.0")) {
                errorMessage = readerText("Unsupported PLY format.");
                return false;
            }
            if (fields[1] == QByteArrayLiteral("ascii")) {
                header.format = PlyFormat::Ascii;
            } else if (fields[1] == QByteArrayLiteral("binary_little_endian")) {
                header.format = PlyFormat::BinaryLittleEndian;
            } else if (fields[1] == QByteArrayLiteral("binary_big_endian")) {
                header.format = PlyFormat::BinaryBigEndian;
            } else {
                errorMessage = readerText("Unsupported PLY format.");
                return false;
            }
        } else if (line.startsWith("element ")) {
            const QList<QByteArray> fields = splitFields(line);
            bool ok = false;
            const qulonglong count = fields.size() == 3 ? fields[2].toULongLong(&ok) : 0;
            if (!ok) {
                errorMessage = readerText("Invalid PLY element count.");
                return false;
            }
            header.elements.push_back({fields[1].toLower(), count, {}});
            currentElement = &header.elements.back();
        } else if (line.startsWith("property ")) {
            if (!currentElement) {
                errorMessage = readerText("PLY property is not associated with an element.");
                return false;
            }
            const QList<QByteArray> fields = splitFields(line);
            PlyProperty property;
            if (fields.size() == 3) {
                property.valueType = plyScalarType(fields[1]);
                property.name = fields[2].toLower();
            } else if (fields.size() == 5 && fields[1] == QByteArrayLiteral("list")) {
                property.isList = true;
                property.countType = plyScalarType(fields[2]);
                property.valueType = plyScalarType(fields[3]);
                property.name = fields[4].toLower();
            } else {
                errorMessage = readerText("Unsupported PLY property declaration.");
                return false;
            }
            if (property.valueType == PlyScalarType::Invalid
                || (property.isList && !isIntegerPlyType(property.countType))) {
                errorMessage = readerText("Unsupported PLY property type.");
                return false;
            }
            currentElement->properties.push_back(property);
        } else if (line == QByteArrayLiteral("end_header")) {
            if (header.format == PlyFormat::Unknown) {
                errorMessage = readerText("PLY format is missing.");
                return false;
            }
            return true;
        }
    }

    errorMessage = readerText("PLY header is incomplete.");
    return false;
}

const PlyElement* vertexElement(const PlyHeader& header)
{
    const auto found = std::find_if(header.elements.cbegin(), header.elements.cend(),
        [](const PlyElement& element) { return element.name == QByteArrayLiteral("vertex"); });
    return found == header.elements.cend() ? nullptr : &*found;
}

bool validateVertexElement(const PlyElement& element, bool& hasRgb, QString& errorMessage)
{
    bool hasX = false;
    bool hasY = false;
    bool hasZ = false;
    bool hasRed = false;
    bool hasGreen = false;
    bool hasBlue = false;
    for (const PlyProperty& property : element.properties) {
        if (property.isList) {
            errorMessage = readerText("PLY list properties on vertices are not supported.");
            return false;
        }
        hasX = hasX || property.name == QByteArrayLiteral("x");
        hasY = hasY || property.name == QByteArrayLiteral("y");
        hasZ = hasZ || property.name == QByteArrayLiteral("z");
        hasRed = hasRed || isColorName(property.name, "red", "r");
        hasGreen = hasGreen || isColorName(property.name, "green", "g");
        hasBlue = hasBlue || isColorName(property.name, "blue", "b");
    }
    if (!hasX || !hasY || !hasZ) {
        errorMessage = readerText("PLY vertex properties must include x, y, and z.");
        return false;
    }
    if ((hasRed || hasGreen || hasBlue) && !(hasRed && hasGreen && hasBlue)) {
        errorMessage = readerText("PLY vertex colors must include red, green, and blue.");
        return false;
    }
    hasRgb = hasRed && hasGreen && hasBlue;
    return true;
}

bool colorComponent(double value, PlyScalarType type, std::uint8_t& component)
{
    if (!std::isfinite(value)) {
        return false;
    }
    if (isFloatingPlyType(type) && value >= 0.0 && value <= 1.0) {
        value *= 255.0;
    }
    if (value < 0.0 || value > 255.0) {
        return false;
    }
    component = static_cast<std::uint8_t>(std::lround(value));
    return true;
}

void assignVertexProperty(
    const PlyProperty& property,
    double value,
    Point& point,
    Color& color,
    bool& valid)
{
    if (property.name == QByteArrayLiteral("x")) {
        point.x = value;
        valid = valid && std::isfinite(value);
    } else if (property.name == QByteArrayLiteral("y")) {
        point.y = value;
        valid = valid && std::isfinite(value);
    } else if (property.name == QByteArrayLiteral("z")) {
        point.z = value;
        valid = valid && std::isfinite(value);
    } else if (isColorName(property.name, "red", "r")) {
        valid = colorComponent(value, property.valueType, color.red) && valid;
    } else if (isColorName(property.name, "green", "g")) {
        valid = colorComponent(value, property.valueType, color.green) && valid;
    } else if (isColorName(property.name, "blue", "b")) {
        valid = colorComponent(value, property.valueType, color.blue) && valid;
    }
}

void reportRecordProgress(const ReadOptions& options, std::uint64_t current, std::uint64_t total)
{
    if (!options.reportProgress || total == 0) {
        return;
    }
    const int percent = static_cast<int>(
        (static_cast<long double>(current) * 100.0L) / static_cast<long double>(total));
    options.reportProgress(std::clamp(percent, 0, 100));
}

ReadResult finishPlyRead(std::shared_ptr<Data> data)
{
    ReadResult result;
    result.data = std::move(data);
    if (result.data->points.empty()) {
        result.errorMessage = readerText("The PLY file does not contain any valid points.");
    } else if (result.data->points.size() != result.data->declaredPointCount) {
        result.warningMessage = readerText("Some PLY vertex records were invalid or missing.");
    }
    return result;
}

ReadResult readAsciiPlyVertices(
    QFile& file,
    const PlyHeader& header,
    std::shared_ptr<Data> data,
    bool hasRgb,
    const ReadOptions& options)
{
    for (const PlyElement& element : header.elements) {
        if (element.name != QByteArrayLiteral("vertex")) {
            for (std::uint64_t index = 0; index < element.count; ++index) {
                if (isCancelled(options)) return {data, {}, {}, true};
                if (file.atEnd()) {
                    return {data, readerText("PLY data is incomplete."), {}, false};
                }
                file.readLine();
            }
            continue;
        }

        for (std::uint64_t index = 0; index < element.count; ++index) {
            if (isCancelled(options)) return {data, {}, {}, true};
            if (file.atEnd()) {
                data->invalidRecordCount += element.count - index;
                break;
            }

            const QList<QByteArray> fields = splitFields(file.readLine().trimmed());
            ++data->sourceRecordCount;
            Point point;
            Color color;
            bool valid = fields.size() >= static_cast<int>(element.properties.size());
            for (int propertyIndex = 0;
                 valid && propertyIndex < static_cast<int>(element.properties.size());
                 ++propertyIndex) {
                double value = 0.0;
                valid = parseFiniteDouble(fields[propertyIndex], value);
                if (valid) {
                    assignVertexProperty(element.properties[static_cast<std::size_t>(propertyIndex)],
                                         value, point, color, valid);
                }
            }
            if (valid) {
                appendPoint(*data, point, hasRgb ? &color : nullptr);
            } else {
                ++data->invalidRecordCount;
            }
            if ((index & 0x0FFFu) == 0u) reportRecordProgress(options, index + 1, element.count);
        }
        reportRecordProgress(options, element.count, element.count);
        return finishPlyRead(std::move(data));
    }
    return {data, readerText("The PLY file does not contain vertices."), {}, false};
}

bool readLittleEndianScalar(const char* bytes, PlyScalarType type, double& value)
{
    const auto* data = reinterpret_cast<const uchar*>(bytes);
    switch (type) {
    case PlyScalarType::Int8:
        value = qFromLittleEndian<qint8>(data);
        return true;
    case PlyScalarType::UInt8:
        value = data[0];
        return true;
    case PlyScalarType::Int16:
        value = qFromLittleEndian<qint16>(data);
        return true;
    case PlyScalarType::UInt16:
        value = qFromLittleEndian<quint16>(data);
        return true;
    case PlyScalarType::Int32:
        value = qFromLittleEndian<qint32>(data);
        return true;
    case PlyScalarType::UInt32:
        value = qFromLittleEndian<quint32>(data);
        return true;
    case PlyScalarType::Float32: {
        const quint32 bits = qFromLittleEndian<quint32>(data);
        float decoded = 0.0f;
        std::memcpy(&decoded, &bits, sizeof(decoded));
        value = decoded;
        return true;
    }
    case PlyScalarType::Float64: {
        const quint64 bits = qFromLittleEndian<quint64>(data);
        double decoded = 0.0;
        std::memcpy(&decoded, &bits, sizeof(decoded));
        value = decoded;
        return true;
    }
    default:
        return false;
    }
}

bool readBinaryScalar(QFile& file, PlyScalarType type, double& value)
{
    const int size = plyScalarSize(type);
    char bytes[8]{};
    return size > 0
        && file.read(bytes, size) == size
        && readLittleEndianScalar(bytes, type, value);
}

bool skipBytes(QFile& file, std::uint64_t count)
{
    if (count > static_cast<std::uint64_t>(std::numeric_limits<qint64>::max())) {
        return false;
    }
    const qint64 byteCount = static_cast<qint64>(count);
    return file.pos() <= file.size() - byteCount && file.seek(file.pos() + byteCount);
}

bool skipBinaryElement(QFile& file, const PlyElement& element, const ReadOptions& options)
{
    for (std::uint64_t recordIndex = 0; recordIndex < element.count; ++recordIndex) {
        if (isCancelled(options)) return false;
        for (const PlyProperty& property : element.properties) {
            if (!property.isList) {
                if (!skipBytes(file, static_cast<std::uint64_t>(plyScalarSize(property.valueType)))) return false;
                continue;
            }
            double countValue = 0.0;
            if (!readBinaryScalar(file, property.countType, countValue)
                || !std::isfinite(countValue)
                || countValue < 0.0
                || countValue > static_cast<double>(std::numeric_limits<quint32>::max())) {
                return false;
            }
            const auto itemCount = static_cast<std::uint64_t>(countValue);
            const auto itemSize = static_cast<std::uint64_t>(plyScalarSize(property.valueType));
            if (itemSize != 0 && itemCount > std::numeric_limits<std::uint64_t>::max() / itemSize) {
                return false;
            }
            if (!skipBytes(file, itemCount * itemSize)) return false;
        }
    }
    return true;
}

ReadResult readBinaryPlyVertices(
    QFile& file,
    const PlyHeader& header,
    std::shared_ptr<Data> data,
    bool hasRgb,
    const ReadOptions& options)
{
    for (const PlyElement& element : header.elements) {
        if (element.name != QByteArrayLiteral("vertex")) {
            if (!skipBinaryElement(file, element, options)) {
                return isCancelled(options)
                    ? ReadResult{data, {}, {}, true}
                    : ReadResult{data, readerText("PLY binary data is incomplete."), {}, false};
            }
            continue;
        }

        std::vector<int> offsets;
        offsets.reserve(element.properties.size());
        std::uint64_t stride = 0;
        for (const PlyProperty& property : element.properties) {
            offsets.push_back(static_cast<int>(stride));
            const int propertySize = plyScalarSize(property.valueType);
            if (propertySize <= 0
                || stride > static_cast<std::uint64_t>(std::numeric_limits<int>::max() - propertySize)) {
                return {data, readerText("PLY vertex record is too large."), {}, false};
            }
            stride += static_cast<std::uint64_t>(propertySize);
        }
        if (stride == 0 || stride > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
            return {data, readerText("PLY vertex record is invalid."), {}, false};
        }

        QByteArray record(static_cast<int>(stride), '\0');
        for (std::uint64_t index = 0; index < element.count; ++index) {
            if (isCancelled(options)) return {data, {}, {}, true};
            if (file.read(record.data(), record.size()) != record.size()) {
                return {data, readerText("PLY binary vertex data is incomplete."), {}, false};
            }

            ++data->sourceRecordCount;
            Point point;
            Color color;
            bool valid = true;
            for (std::size_t propertyIndex = 0; propertyIndex < element.properties.size(); ++propertyIndex) {
                double value = 0.0;
                valid = readLittleEndianScalar(
                    record.constData() + offsets[propertyIndex],
                    element.properties[propertyIndex].valueType,
                    value) && valid;
                if (valid) {
                    assignVertexProperty(element.properties[propertyIndex], value, point, color, valid);
                }
            }
            if (valid) {
                appendPoint(*data, point, hasRgb ? &color : nullptr);
            } else {
                ++data->invalidRecordCount;
            }
            if ((index & 0x0FFFu) == 0u) reportRecordProgress(options, index + 1, element.count);
        }
        reportRecordProgress(options, element.count, element.count);
        return finishPlyRead(std::move(data));
    }
    return {data, readerText("The PLY file does not contain vertices."), {}, false};
}

ReadResult readPly(QFile& file, const QString& filePath, const ReadOptions& options)
{
    auto data = std::make_shared<Data>();
    data->sourcePath = QFileInfo(filePath).absoluteFilePath();
    data->fileSize = static_cast<std::uint64_t>(std::max<qint64>(file.size(), 0));

    PlyHeader header;
    QString errorMessage;
    if (!parsePlyHeader(file, header, errorMessage, options)) {
        return isCancelled(options)
            ? ReadResult{data, {}, {}, true}
            : ReadResult{data, errorMessage, {}, false};
    }
    if (header.format == PlyFormat::BinaryBigEndian) {
        return {data, readerText("Binary big-endian PLY files are not supported."), {}, false};
    }

    const PlyElement* vertices = vertexElement(header);
    if (!vertices || vertices->count == 0) {
        return {data, readerText("The PLY file does not contain vertices."), {}, false};
    }
    bool hasRgb = false;
    if (!validateVertexElement(*vertices, hasRgb, errorMessage)) {
        return {data, errorMessage, {}, false};
    }

    data->declaredPointCount = vertices->count;
    const std::size_t reserveCount = safeReserveCount(vertices->count, file.size());
    data->points.reserve(reserveCount);
    if (hasRgb) data->colors.reserve(reserveCount);

    return header.format == PlyFormat::Ascii
        ? readAsciiPlyVertices(file, header, std::move(data), hasRgb, options)
        : readBinaryPlyVertices(file, header, std::move(data), hasRgb, options);
}

} // namespace

bool Reader::supportsFile(const QString& filePath)
{
    const QString suffix = QFileInfo(filePath).suffix().toLower();
    return suffix == QStringLiteral("xyz")
        || suffix == QStringLiteral("txt")
        || suffix == QStringLiteral("pts")
        || suffix == QStringLiteral("ply");
}

ReadResult Reader::read(const QString& filePath, const ReadOptions& options)
{
    if (!supportsFile(filePath)) {
        return {{}, readerText("Unsupported point cloud file format."), {}, false};
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {{}, readerText("Unable to open the point cloud file."), {}, false};
    }

    try {
        if (QFileInfo(filePath).suffix().compare(QStringLiteral("ply"), Qt::CaseInsensitive) == 0) {
            return readPly(file, filePath, options);
        }
        return readTextPoints(file, filePath, options);
    } catch (const std::bad_alloc&) {
        return {{}, readerText("Not enough memory to read the point cloud."), {}, false};
    } catch (const std::length_error&) {
        return {{}, readerText("The point cloud is too large to read."), {}, false};
    }
}

} // namespace PointCloud
