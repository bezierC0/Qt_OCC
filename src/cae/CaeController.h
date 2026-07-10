#pragma once

#include "CaeProject.h"
#include "CaeTypes.h"

#include <memory>

namespace Cae {

class ICaeCommand;

class CaeController {
public:
    CaeController();
    ~CaeController();

    QString execute(std::unique_ptr<ICaeCommand> command);

    QString createStudy(StudyType type);
    QString useCurrentGeometry();
    QString generateMesh();
    QString runSolver();
    QString showResult(ResultFieldType fieldType);
    QString summary() const;

    CaeProject& project();
    const CaeProject& project() const;

private:
    QString requireActiveStudyMessage() const;

private:
    std::unique_ptr<CaeProject> m_project;
};

} // namespace Cae
