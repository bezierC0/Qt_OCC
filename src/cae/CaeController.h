#pragma once

#include "CaeProject.h"
#include "CaeServiceFactory.h"
#include "CaeTypes.h"

#include <array>
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
    bool activateStudy(const QUuid& studyId);

    QString createStudy(StudyType type);
    QString useCurrentGeometry(bool hasGeometry = true);
    QString createDefaultNamedSelection();
    QString createNamedSelection(const QString& name, const PlanarSelectionRegion& region);
    QString assignMaterial(const QString& name, double youngModulus, double poissonRatio);
    QString assignDefaultMaterial();
    QString addFixedSupport(const QString& targetName = QString());
    QString addForce(double force, const QString& targetName = QString());
    QString addForce(
        const std::array<double, 3>& force,
        const QString& targetName = QString());
    QString addPressure(double pressure, const QString& targetName = QString());
    QString removeBoundaryCondition(const QUuid& studyId, const QString& name);
    QString addDefaultForce();
    QString generateMesh(const QString& geometryFilePath = QString(), double globalSize = 1.0);
    QString runSolver();
    QString showResult(ResultFieldType fieldType);
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
