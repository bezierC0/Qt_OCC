#include "CaeCommand.h"

#include "CaeController.h"

namespace Cae {

CreateStudyCommand::CreateStudyCommand(StudyType type)
    : m_type(type)
{
}

QString CreateStudyCommand::execute(CaeController& controller)
{
    return controller.createStudy(m_type);
}

} // namespace Cae
