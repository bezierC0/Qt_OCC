#pragma once

#include "CaeTypes.h"

#include <QString>

namespace Cae {

class CaeController;

class ICaeCommand {
public:
    virtual ~ICaeCommand() = default;
    virtual QString execute(CaeController& controller) = 0;
};

class CreateStudyCommand final : public ICaeCommand {
public:
    explicit CreateStudyCommand(StudyType type);
    QString execute(CaeController& controller) override;

private:
    StudyType m_type;
};

} // namespace Cae
