#pragma once

#include "PointCloudData.h"

#include <functional>
#include <memory>

namespace PointCloud
{

struct ReadOptions
{
    std::function<bool()> isCancelled;
    std::function<void(int)> reportProgress;
};

struct ReadResult
{
    std::shared_ptr<Data> data;
    QString errorMessage;
    QString warningMessage;
    bool cancelled{false};

    bool succeeded() const
    {
        return !cancelled && errorMessage.isEmpty() && data && !data->points.empty();
    }
};

class Reader
{
public:
    static bool supportsFile(const QString& filePath);
    static ReadResult read(const QString& filePath, const ReadOptions& options = {});
};

} // namespace PointCloud
