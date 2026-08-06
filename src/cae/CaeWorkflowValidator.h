#pragma once

#include "CaeStudy.h"

#include <QString>

namespace Cae {

class CaeWorkflowValidator {
public:
    static QString requireActiveStudy(const CaeStudy* study);
    static QString requireGeometryReady(const CaeStudy* study);
    static QString requireNamedSelection(const CaeStudy* study);
    static QString requireMeshInputs(const CaeStudy* study);
    static QString requireMeshReady(const CaeStudy* study);
    static QString requireSolved(const CaeStudy* study, ResultFieldType fieldType);
};

} // namespace Cae
