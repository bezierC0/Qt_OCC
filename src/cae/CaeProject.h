#pragma once

#include "CaeStudy.h"

#include <memory>
#include <vector>

namespace Cae {

class CaeProject {
public:
    CaeStudy& createStudy(StudyType type);

    CaeStudy* activeStudy();
    const CaeStudy* activeStudy() const;

    const std::vector<std::unique_ptr<CaeStudy>>& studies() const;

private:
    std::vector<std::unique_ptr<CaeStudy>> m_studies;
    CaeStudy* m_activeStudy{nullptr};
};

} // namespace Cae
