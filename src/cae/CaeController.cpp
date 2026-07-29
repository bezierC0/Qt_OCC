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

QString CaeController::addFixedSupport(const QString& targetName)
{
    CaeStudy* study = m_project->activeStudy();
    const QString validationError = CaeWorkflowValidator::requireNamedSelection(study);
    if (!validationError.isEmpty()) {
        return validationError;
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
    for (const CaeBoundaryCondition& condition : study->boundaryConditions()) {
        const CaeNamedSelection* target = study->findNamedSelection(condition.targetName());
        if (!target || !target->planarRegion()) {
            study->setState(StudyState::Failed);
            return QStringLiteral("Boundary condition target is no longer available: %1.")
                .arg(condition.targetName());
        }
        if (condition.type() == BoundaryConditionType::FixedSupport) {
            fixedRegion = target->planarRegion();
        }
        if (condition.type() == BoundaryConditionType::Force) {
            for (int axis = 0; axis < 3; ++axis) {
                force[axis] += condition.components()[axis];
            }
            loadRegion = target->planarRegion();
        } else if (condition.type() == BoundaryConditionType::Pressure) {
            pressure = condition.value();
            pressureRegion = target->planarRegion();
        }
    }
    const CaeMaterial& material = study->materials().front();
    const SolverRequest solverRequest{
        study->mesh()->source(),
        m_services.externalToolConfig.workingDirectory(),
        material.youngModulus(),
        material.poissonRatio(),
        force,
        fixedRegion,
        loadRegion,
        pressure,
        pressureRegion,
        study->type()};
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
