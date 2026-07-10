#include "CaeNamedSelection.h"

#include <utility>

namespace Cae {

CaeNamedSelection::CaeNamedSelection(QString name, NamedSelectionScope scope)
    : m_name(std::move(name))
    , m_scope(scope)
{
}

QString CaeNamedSelection::name() const
{
    return m_name;
}

NamedSelectionScope CaeNamedSelection::scope() const
{
    return m_scope;
}

QString toDisplayString(NamedSelectionScope scope)
{
    switch (scope) {
    case NamedSelectionScope::Geometry:
        return QStringLiteral("Geometry");
    case NamedSelectionScope::Face:
        return QStringLiteral("Face");
    case NamedSelectionScope::Edge:
        return QStringLiteral("Edge");
    case NamedSelectionScope::Vertex:
        return QStringLiteral("Vertex");
    }
    return QStringLiteral("Unknown Scope");
}

} // namespace Cae
