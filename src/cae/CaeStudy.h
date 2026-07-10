#pragma once

#include "CaeTypes.h"

#include <QString>
#include <QUuid>

namespace Cae {

class CaeStudy {
public:
    explicit CaeStudy(StudyType type);

    QUuid id() const;
    StudyType type() const;
    StudyState state() const;
    QString name() const;

    void setName(const QString& name);
    void setState(StudyState state);

private:
    QUuid m_id;
    StudyType m_type;
    StudyState m_state{StudyState::Empty};
    QString m_name;
};

} // namespace Cae
