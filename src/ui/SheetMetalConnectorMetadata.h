#pragma once

#include "SheetMetalConnectorModel.h"

#include <QString>

struct SheetMetalConnectorMetadataResult
{
    SheetMetalConnectorData data;
    QString startTargetEntry;
    QString endTargetEntry;
};

class SheetMetalConnectorMetadata
{
public:
    static QString serialize(const SheetMetalConnectorData& data);
    static bool deserialize(const QString& text,
                            SheetMetalConnectorMetadataResult& result,
                            QString* errorMessage = nullptr);
};
