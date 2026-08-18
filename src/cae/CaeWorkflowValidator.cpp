#include "CaeWorkflowValidator.h"

#include <QStringList>

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
    const CaeMaterial& material = study->materials().front();
    if (study->type() == StudyType::StaticStructural && material.youngModulus() <= 0.0) {
        return QStringLiteral("Assign an elastic material before generating the mesh.");
    }
    if (study->type() == StudyType::SteadyThermal && material.thermalConductivity() <= 0.0) {
        return QStringLiteral("Assign a thermal material before generating the mesh.");
    }

    return QString();
}

QString CaeWorkflowValidator::requireMeshReady(const CaeStudy* study)
{
    const QString meshInputError = requireMeshInputs(study);
    if (!meshInputError.isEmpty()) {
        return meshInputError;
    }

    if (study->state() != StudyState::Meshed && study->state() != StudyState::Solved) {
        return QStringLiteral("Generate mesh before running solver.");
    }

    bool hasConstraint = false;
    bool hasLoad = false;
    QString fixedTarget;
    QStringList loadTargets;
    for (const CaeBoundaryCondition& condition : study->boundaryConditions()) {
        const bool structuralConstraint =
            condition.type() == BoundaryConditionType::FixedSupport;
        const bool thermalConstraint =
            condition.type() == BoundaryConditionType::FixedTemperature ||
            condition.type() == BoundaryConditionType::Convection;
        const bool structuralLoad =
            condition.type() == BoundaryConditionType::Force ||
            condition.type() == BoundaryConditionType::Pressure;
        const bool thermalLoad =
            condition.type() == BoundaryConditionType::HeatFlux ||
            condition.type() == BoundaryConditionType::Convection ||
            condition.type() == BoundaryConditionType::HeatGeneration;
        hasConstraint = hasConstraint ||
            (study->type() == StudyType::StaticStructural && structuralConstraint) ||
            (study->type() == StudyType::SteadyThermal && thermalConstraint);
        hasLoad = hasLoad ||
            (study->type() == StudyType::StaticStructural && structuralLoad) ||
            (study->type() == StudyType::SteadyThermal && thermalLoad);
        const CaeNamedSelection* target = study->findNamedSelection(condition.targetName());
        const bool geometryLoad =
            condition.type() == BoundaryConditionType::HeatGeneration &&
            target && target->scope() == NamedSelectionScope::Geometry;
        if (!target || (!geometryLoad && !target->planarRegion())) {
            return QStringLiteral("Boundary condition target is invalid: %1.")
                .arg(condition.targetName());
        }
        if (structuralConstraint ||
            condition.type() == BoundaryConditionType::FixedTemperature) {
            fixedTarget = condition.targetName();
        }
        if (structuralLoad || thermalLoad) {
            loadTargets.push_back(condition.targetName());
        }
    }

    if (!hasConstraint) {
        return study->type() == StudyType::StaticStructural
            ? QStringLiteral("Add a fixed support before running solver.")
            : QStringLiteral("Add a fixed temperature or convection before running solver.");
    }

    if (!hasLoad) {
        return study->type() == StudyType::StaticStructural
            ? QStringLiteral("Add a force or pressure before running a static structural analysis.")
            : QStringLiteral("Add a heat flux, convection or heat generation before running a steady thermal analysis.");
    }
    if (!fixedTarget.isEmpty() && loadTargets.contains(fixedTarget)) {
        return QStringLiteral("Constraint and surface load must target different named faces.");
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
