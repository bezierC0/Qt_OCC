#include "CaeWorkflowValidator.h"

namespace Cae {

QString CaeWorkflowValidator::requireActiveStudy(const CaeStudy* study)
{
    if (!study) {
        return QStringLiteral("Create a CAE study first.");
    }

    return QString();
}

QString CaeWorkflowValidator::requireGeometryReady(const CaeStudy* study)
{
    const QString activeStudyError = requireActiveStudy(study);
    if (!activeStudyError.isEmpty()) {
        return activeStudyError;
    }

    if (study->state() == StudyState::Empty) {
        return QStringLiteral("Select geometry before continuing the CAE workflow.");
    }

    return QString();
}

QString CaeWorkflowValidator::requireNamedSelection(const CaeStudy* study)
{
    const QString geometryError = requireGeometryReady(study);
    if (!geometryError.isEmpty()) {
        return geometryError;
    }

    if (study->namedSelections().empty()) {
        return QStringLiteral("Create a named selection before continuing the CAE workflow.");
    }

    return QString();
}

QString CaeWorkflowValidator::requireMeshInputs(const CaeStudy* study)
{
    const QString namedSelectionError = requireNamedSelection(study);
    if (!namedSelectionError.isEmpty()) {
        return namedSelectionError;
    }

    if (study->materials().empty()) {
        return QStringLiteral("Assign a material before generating the mesh.");
    }

    return QString();
}

QString CaeWorkflowValidator::requireMeshReady(const CaeStudy* study)
{
    const QString activeStudyError = requireActiveStudy(study);
    if (!activeStudyError.isEmpty()) {
        return activeStudyError;
    }

    if (study->state() != StudyState::Meshed && study->state() != StudyState::Solved) {
        return QStringLiteral("Generate mesh before running solver.");
    }

    bool hasConstraint = false;
    bool hasLoad = false;
    for (const CaeBoundaryCondition& condition : study->boundaryConditions()) {
        hasConstraint = hasConstraint || condition.type() == BoundaryConditionType::FixedSupport;
        hasLoad = hasLoad || condition.type() == BoundaryConditionType::Force;
    }

    if (!hasConstraint) {
        return QStringLiteral("Add a fixed support before running solver.");
    }

    if (study->type() == StudyType::StaticStructural && !hasLoad) {
        return QStringLiteral("Add a force before running a static structural analysis.");
    }

    return QString();
}

QString CaeWorkflowValidator::requireSolved(const CaeStudy* study, ResultFieldType fieldType)
{
    const QString activeStudyError = requireActiveStudy(study);
    if (!activeStudyError.isEmpty()) {
        return activeStudyError;
    }

    if (study->state() != StudyState::Solved) {
        return QStringLiteral("Run solver before showing %1.").arg(toDisplayString(fieldType));
    }

    if (!study->solution() || study->solution()->status() != SolutionStatus::Solved) {
        return QStringLiteral("The solver did not produce a valid solution.");
    }

    const bool thermalField = fieldType == ResultFieldType::Temperature;
    if (study->type() == StudyType::SteadyThermal && !thermalField) {
        return QStringLiteral("Thermal studies provide temperature results.");
    }
    if (study->type() == StudyType::StaticStructural && thermalField) {
        return QStringLiteral("Static structural studies provide displacement and stress results.");
    }

    return QString();
}

} // namespace Cae
