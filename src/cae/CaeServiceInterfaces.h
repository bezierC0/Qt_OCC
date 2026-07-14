#pragma once

#include "CaeTypes.h"

#include <QString>
#include <map>
#include <vector>

namespace Cae {

struct MeshRequest {
    double globalSize{1.0};
    QString geometryFilePath;
};

struct SolverRequest {
    QString meshFilePath;
    QString workingDirectory;
    double youngModulus{210000.0};
    double poissonRatio{0.3};
    double force{100.0};
    StudyType studyType{StudyType::StaticStructural};
};

struct MeshResult {
    QString meshFilePath;
    int nodeCount{0};
    int elementCount{0};
};

struct SolverResult {
    QString resultFilePath;
    QString logFilePath;
};

struct ResultField {
    ResultFieldType type{ResultFieldType::Displacement};
    std::vector<double> values;
    std::map<int, double> nodalValues;
};

struct ResultRenderOptions {
    ResultFieldType fieldType{ResultFieldType::Displacement};
    double deformationScale{1.0};
};

class IMeshGenerator {
public:
    virtual ~IMeshGenerator() = default;
    virtual QString name() const = 0;
    virtual bool generate(const MeshRequest& request, MeshResult* result, QString* errorMessage) = 0;
};

class ISolver {
public:
    virtual ~ISolver() = default;
    virtual QString name() const = 0;
    virtual bool solve(const SolverRequest& request, SolverResult* result, QString* errorMessage) = 0;
};

class IResultReader {
public:
    virtual ~IResultReader() = default;
    virtual QString name() const = 0;
    virtual bool read(const SolverResult& solverResult, ResultField* field, QString* errorMessage) = 0;
};

class IResultRenderer {
public:
    virtual ~IResultRenderer() = default;
    virtual QString name() const = 0;
    virtual bool render(const ResultField& field, const ResultRenderOptions& options, QString* errorMessage) = 0;
};

} // namespace Cae
