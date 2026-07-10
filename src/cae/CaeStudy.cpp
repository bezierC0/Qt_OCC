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

void CaeStudy::setName(const QString& name)
{
    m_name = name;
}

void CaeStudy::setState(StudyState state)
{
    m_state = state;
}

} // namespace Cae
