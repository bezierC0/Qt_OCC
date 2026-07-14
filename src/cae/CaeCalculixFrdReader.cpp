#include "CaeCalculixFrdReader.h"

#include <cmath>

#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

namespace Cae {

namespace {

enum class FrdDataset {
    None,
    Displacement,
    Stress
};

std::vector<double> numbers(const QString& line)
{
    static const QRegularExpression numberPattern(
        QStringLiteral(R"([+-]?(?:\d+\.?\d*|\.\d+)(?:[EeDd][+-]?\d+)?)"));
    std::vector<double> values;
    QRegularExpressionMatchIterator matches = numberPattern.globalMatch(line);
    while (matches.hasNext()) {
        QString value = matches.next().captured();
        value.replace(QLatin1Char('D'), QLatin1Char('E'));
        values.push_back(value.toDouble());
    }
    return values;
}

bool nodalValues(const QString& line, int* nodeId, std::vector<double>* values)
{
    if (!nodeId || !values) {
        return false;
    }

    static const QRegularExpression longValuePattern(
        QStringLiteral(R"([+-]?(?:\d+\.\d*|\.\d+)[EeDd][+-]\d{3})"));
    static const QRegularExpression shortValuePattern(
        QStringLiteral(R"([+-]?(?:\d+\.\d*|\.\d+)[EeDd][+-]\d{2})"));
    const auto readFixedRecord = [&](int nodeFieldWidth, const QRegularExpression& valuePattern) {
        bool nodeIdValid = false;
        *nodeId = line.mid(3, nodeFieldWidth).trimmed().toInt(&nodeIdValid);
        if (!nodeIdValid) {
            return false;
        }

        values->clear();
        QRegularExpressionMatchIterator matches = valuePattern.globalMatch(line.mid(3 + nodeFieldWidth));
        while (matches.hasNext()) {
            QString value = matches.next().captured();
            value.replace(QLatin1Char('D'), QLatin1Char('E'));
            values->push_back(value.toDouble());
        }
        return !values->empty();
    };

    if (readFixedRecord(10, longValuePattern) || readFixedRecord(5, shortValuePattern)) {
        return true;
    }

    const std::vector<double> separatedValues = numbers(line);
    if (separatedValues.size() < 3) {
        return false;
    }
    *nodeId = static_cast<int>(separatedValues[1]);
    values->assign(separatedValues.cbegin() + 2, separatedValues.cend());
    return true;
}

double vonMises(const std::vector<double>& values)
{
    const double sxx = values[0];
    const double syy = values[1];
    const double szz = values[2];
    const double sxy = values[3];
    const double syz = values[4];
    const double szx = values[5];
    return std::sqrt(0.5 * (
        (sxx - syy) * (sxx - syy) +
        (syy - szz) * (syy - szz) +
        (szz - sxx) * (szz - sxx) +
        6.0 * (sxy * sxy + syz * syz + szx * szx)));
}

} // namespace

QString CalculixFrdReader::name() const
{
    return QStringLiteral("CalculiX FRD Reader");
}

bool CalculixFrdReader::read(
    const SolverResult& solverResult,
    ResultField* field,
    QString* errorMessage)
{
    if (!field) {
        if (errorMessage) *errorMessage = QStringLiteral("FRD result field output is null.");
        return false;
    }
    if (field->type == ResultFieldType::Temperature) {
        if (errorMessage) *errorMessage = QStringLiteral("FRD temperature reading is not implemented yet.");
        return false;
    }

    QFile file(solverResult.resultFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) *errorMessage = QStringLiteral("Cannot open CalculiX FRD file: %1").arg(solverResult.resultFilePath);
        return false;
    }

    FrdDataset dataset = FrdDataset::None;
    QTextStream stream(&file);
    field->values.clear();
    field->nodalValues.clear();
    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        const QString trimmedLine = line.trimmed();
        if (trimmedLine.startsWith(QStringLiteral("-4"))) {
            if (line.contains(QStringLiteral("DISP"), Qt::CaseInsensitive)) {
                dataset = FrdDataset::Displacement;
            } else if (line.contains(QStringLiteral("STRESS"), Qt::CaseInsensitive)) {
                dataset = FrdDataset::Stress;
            } else {
                dataset = FrdDataset::None;
            }
            const bool wantsDisplacement = field->type == ResultFieldType::Displacement;
            if ((wantsDisplacement && dataset == FrdDataset::Displacement) ||
                (!wantsDisplacement && dataset == FrdDataset::Stress)) {
                field->values.clear();
                field->nodalValues.clear();
            }
            continue;
        }
        if (trimmedLine.startsWith(QStringLiteral("-3"))) {
            dataset = FrdDataset::None;
            continue;
        }
        if (!trimmedLine.startsWith(QStringLiteral("-1"))) {
            continue;
        }

        const bool wantsDisplacement = field->type == ResultFieldType::Displacement;
        if ((wantsDisplacement && dataset != FrdDataset::Displacement) ||
            (!wantsDisplacement && dataset != FrdDataset::Stress)) {
            continue;
        }

        int nodeId = 0;
        std::vector<double> components;
        if (!nodalValues(line, &nodeId, &components)) {
            continue;
        }
        if (wantsDisplacement && components.size() >= 3) {
            const double x = components[0];
            const double y = components[1];
            const double z = components[2];
            const double magnitude = std::sqrt(x * x + y * y + z * z);
            field->values.push_back(magnitude);
            field->nodalValues[nodeId] = magnitude;
        } else if (!wantsDisplacement && components.size() >= 6) {
            const double stress = vonMises({
                components[0], components[1], components[2],
                components[3], components[4], components[5]});
            field->values.push_back(stress);
            field->nodalValues[nodeId] = stress;
        }
    }

    if (field->values.empty()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("Requested result dataset was not found in FRD: %1")
                .arg(solverResult.resultFilePath);
        }
        return false;
    }
    return true;
}

} // namespace Cae
