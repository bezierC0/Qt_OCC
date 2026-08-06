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

void CaeProject::clear()
{
    m_activeStudy = nullptr;
    m_studies.clear();
}

CaeStudy* CaeProject::activeStudy()
{
    return m_activeStudy;
}

const CaeStudy* CaeProject::activeStudy() const
{
    return m_activeStudy;
}

bool CaeProject::activateStudy(const QUuid& studyId)
{
    for (const auto& study : m_studies) {
        if (study && study->id() == studyId) {
            m_activeStudy = study.get();
            return true;
        }
    }
    return false;
}

const std::vector<std::unique_ptr<CaeStudy>>& CaeProject::studies() const
{
    return m_studies;
}

} // namespace Cae
