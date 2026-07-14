#include "CaeController.h"

#include "CaeCommand.h"
#include "CaeWorkflowValidator.h"

#include <algorithm>
#include <utility>

namespace Cae {

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

QString CaeController::assignDefaultMaterial()
{
    CaeStudy* study = m_project->activeStudy();
    const QString validationError = CaeWorkflowValidator::requireNamedSelection(study);
    if (!validationError.isEmpty()) {
        return validationError;
    }

    const QString targetName = study->namedSelections().front().name();
    study->addMaterial(CaeMaterial(QStringLiteral("Default Steel"), targetName, 210000.0, 0.3));
    return QStringLiteral("Assigned material to %1: Default Steel -> %2.").arg(study->name(), targetName);
}

QString CaeController::addFixedSupport()
{
    CaeStudy* study = m_project->activeStudy();
    const QString validationError = CaeWorkflowValidator::requireNamedSelection(study);
    if (!validationError.isEmpty()) {
        return validationError;
    }

    const QString targetName = study->namedSelections().front().name();
    study->addBoundaryCondition(CaeBoundaryCondition(QStringLiteral("Fixed Support"), targetName, BoundaryConditionType::FixedSupport));
    return QStringLiteral("Added boundary condition to %1: Fixed Support -> %2.").arg(study->name(), targetName);
}

QString CaeController::addDefaultForce()
{
    CaeStudy* study = m_project->activeStudy();
    const QString validationError = CaeWorkflowValidator::requireNamedSelection(study);
    if (!validationError.isEmpty()) {
        return validationError;
    }

    const QString targetName = study->namedSelections().front().name();
    study->addBoundaryCondition(CaeBoundaryCondition(QStringLiteral("Default Force"), targetName, BoundaryConditionType::Force, 100.0, QStringLiteral("N")));
    return QStringLiteral("Added load to %1: Force 100 N -> %2.").arg(study->name(), targetName);
}

QString CaeController::generateMesh(const QString& geometryFilePath)
{
    CaeStudy* study = m_project->activeStudy();
    const QString validationError = CaeWorkflowValidator::requireMeshInputs(study);
    if (!validationError.isEmpty()) {
        return validationError;
    }

    const CaeMeshSetup setup(1.0, MeshElementOrder::First);
    MeshResult meshResult;
    QString errorMessage;
    if (!m_services.meshGenerator->generate(MeshRequest{setup.globalSize(), geometryFilePath}, &meshResult, &errorMessage)) {
        study->setState(StudyState::Failed);
        return errorMessage;
    }

    study->setMesh(CaeMesh(setup, meshResult.nodeCount, meshResult.elementCount, meshResult.meshFilePath));
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
    double force = 0.0;
    for (const CaeBoundaryCondition& condition : study->boundaryConditions()) {
        if (condition.type() == BoundaryConditionType::Force) {
            force += condition.value();
        }
    }
    const CaeMaterial& material = study->materials().front();
    const SolverRequest solverRequest{
        study->mesh()->source(),
        m_services.externalToolConfig.workingDirectory(),
        material.youngModulus(),
        material.poissonRatio(),
        force,
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
        std::move(resultField.nodalValues)));

    return QStringLiteral("Result field prepared: %1.").arg(toDisplayString(fieldType));
}

QString CaeController::runDemoAnalysis(bool hasGeometry, const QString& geometryFilePath)
{
    createStudy(StudyType::StaticStructural);

    QString message = useCurrentGeometry(hasGeometry);
    if (m_project->activeStudy()->state() != StudyState::GeometryReady) {
        return message;
    }

    createDefaultNamedSelection();
    assignDefaultMaterial();
    addFixedSupport();
    addDefaultForce();

    message = generateMesh(geometryFilePath);
    if (m_project->activeStudy()->state() == StudyState::Failed) {
        return message;
    }

    message = runSolver();
    if (m_project->activeStudy()->state() == StudyState::Failed) {
        return message;
    }

    showResult(ResultFieldType::Displacement);
    showResult(ResultFieldType::VonMisesStress);
    return QStringLiteral("Demo static CAE analysis completed with %1 and %2.")
        .arg(m_services.meshGenerator->name(), m_services.solver->name());
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
