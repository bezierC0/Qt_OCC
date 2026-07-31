#pragma once

#include "CaeTypes.h"
#include "CaeMaterial.h"
#include "CaeBoundaryCondition.h"
#include "CaeNamedSelection.h"
#include "CaeMesh.h"
#include "CaeSolution.h"
#include "CaeResult.h"

#include <QString>
#include <QUuid>
#include <optional>
#include <vector>

namespace Cae {

class CaeStudy {
public:
    explicit CaeStudy(StudyType type);

    QUuid id() const;
    StudyType type() const;
    StudyState state() const;
    QString name() const;
    const std::vector<CaeNamedSelection>& namedSelections() const;
    const CaeNamedSelection* findNamedSelection(const QString& name) const;
    const std::vector<CaeMaterial>& materials() const;
    const std::vector<CaeBoundaryCondition>& boundaryConditions() const;
    const std::optional<CaeMesh>& mesh() const;
    const std::optional<CaeSolution>& solution() const;
    const std::optional<CaeResult>& result() const;

    void setName(const QString& name);
    void setState(StudyState state);
    void resetForGeometry();
    void addNamedSelection(const CaeNamedSelection& namedSelection);
    bool removeNamedSelection(const QString& name);
    void addMaterial(const CaeMaterial& material);
    bool removeMaterial(const QString& name);
    void addBoundaryCondition(const CaeBoundaryCondition& boundaryCondition);
    bool removeBoundaryCondition(const QString& name);
    void setMesh(const CaeMesh& mesh);
    void setSolution(const CaeSolution& solution);
    void addResultField(CaeResultField field);

private:
    void invalidateMeshAndSolution();
    void invalidateSolution();

    QUuid m_id;
    StudyType m_type;
    StudyState m_state{StudyState::Empty};
    QString m_name;
    std::vector<CaeNamedSelection> m_namedSelections;
    std::vector<CaeMaterial> m_materials;
    std::vector<CaeBoundaryCondition> m_boundaryConditions;
    std::optional<CaeMesh> m_mesh;
    std::optional<CaeSolution> m_solution;
    std::optional<CaeResult> m_result;
};

} // namespace Cae
