#include "CaeStudy.h"

#include <algorithm>
#include <utility>

namespace Cae {

CaeStudy::CaeStudy(StudyType type)
    : m_id(QUuid::createUuid())
    , m_type(type)
    , m_name(toDisplayString(type))
{
}

QUuid CaeStudy::id() const
{
    return m_id;
}

StudyType CaeStudy::type() const
{
    return m_type;
}

StudyState CaeStudy::state() const
{
    return m_state;
}

QString CaeStudy::name() const
{
    return m_name;
}

const std::vector<CaeNamedSelection>& CaeStudy::namedSelections() const
{
    return m_namedSelections;
}

const CaeNamedSelection* CaeStudy::findNamedSelection(const QString& name) const
{
    const auto selection = std::find_if(
        m_namedSelections.cbegin(),
        m_namedSelections.cend(),
        [&name](const CaeNamedSelection& item) { return item.name() == name; });
    return selection == m_namedSelections.cend() ? nullptr : &*selection;
}

const std::vector<CaeMaterial>& CaeStudy::materials() const
{
    return m_materials;
}

const std::vector<CaeBoundaryCondition>& CaeStudy::boundaryConditions() const
{
    return m_boundaryConditions;
}

const std::optional<CaeMesh>& CaeStudy::mesh() const
{
    return m_mesh;
}

const std::optional<CaeSolution>& CaeStudy::solution() const
{
    return m_solution;
}

const std::optional<CaeResult>& CaeStudy::result() const
{
    return m_result;
}

void CaeStudy::setName(const QString& name)
{
    m_name = name;
}

void CaeStudy::setState(StudyState state)
{
    m_state = state;
}

void CaeStudy::resetForGeometry()
{
    m_namedSelections.clear();
    m_materials.clear();
    m_boundaryConditions.clear();
    m_mesh.reset();
    m_solution.reset();
    m_result.reset();
    m_state = StudyState::GeometryReady;
}

void CaeStudy::addNamedSelection(const CaeNamedSelection& namedSelection)
{
    invalidateMeshAndSolution();
    const auto existing = std::find_if(
        m_namedSelections.begin(),
        m_namedSelections.end(),
        [&namedSelection](const CaeNamedSelection& item) {
            return item.name() == namedSelection.name();
        });
    if (existing != m_namedSelections.end()) {
        *existing = namedSelection;
        return;
    }
    m_namedSelections.push_back(namedSelection);
}

bool CaeStudy::removeNamedSelection(const QString& name)
{
    const auto selection = std::find_if(
        m_namedSelections.begin(),
        m_namedSelections.end(),
        [&name](const CaeNamedSelection& item) {
            return item.name() == name;
        });
    if (selection == m_namedSelections.end()) {
        return false;
    }

    invalidateMeshAndSolution();
    m_materials.erase(
        std::remove_if(
            m_materials.begin(),
            m_materials.end(),
            [&name](const CaeMaterial& item) {
                return item.targetName() == name;
            }),
        m_materials.end());
    m_boundaryConditions.erase(
        std::remove_if(
            m_boundaryConditions.begin(),
            m_boundaryConditions.end(),
            [&name](const CaeBoundaryCondition& item) {
                return item.targetName() == name;
            }),
        m_boundaryConditions.end());
    m_namedSelections.erase(selection);
    return true;
}

void CaeStudy::addMaterial(const CaeMaterial& material)
{
    invalidateSolution();
    const auto existing = std::find_if(
        m_materials.begin(),
        m_materials.end(),
        [&material](const CaeMaterial& item) {
            return item.targetName() == material.targetName();
        });
    if (existing != m_materials.end()) {
        *existing = material;
        return;
    }
    m_materials.push_back(material);
}

bool CaeStudy::removeMaterial(const QString& name)
{
    const auto material = std::find_if(
        m_materials.begin(),
        m_materials.end(),
        [&name](const CaeMaterial& item) {
            return item.name() == name;
        });
    if (material == m_materials.end()) {
        return false;
    }

    invalidateSolution();
    m_materials.erase(material);
    return true;
}

void CaeStudy::addBoundaryCondition(const CaeBoundaryCondition& boundaryCondition)
{
    invalidateSolution();
    const auto existing = std::find_if(
        m_boundaryConditions.begin(),
        m_boundaryConditions.end(),
        [&boundaryCondition](const CaeBoundaryCondition& item) {
            return item.type() == boundaryCondition.type();
        });
    if (existing != m_boundaryConditions.end()) {
        *existing = boundaryCondition;
        return;
    }
    m_boundaryConditions.push_back(boundaryCondition);
}

bool CaeStudy::removeBoundaryCondition(const QString& name)
{
    const auto condition = std::find_if(
        m_boundaryConditions.begin(),
        m_boundaryConditions.end(),
        [&name](const CaeBoundaryCondition& item) {
            return item.name() == name;
        });
    if (condition == m_boundaryConditions.end()) {
        return false;
    }

    invalidateSolution();
    m_boundaryConditions.erase(condition);
    return true;
}

void CaeStudy::setMesh(const CaeMesh& mesh)
{
    m_solution.reset();
    m_result.reset();
    m_mesh = mesh;
}

void CaeStudy::setSolution(const CaeSolution& solution)
{
    m_result.reset();
    m_solution = solution;
}

void CaeStudy::addResultField(CaeResultField field)
{
    if (!m_result) {
        m_result = CaeResult();
    }
    m_result->addOrReplaceField(std::move(field));
}

void CaeStudy::invalidateMeshAndSolution()
{
    if (m_mesh || m_solution || m_result || m_state == StudyState::Failed) {
        m_mesh.reset();
        m_solution.reset();
        m_result.reset();
        m_state = StudyState::GeometryReady;
    }
}

void CaeStudy::invalidateSolution()
{
    if (m_solution || m_result || m_state == StudyState::Solving ||
        m_state == StudyState::Solved || m_state == StudyState::Failed) {
        m_solution.reset();
        m_result.reset();
        m_state = m_mesh ? StudyState::Meshed : StudyState::GeometryReady;
    }
}

} // namespace Cae
