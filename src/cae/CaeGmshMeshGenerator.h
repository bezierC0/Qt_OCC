#pragma once

#include "CaeExternalProcess.h"
#include "CaeExternalToolConfig.h"
#include "CaeServiceInterfaces.h"

namespace Cae {

class GmshMeshGenerator final : public IMeshGenerator {
public:
    GmshMeshGenerator(CaeExternalToolConfig config, IExternalProcessRunner* processRunner);

    QString name() const override;
    bool generate(const MeshRequest& request, MeshResult* result, QString* errorMessage) override;

private:
    bool readMeshStatistics(const QString& meshFilePath, MeshResult* result, QString* errorMessage) const;

    CaeExternalToolConfig m_config;
    IExternalProcessRunner* m_processRunner{nullptr};
};

} // namespace Cae
