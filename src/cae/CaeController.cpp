#include "CaeController.h"

#include "CaeCommand.h"

namespace Cae {

CaeController::CaeController()
    : m_project(std::make_unique<CaeProject>())
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

QString CaeController::createStudy(StudyType type)
{
    CaeStudy& study = m_project->createStudy(type);
    return QStringLiteral("Created CAE study: %1").arg(study.name());
}

QString CaeController::useCurrentGeometry()
{
    CaeStudy* study = m_project->activeStudy();
    if (!study) {
        return requireActiveStudyMessage();
    }

    study->setState(StudyState::GeometryReady);
    return QStringLiteral("%1 uses the current CAD geometry.").arg(study->name());
}

QString CaeController::generateMesh()
{
    CaeStudy* study = m_project->activeStudy();
    if (!study) {
        return requireActiveStudyMessage();
    }

    if (study->state() == StudyState::Empty) {
        return QStringLiteral("Select geometry before generating mesh.");
    }

    study->setState(StudyState::Meshed);
    return QStringLiteral("Mesh generation step is prepared for %1.").arg(study->name());
}

QString CaeController::runSolver()
{
    CaeStudy* study = m_project->activeStudy();
    if (!study) {
        return requireActiveStudyMessage();
    }

    if (study->state() != StudyState::Meshed && study->state() != StudyState::Solved) {
        return QStringLiteral("Generate mesh before running solver.");
    }

    study->setState(StudyState::Solved);
    return QStringLiteral("Solver step is prepared for %1.").arg(study->name());
}

QString CaeController::showResult(ResultFieldType fieldType)
{
    const CaeStudy* study = m_project->activeStudy();
    if (!study) {
        return requireActiveStudyMessage();
    }

    if (study->state() != StudyState::Solved) {
        return QStringLiteral("Run solver before showing %1.").arg(toDisplayString(fieldType));
    }

    return QStringLiteral("Result display step is prepared: %1.").arg(toDisplayString(fieldType));
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

QString CaeController::requireActiveStudyMessage() const
{
    return QStringLiteral("Create a CAE study first.");
}

} // namespace Cae
