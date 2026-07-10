#pragma once

#include <QString>

namespace Cae {

enum class NamedSelectionScope {
    Geometry,
    Face,
    Edge,
    Vertex
};

class CaeNamedSelection {
public:
    CaeNamedSelection(QString name, NamedSelectionScope scope);

    QString name() const;
    NamedSelectionScope scope() const;

private:
    QString m_name;
    NamedSelectionScope m_scope{NamedSelectionScope::Geometry};
};

QString toDisplayString(NamedSelectionScope scope);

} // namespace Cae
