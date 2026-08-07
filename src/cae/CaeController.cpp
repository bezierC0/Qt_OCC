#include "CaeController.h"

#include "CaeCommand.h"
#include "CaeWorkflowValidator.h"

#include <algorithm>
#include <utility>

namespace Cae {

namespace {

const CaeNamedSelection* findPlanarTarget(
    const CaeStudy* study,
    const QString& targetName)
{
    if (!study) {
        return nullptr;
    }
    const CaeNamedSelection* target = targetName.isEmpty()
        ? nullptr
        : study->findNamedSelection(targetName);
    if (target) {
        return target;
    }

    const auto firstFace = std::find_if(
        study->namedSelections().cbegin(),
        study->namedSelections().cend(),
        [](const CaeNamedSelection& selection) {
            return selection.scope() == NamedSelectionScope::Face &&
                selection.planarRegion().has_value();
        });
    return firstFace == study->namedSelections().cend() ? nullptr : &*firstFace;
}

} // namespace

CaeController::CaeController()
    : CaeController(CaeServiceProfile::Dummy)
{
}

CaeController::CaeController(CaeServiceProfile serviceProfile)
    : m_project(std::make_unique<CaeProject>())
    , m_services(CaeServiceFactory::create(serviceProfile))
{
}

CaeController::~CaeController() = default;

QString CaeController::execute(std::unique_ptr<ICaeCommand> command)
{
    if (!command) {
        return QStringLiteral("CAE command is empty.");
    }
    return command->execute(*this);
}

void CaeController::clearProject()
{
    m_project->clear();
}

bool CaeController::activateStudy(const QUuid& studyId)
{
    return m_project->activateStudy(studyId);
}

QString CaeController::createStudy(StudyType type)
{
    CaeStudy& study = m_project->createStudy(type);
    return QStringLiteral("Created CAE study: %1").arg(study.name());
}

QString CaeController::useCurrentGeometry(bool hasGeometry)
{
    CaeStudy* study = m_project->activeStudy();
    const QString validationError = CaeWorkflowValidator::requireActiveStudy(study);
    if (!validationError.isEmpty()) {
        return validationError;
    }

    if (!hasGeometry) {
        return QStringLiteral("Create or open CAD geometry before using it in a CAE study.");
    }

    study->resetForGeometry();
    study->addNamedSelection(
        CaeNamedSelection(QStringLiteral("All Geometry"), NamedSelectionScope::Geometry));
    return QStringLiteral("%1 uses the current CAD geometry.").arg(study->name());
}

QString CaeController::createDefaultNamedSelection()
{
    CaeStudy* study = m_project->activeStudy();
    const QString validationError = CaeWorkflowValidator::requireGeometryReady(study);
    if (!validationError.isEmpty()) {
        return validationError;
    }

    study->addNamedSelection(CaeNamedSelection(QStringLiteral("Current Geometry"), NamedSelectionScope::Geometry));
    return QStringLiteral("Created named selection in %1: Current Geometry.").arg(study->name());
}

QString CaeController::createNamedSelection(
    const QString& name,
    const PlanarSelectionRegion& region)
{
    CaeStudy* study = m_project->activeStudy();
    const QString validationError = CaeWorkflowValidator::requireGeometryReady(study);
    if (!validationError.isEmpty()) {
        return validationError;
    }

    const QString selectionName = name.trimmed();
    if (selectionName.isEmpty()) {
        return QStringLiteral("Named selection requires a name.");
    }
    if (selectionName.compare(QStringLiteral("All Geometry"), Qt::CaseInsensitive) == 0) {
        return QStringLiteral("All Geometry is reserved for material assignment.");
    }
    study->addNamedSelection(
        CaeNamedSelection(selectionName, NamedSelectionScope::Face, region));
    return QStringLiteral("Created planar face selection in %1: %2.")
        .arg(study->name(), selectionName);
}

QString CaeController::removeNamedSelection(
    const QUuid& studyId,
    const QString& name)
{
    if (!m_project->activateStudy(studyId)) {
        return QStringLiteral("The selected CAE study is unavailable.");
    }

    CaeStudy* study = m_project->activeStudy();
    const CaeNamedSelection* selection = study
        ? study->findNamedSelection(name)
        : nullptr;
    if (!selection) {
        return QStringLiteral("Named selection was not found: %1.").arg(name);
    }
    if (selection->scope() == NamedSelectionScope::Geometry) {
        return QStringLiteral("The geometry selection cannot be removed.");
    }
    if (!study->removeNamedSelection(name)) {
        return QStringLiteral("Named selection was not removed: %1.").arg(name);
    }
    return QStringLiteral("Removed named selection from %1: %2.")
        .arg(study->name(), name);
}

QString CaeController::assignDefaultMaterial()
{
    return assignMaterial(QStringLiteral("Default Steel"), 210000.0, 0.3);
}

QString CaeController::assignMaterial(
    const QString& name,
    double youngModulus,
    double poissonRatio)
{
    CaeStudy* study = m_project->activeStudy();
    const QString validationError = CaeWorkflowValidator::requireNamedSelection(study);
    if (!validationError.isEmpty()) {
        return validationError;
    }
    if (study->type() != StudyType::StaticStructural) {
        return QStringLiteral("Elastic material is only available for static structural studies.");
    }

    if (youngModulus <= 0.0 || poissonRatio < 0.0 || poissonRatio >= 0.5) {
        return QStringLiteral("Material requires E > 0 and 0 <= nu < 0.5.");
    }

    const QString materialName = name.trimmed().isEmpty() ? QStringLiteral("Material") : name.trimmed();
    const auto geometrySelection = std::find_if(
        study->namedSelections().cbegin(),
        study->namedSelections().cend(),
        [](const CaeNamedSelection& selection) {
            return selection.scope() == NamedSelectionScope::Geometry;
        });
    const QString targetName = geometrySelection != study->namedSelections().cend()
        ? geometrySelection->name()
        : study->namedSelections().front().name();
    study->addMaterial(CaeMaterial(materialName, targetName, youngModulus, poissonRatio));
    return QStringLiteral("Assigned material to %1: %2 (E=%3 MPa, nu=%4) -> %5.")
        .arg(study->name(), materialName)
        .arg(youngModulus)
        .arg(poissonRatio)
        .arg(targetName);
}

QString CaeController::assignThermalMaterial(
    const QString& name,
    double thermalConductivity)
{
    CaeStudy* study = m_project->activeStudy();
    const QString validationError = CaeWorkflowValidator::requireNamedSelection(study);
    if (!validationError.isEmpty()) {
        return validationError;
    }
    if (study->type() != StudyType::SteadyThermal) {
        return QStringLiteral("Thermal material is only available for steady thermal studies.");
    }
    if (thermalConductivity <= 0.0) {
        return QStringLiteral("Thermal conductivity must be greater than zero.");
    }

    const QString materialName = name.trimmed().isEmpty()
        ? QStringLiteral("Thermal Material")
        : name.trimmed();
    const auto geometrySelection = std::find_if(
        study->namedSelections().cbegin(),
        study->namedSelections().cend(),
        [](const CaeNamedSelection& selection) {
            return selection.scope() == NamedSelectionScope::Geometry;
        });
    const QString targetName = geometrySelection != study->namedSelections().cend()
        ? geometrySelection->name()
        : study->namedSelections().front().name();
    study->addMaterial(CaeMaterial(materialName, targetName, thermalConductivity));
    return QStringLiteral("Assigned thermal material to %1: %2 (k=%3 W/(m*K)) -> %4.")
        .arg(study->name(), materialName)
        .arg(thermalConductivity)
        .arg(targetName);
}

QString CaeController::removeMaterial(
    const QUuid& studyId,
    const QString& name)
{
    if (!m_project->activateStudy(studyId)) {
        return QStringLiteral("The selected CAE study is unavailable.");
    }

    CaeStudy* study = m_project->activeStudy();
    if (!study || !study->removeMaterial(name)) {
        return QStringLiteral("Material was not found: %1.").arg(name);
    }
    return QStringLiteral("Removed material from %1: %2.")
        .arg(study->name(), name);
}

QString CaeController::addFixedSupport(const QString& targetName)
{
    CaeStudy* study = m_project->activeStudy();
    const QString validationError = CaeWorkflowValidator::requireNamedSelection(study);
    if (!validationError.isEmpty()) {
        return validationError;
    }
    if (study->type() != StudyType::StaticStructural) {
        return QStringLiteral("Fixed Support is only available for static structural studies.");
    }

    const CaeNamedSelection* target = findPlanarTarget(study, targetName);
    if (!target || target->scope() != NamedSelectionScope::Face || !target->planarRegion()) {
        return QStringLiteral("Fixed support requires a planar face named selection.");
    }
    study->addBoundaryCondition(CaeBoundaryCondition(
        QStringLiteral("Fixed Support"),
        target->name(),
        BoundaryConditionType::FixedSupport));
    return QStringLiteral("Added boundary condition to %1: Fixed Support -> %2.")
        .arg(study->name(), target->name());
}

QString CaeController::addDefaultForce()
{
    return addForce(100.0);
}

QString CaeController::addForce(double force, const QString& targetName)
{
    return addForce({force, 0.0, 0.0}, targetName);
}

QString CaeController::addForce(
    const std::array<double, 3>& force,
    const QString& targetName)
{
    CaeStudy* study = m_project->activeStudy();
    const QString validationError = CaeWorkflowValidator::requireNamedSelection(study);
    if (!validationError.isEmpty()) {
        return validationError;
    }
    if (study->type() != StudyType::StaticStructural) {
        return QStringLiteral("Force is only available for static structural studies.");
    }

    if (force[0] == 0.0 && force[1] == 0.0 && force[2] == 0.0) {
        return QStringLiteral("At least one force component must be non-zero.");
    }

    const CaeNamedSelection* target = findPlanarTarget(study, targetName);
    if (!target || target->scope() != NamedSelectionScope::Face || !target->planarRegion()) {
        return QStringLiteral("Force requires a planar face named selection.");
    }
    study->addBoundaryCondition(CaeBoundaryCondition(
        QStringLiteral("Force"),
        target->name(),
        BoundaryConditionType::Force,
        force,
        QStringLiteral("N")));
    return QStringLiteral("Added load to %1: F=(%2, %3, %4) N -> %5.")
        .arg(study->name())
        .arg(force[0])
        .arg(force[1])
        .arg(force[2])
        .arg(target->name());
}

QString CaeController::addPressure(double pressure, const QString& targetName)
{
    CaeStudy* study = m_project->activeStudy();
    const QString validationError = CaeWorkflowValidator::requireNamedSelection(study);
    if (!validationError.isEmpty()) {
        return validationError;
    }
    if (study->type() != StudyType::StaticStructural) {
        return QStringLiteral("Pressure is only available for static structural studies.");
    }
    if (pressure <= 0.0) {
        return QStringLiteral("Pressure must be greater than zero.");
    }

    const CaeNamedSelection* target = findPlanarTarget(study, targetName);
    if (!target || target->scope() != NamedSelectionScope::Face || !target->planarRegion()) {
        return QStringLiteral("Pressure requires a planar face named selection.");
    }
    study->addBoundaryCondition(CaeBoundaryCondition(
        QStringLiteral("Pressure"),
        target->name(),
        BoundaryConditionType::Pressure,
        pressure,
        QStringLiteral("MPa")));
    return QStringLiteral("Added pressure to %1: %2 MPa -> %3.")
        .arg(study->name())
        .arg(pressure)
        .arg(target->name());
}

QString CaeController::addFixedTemperature(
    double temperature,
    const QString& targetName)
{
    CaeStudy* study = m_project->activeStudy();
    const QString validationError = CaeWorkflowValidator::requireNamedSelection(study);
    if (!validationError.isEmpty()) {
        return validationError;
    }
    if (study->type() != StudyType::SteadyThermal) {
        return QStringLiteral("Fixed Temperature is only available for steady thermal studies.");
    }
    if (temperature < -273.15) {
        return QStringLiteral("Fixed temperature cannot be below absolute zero.");
    }

    const CaeNamedSelection* target = findPlanarTarget(study, targetName);
    if (!target || target->scope() != NamedSelectionScope::Face || !target->planarRegion()) {
        return QStringLiteral("Fixed Temperature requires a planar face named selection.");
    }
    study->addBoundaryCondition(CaeBoundaryCondition(
        QStringLiteral("Fixed Temperature"),
        target->name(),
        BoundaryConditionType::FixedTemperature,
        temperature,
        QStringLiteral("C")));
    return QStringLiteral("Added fixed temperature to %1: %2 C -> %3.")
        .arg(study->name())
        .arg(temperature)
        .arg(target->name());
}

QString CaeController::addHeatFlux(double heatFlux, const QString& targetName)
{
    CaeStudy* study = m_project->activeStudy();
    const QString validationError = CaeWorkflowValidator::requireNamedSelection(study);
    if (!validationError.isEmpty()) {
        return validationError;
    }
    if (study->type() != StudyType::SteadyThermal) {
        return QStringLiteral("Heat Flux is only available for steady thermal studies.");
    }
    if (heatFlux <= 0.0) {
        return QStringLiteral("Heat flux must be greater than zero.");
    }

    const CaeNamedSelection* target = findPlanarTarget(study, targetName);
    if (!target || target->scope() != NamedSelectionScope::Face || !target->planarRegion()) {
        return QStringLiteral("Heat Flux requires a planar face named selection.");
    }
    study->addBoundaryCondition(CaeBoundaryCondition(
        QStringLiteral("Heat Flux"),
        target->name(),
        BoundaryConditionType::HeatFlux,
        heatFlux,
        QStringLiteral("W/mm^2")));
    return QStringLiteral("Added heat flux to %1: %2 W/mm^2 -> %3.")
        .arg(study->name())
        .arg(heatFlux)
        .arg(target->name());
}

QString CaeController::addConvection(
    double filmCoefficient,
    double ambientTemperature,
    const QString& targetName)
{
    CaeStudy* study = m_project->activeStudy();
    const QString validationError = CaeWorkflowValidator::requireNamedSelection(study);
    if (!validationError.isEmpty()) {
        return validationError;
    }
    if (study->type() != StudyType::SteadyThermal) {
        return QStringLiteral("Convection is only available for steady thermal studies.");
    }
    if (filmCoefficient <= 0.0) {
        return QStringLiteral("Film coefficient must be greater than zero.");
    }
    if (ambientTemperature < -273.15) {
        return QStringLiteral("Ambient temperature cannot be below absolute zero.");
    }

    const CaeNamedSelection* target = findPlanarTarget(study, targetName);
    if (!target || target->scope() != NamedSelectionScope::Face || !target->planarRegion()) {
        return QStringLiteral("Convection requires a planar face named selection.");
    }
    study->addBoundaryCondition(CaeBoundaryCondition(
        QStringLiteral("Convection"),
        target->name(),
        BoundaryConditionType::Convection,
        filmCoefficient,
        QStringLiteral("W/(m^2*K)"),
        ambientTemperature,
        QStringLiteral("C")));
    return QStringLiteral("Added convection to %1: h=%2 W/(m^2*K), ambient=%3 C -> %4.")
        .arg(study->name())
        .arg(filmCoefficient)
        .arg(ambientTemperature)
        .arg(target->name());
}

QString CaeController::removeBoundaryCondition(
    const QUuid& studyId,
    const QString& name)
{
    if (!m_project->activateStudy(studyId)) {
        return QStringLiteral("The selected CAE study is unavailable.");
    }

    CaeStudy* study = m_project->activeStudy();
    if (!study || !study->removeBoundaryCondition(name)) {
        return QStringLiteral("Boundary condition was not found: %1.").arg(name);
    }
    return QStringLiteral("Removed boundary condition from %1: %2.")
        .arg(study->name(), name);
}

QString CaeController::generateMesh(const QString& geometryFilePath, double globalSize)
{
    CaeStudy* study = m_project->activeStudy();
    const QString validationError = CaeWorkflowValidator::requireMeshInputs(study);
    if (!validationError.isEmpty()) {
        return validationError;
    }

    if (globalSize <= 0.0) {
        return QStringLiteral("Maximum element size must be greater than zero.");
    }

    const CaeMeshSetup setup(globalSize, MeshElementOrder::First);
    MeshResult meshResult;
    QString errorMessage;
    if (!m_services.meshGenerator->generate(MeshRequest{setup.globalSize(), geometryFilePath}, &meshResult, &errorMessage)) {
        study->setState(StudyState::Failed);
        return errorMessage;
    }

    study->setMesh(CaeMesh(
        setup,
        meshResult.nodeCount,
        meshResult.surfaceElementCount,
        meshResult.volumeElementCount,
        meshResult.meshFilePath));
    study->setState(StudyState::Meshed);
    return QStringLiteral("Mesh setup prepared for %1 by %2: size=%3, order=%4.")
        .arg(study->name())
        .arg(m_services.meshGenerator->name())
        .arg(setup.globalSize())
        .arg(toDisplayString(setup.elementOrder()));
}

QString CaeController::runSolver()
{
    CaeStudy* study = m_project->activeStudy();
    const QString validationError = CaeWorkflowValidator::requireMeshReady(study);
    if (!validationError.isEmpty()) {
        return validationError;
    }

    const CaeSolverSetup setup(SolverBackend::CalculiX, study->type());
    study->setState(StudyState::Solving);
    SolverResult solverResult;
    QString errorMessage;
    std::array<double, 3> force{};
    std::optional<PlanarSelectionRegion> fixedRegion;
    std::optional<PlanarSelectionRegion> loadRegion;
    double pressure = 0.0;
    std::optional<PlanarSelectionRegion> pressureRegion;
    double fixedTemperature = 20.0;
    std::optional<PlanarSelectionRegion> fixedTemperatureRegion;
    double heatFlux = 0.0;
    std::optional<PlanarSelectionRegion> heatFluxRegion;
    double filmCoefficient = 0.0;
    double ambientTemperature = 20.0;
    std::optional<PlanarSelectionRegion> convectionRegion;
    for (const CaeBoundaryCondition& condition : study->boundaryConditions()) {
        const CaeNamedSelection* target = study->findNamedSelection(condition.targetName());
        if (!target || !target->planarRegion()) {
            study->setState(StudyState::Failed);
            return QStringLiteral("Boundary condition target is no longer available: %1.")
                .arg(condition.targetName());
        }
        switch (condition.type()) {
        case BoundaryConditionType::FixedSupport:
            fixedRegion = target->planarRegion();
            break;
        case BoundaryConditionType::Force:
            for (int axis = 0; axis < 3; ++axis) {
                force[axis] += condition.components()[axis];
            }
            loadRegion = target->planarRegion();
            break;
        case BoundaryConditionType::Pressure:
            pressure = condition.value();
            pressureRegion = target->planarRegion();
            break;
        case BoundaryConditionType::FixedTemperature:
            fixedTemperature = condition.value();
            fixedTemperatureRegion = target->planarRegion();
            break;
        case BoundaryConditionType::HeatFlux:
            heatFlux = condition.value();
            heatFluxRegion = target->planarRegion();
            break;
        case BoundaryConditionType::Convection:
            filmCoefficient = condition.value();
            ambientTemperature = condition.referenceValue();
            convectionRegion = target->planarRegion();
            break;
        }
    }
    const CaeMaterial& material = study->materials().front();
    SolverRequest solverRequest;
    solverRequest.meshFilePath = study->mesh()->source();
    solverRequest.workingDirectory = m_services.externalToolConfig.workingDirectory();
    solverRequest.youngModulus = material.youngModulus();
    solverRequest.poissonRatio = material.poissonRatio();
    solverRequest.force = force;
    solverRequest.fixedRegion = fixedRegion;
    solverRequest.loadRegion = loadRegion;
    solverRequest.pressure = pressure;
    solverRequest.pressureRegion = pressureRegion;
    solverRequest.thermalConductivity = material.thermalConductivity();
    solverRequest.fixedTemperature = fixedTemperature;
    solverRequest.fixedTemperatureRegion = fixedTemperatureRegion;
    solverRequest.heatFlux = heatFlux;
    solverRequest.heatFluxRegion = heatFluxRegion;
    solverRequest.filmCoefficient = filmCoefficient;
    solverRequest.ambientTemperature = ambientTemperature;
    solverRequest.convectionRegion = convectionRegion;
    solverRequest.studyType = study->type();
    if (!m_services.solver->solve(solverRequest, &solverResult, &errorMessage)) {
        study->setSolution(CaeSolution(setup, SolutionStatus::Failed, errorMessage));
        study->setState(StudyState::Failed);
        return errorMessage;
    }

    study->setSolution(CaeSolution(
        setup,
        SolutionStatus::Solved,
        QStringLiteral("Analysis completed by %1.").arg(m_services.solver->name()),
        solverResult.resultFilePath,
        solverResult.logFilePath));
    study->setState(StudyState::Solved);
    return QStringLiteral("Solver completed for %1: %2 / %3.")
        .arg(study->name(), toDisplayString(setup.backend()), toDisplayString(setup.studyType()));
}

QString CaeController::showResult(ResultFieldType fieldType)
{
    CaeStudy* study = m_project->activeStudy();
    const QString validationError = CaeWorkflowValidator::requireSolved(study, fieldType);
    if (!validationError.isEmpty()) {
        return validationError;
    }

    QString unit;
    switch (fieldType) {
    case ResultFieldType::Displacement:
        unit = QStringLiteral("mm");
        break;
    case ResultFieldType::VonMisesStress:
        unit = QStringLiteral("MPa");
        break;
    case ResultFieldType::Temperature:
        unit = QStringLiteral("C");
        break;
    }

    ResultField resultField;
    resultField.type = fieldType;
    QString errorMessage;
    const CaeSolution& solution = *study->solution();
    const SolverResult solverResult{solution.resultFilePath(), solution.logFilePath()};
    if (!m_services.resultReader->read(solverResult, &resultField, &errorMessage)) {
        return errorMessage;
    }

    double minValue = 0.0;
    double maxValue = 0.0;
    if (!resultField.values.empty()) {
        const auto [minIt, maxIt] = std::minmax_element(resultField.values.begin(), resultField.values.end());
        minValue = *minIt;
        maxValue = *maxIt;
    }

    study->addResultField(CaeResultField(
        fieldType,
        unit,
        minValue,
        maxValue,
        QStringLiteral("Read by %1 from %2.")
            .arg(m_services.resultReader->name(), solution.resultFilePath()),
        std::move(resultField.nodalValues),
        std::move(resultField.nodalDisplacements)));

    return QStringLiteral("Result field prepared: %1.").arg(toDisplayString(fieldType));
}

QString CaeController::summary() const
{
    const CaeStudy* study = m_project->activeStudy();
    if (!study) {
        return QStringLiteral("No active CAE study.");
    }

    return QStringLiteral("%1 [%2]").arg(study->name(), toDisplayString(study->state()));
}

CaeProject& CaeController::project()
{
    return *m_project;
}

const CaeProject& CaeController::project() const
{
    return *m_project;
}

CaeExternalToolConfig& CaeController::externalToolConfig()
{
    return m_services.externalToolConfig;
}

const CaeExternalToolConfig& CaeController::externalToolConfig() const
{
    return m_services.externalToolConfig;
}

void CaeController::setExternalToolConfig(const CaeExternalToolConfig& config)
{
    m_services.externalToolConfig = config;
    CaeServiceFactory::configureExternalMeshGenerator(m_services);
    CaeServiceFactory::configureExternalSolver(m_services);
}

} // namespace Cae
