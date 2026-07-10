#include "CaeStudy.h"

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
    m_namedSelections.push_back(namedSelection);
}

void CaeStudy::addMaterial(const CaeMaterial& material)
{
    invalidateMeshAndSolution();
    m_materials.push_back(material);
}

void CaeStudy::addBoundaryCondition(const CaeBoundaryCondition& boundaryCondition)
{
    invalidateSolution();
    m_boundaryConditions.push_back(boundaryCondition);
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

void CaeStudy::addResultField(const CaeResultField& field)
{
    if (!m_result) {
        m_result = CaeResult();
    }
    m_result->addOrReplaceField(field);
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
