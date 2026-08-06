#include "CaeServiceFactory.h"

#include "CaeDummyServices.h"
#include "CaeCalculixSolver.h"
#include "CaeCalculixFrdReader.h"
#include "CaeGmshMeshGenerator.h"
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

void CaeServiceFactory::configureExternalMeshGenerator(CaeServiceBundle& bundle)
{
    if (bundle.externalToolConfig.hasExecutablePath(ExternalTool::Gmsh) && bundle.processRunner) {
        bundle.meshGenerator = std::make_unique<GmshMeshGenerator>(
            bundle.externalToolConfig,
            bundle.processRunner.get());
        return;
    }

    bundle.meshGenerator = std::make_unique<DummyMeshGenerator>();
}

void CaeServiceFactory::configureExternalSolver(CaeServiceBundle& bundle)
{
    if (bundle.externalToolConfig.hasExecutablePath(ExternalTool::CalculiX) && bundle.processRunner) {
        bundle.solver = std::make_unique<CalculixSolver>(
            bundle.externalToolConfig,
            bundle.processRunner.get());
        bundle.resultReader = std::make_unique<CalculixFrdReader>();
        return;
    }

    bundle.solver = std::make_unique<DummySolver>();
    bundle.resultReader = std::make_unique<DummyResultReader>();
}

} // namespace Cae
