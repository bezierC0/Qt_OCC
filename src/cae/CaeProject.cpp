#include "CaeProject.h"

namespace Cae {

CaeStudy& CaeProject::createStudy(StudyType type)
{
    auto study = std::make_unique<CaeStudy>(type);
    CaeStudy& ref = *study;
    m_activeStudy = &ref;
    m_studies.emplace_back(std::move(study));
    return ref;
}

CaeStudy* CaeProject::activeStudy()
{
    return m_activeStudy;
}

const CaeStudy* CaeProject::activeStudy() const
{
    return m_activeStudy;
}

const std::vector<std::unique_ptr<CaeStudy>>& CaeProject::studies() const
{
    return m_studies;
}

} // namespace Cae
