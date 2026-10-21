#pragma once

#include <QString>

#include <algorithm>
#include <cstdint>
#include <vector>

namespace PointCloud
{

struct Point
{
    double x{};
    double y{};
    double z{};
};

struct Color
{
    std::uint8_t red{};
    std::uint8_t green{};
    std::uint8_t blue{};
};

struct Bounds
{
    bool valid{false};
    double xMin{};
    double yMin{};
    double zMin{};
    double xMax{};
    double yMax{};
    double zMax{};

    void add(const Point& point)
    {
        if (!valid) {
            valid = true;
            xMin = xMax = point.x;
            yMin = yMax = point.y;
            zMin = zMax = point.z;
            return;
        }

        xMin = std::min(xMin, point.x);
        yMin = std::min(yMin, point.y);
        zMin = std::min(zMin, point.z);
        xMax = std::max(xMax, point.x);
        yMax = std::max(yMax, point.y);
        zMax = std::max(zMax, point.z);
    }
};

struct Data
{
    QString sourcePath;
    std::vector<Point> points;
    std::vector<Color> colors;
    Bounds bounds;
    std::uint64_t declaredPointCount{};
    std::uint64_t sourceRecordCount{};
    std::uint64_t invalidRecordCount{};
    std::uint64_t fileSize{};

    bool hasColors() const
    {
        return !colors.empty() && colors.size() == points.size();
    }
};

enum class ColorMode
{
    Original = 0,
    Single,
    Height
};

} // namespace PointCloud
