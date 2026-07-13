#pragma once

#include "CaeProject.h"
#include "CaeServiceFactory.h"
#include "CaeTypes.h"

#include <memory>

namespace Cae {

class ICaeCommand;

class CaeController {
public:
    CaeController();
    explicit CaeController(CaeServiceProfile serviceProfile);
    ~CaeController();

    QString execute(std::unique_ptr<ICaeCommand> command);
    void clearProject();

    QString createStudy(StudyType type);
    QString useCurrentGeometry(bool hasGeometry = true);
    QString createDefaultNamedSelection();
    QString assignDefaultMaterial();
    QString addFixedSupport();
    QString addDefaultForce();
    QString generateMesh(const QString& geometryFilePath = QString());
    QString runSolver();
    QString showResult(ResultFieldType fieldType);
    QString runDemoAnalysis(bool hasGeometry = true, const QString& geometryFilePath = QString());
    QString summary() const;

    CaeProject& project();
    const CaeProject& project() const;
    CaeExternalToolConfig& externalToolConfig();
    const CaeExternalToolConfig& externalToolConfig() const;
    void setExternalToolConfig(const CaeExternalToolConfig& config);

private:
    std::unique_ptr<CaeProject> m_project;
    CaeServiceBundle m_services;
};

} // namespace Cae
