#include "CaeServiceFactory.h"

#include "CaeDummyServices.h"
#include "CaeQtProcessRunner.h"

namespace Cae {

QString toDisplayString(CaeServiceProfile profile)
{
    switch (profile) {
    case CaeServiceProfile::Dummy:
        return QStringLiteral("Dummy");
    }

    return QStringLiteral("Unknown");
}

bool CaeServiceBundle::isValid() const
{
    return meshGenerator && solver && resultReader && processRunner;
}

CaeServiceBundle CaeServiceFactory::create(CaeServiceProfile profile)
{
    CaeServiceBundle bundle;
    bundle.profile = profile;

    switch (profile) {
    case CaeServiceProfile::Dummy:
        bundle.processRunner = std::make_unique<QtProcessRunner>();
        bundle.meshGenerator = std::make_unique<DummyMeshGenerator>();
        bundle.solver = std::make_unique<DummySolver>();
        bundle.resultReader = std::make_unique<DummyResultReader>();
        break;
    }

    return bundle;
}

} // namespace Cae
