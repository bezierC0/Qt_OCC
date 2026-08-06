#pragma once

#include "CaeStudy.h"

#include <memory>
#include <vector>

namespace Cae {

class CaeProject {
public:
    CaeStudy& createStudy(StudyType type);
    void clear();

    CaeStudy* activeStudy();
    const CaeStudy* activeStudy() const;
    bool activateStudy(const QUuid& studyId);

    const std::vector<std::unique_ptr<CaeStudy>>& studies() const;

private:
    std::vector<std::unique_ptr<CaeStudy>> m_studies;
    CaeStudy* m_activeStudy{nullptr};
};

} // namespace Cae
