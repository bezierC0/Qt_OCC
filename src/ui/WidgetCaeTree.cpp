#include "WidgetCaeTree.h"

#include "cae/CaeProject.h"
#include "cae/CaeStudy.h"
#include "cae/CaeTypes.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

CaeTreeWidget::CaeTreeWidget(QWidget* parent)
    : QWidget(parent)
{
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setColumnCount(2);
    m_treeWidget->setHeaderLabels({tr("CAE Item"), tr("State")});

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(m_treeWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);
}

void CaeTreeWidget::setProject(const Cae::CaeProject& project)
{
    m_treeWidget->clear();

    auto* root = new QTreeWidgetItem(m_treeWidget, {tr("CAE Project"), QString()});
    for (const auto& study : project.studies()) {
        if (study) {
            addStudyItem(root, *study);
        }
    }

    root->setExpanded(true);
    m_treeWidget->resizeColumnToContents(0);
}

void CaeTreeWidget::clearProject()
{
    m_treeWidget->clear();
}

void CaeTreeWidget::addStudyItem(QTreeWidgetItem* parent, const Cae::CaeStudy& study)
{
    auto* studyItem = new QTreeWidgetItem(parent, {study.name(), Cae::toDisplayString(study.state())});
    studyItem->setExpanded(true);

    new QTreeWidgetItem(studyItem, {tr("Geometry"), study.state() == Cae::StudyState::Empty ? tr("Not selected") : tr("Ready")});
    auto* namedSelectionsItem = new QTreeWidgetItem(
        studyItem,
        {tr("Named Selections"), study.namedSelections().empty() ? tr("Pending") : tr("%1 defined").arg(static_cast<int>(study.namedSelections().size()))});
    for (const auto& namedSelection : study.namedSelections()) {
        new QTreeWidgetItem(namedSelectionsItem, {namedSelection.name(), Cae::toDisplayString(namedSelection.scope())});
    }

    auto* materialsItem = new QTreeWidgetItem(
        studyItem,
        {tr("Materials"), study.materials().empty() ? tr("Pending") : tr("%1 assigned").arg(static_cast<int>(study.materials().size()))});
    for (const auto& material : study.materials()) {
        auto* materialItem = new QTreeWidgetItem(materialsItem, {material.name(), tr("Target: %1").arg(material.targetName())});
        new QTreeWidgetItem(materialItem, {tr("Young's Modulus"), tr("%1 MPa").arg(material.youngModulus())});
        new QTreeWidgetItem(materialItem, {tr("Poisson Ratio"), QString::number(material.poissonRatio())});
    }

    auto* boundaryItem = new QTreeWidgetItem(
        studyItem,
        {tr("Boundary Conditions"), study.boundaryConditions().empty() ? tr("Pending") : tr("%1 defined").arg(static_cast<int>(study.boundaryConditions().size()))});
    for (const auto& boundaryCondition : study.boundaryConditions()) {
        new QTreeWidgetItem(boundaryItem, {boundaryCondition.name(), boundaryCondition.summary()});
    }

    const bool hasMesh = study.state() == Cae::StudyState::Meshed ||
                         study.state() == Cae::StudyState::Solving ||
                         study.state() == Cae::StudyState::Solved;
    auto* meshItem = new QTreeWidgetItem(studyItem, {tr("Mesh"), hasMesh ? tr("Ready") : tr("Pending")});
    if (study.mesh()) {
        const auto& mesh = *study.mesh();
        new QTreeWidgetItem(meshItem, {tr("Maximum Element Size"), QString::number(mesh.setup().globalSize())});
        new QTreeWidgetItem(meshItem, {tr("Element Order"), Cae::toDisplayString(mesh.setup().elementOrder())});
        new QTreeWidgetItem(meshItem, {tr("Nodes"), QString::number(mesh.nodeCount())});
        new QTreeWidgetItem(meshItem, {tr("Surface Elements"), QString::number(mesh.surfaceElementCount())});
        new QTreeWidgetItem(meshItem, {tr("Volume Elements"), QString::number(mesh.volumeElementCount())});
        new QTreeWidgetItem(meshItem, {tr("Supported Elements"), QString::number(mesh.elementCount())});
        new QTreeWidgetItem(meshItem, {tr("Source"), mesh.source()});
    }

    const bool hasSolution = study.state() == Cae::StudyState::Solved;
    auto* solutionItem = new QTreeWidgetItem(studyItem, {tr("Solution"), hasSolution ? tr("Solved") : tr("Pending")});
    if (study.solution()) {
        const auto& solution = *study.solution();
        new QTreeWidgetItem(solutionItem, {tr("Solver"), Cae::toDisplayString(solution.setup().backend())});
        new QTreeWidgetItem(solutionItem, {tr("Analysis"), Cae::toDisplayString(solution.setup().studyType())});
        new QTreeWidgetItem(solutionItem, {tr("Status"), Cae::toDisplayString(solution.status())});
        new QTreeWidgetItem(solutionItem, {tr("Summary"), solution.summary()});
        if (!solution.resultFilePath().isEmpty()) {
            new QTreeWidgetItem(solutionItem, {tr("Result File"), solution.resultFilePath()});
        }
        if (!solution.logFilePath().isEmpty()) {
            new QTreeWidgetItem(solutionItem, {tr("Log File"), solution.logFilePath()});
        }
    }
    auto* resultsItem = new QTreeWidgetItem(studyItem, {tr("Results"), study.result() ? tr("Available") : tr("Pending")});
    if (study.result()) {
        for (const auto& field : study.result()->fields()) {
            auto* fieldItem = new QTreeWidgetItem(resultsItem, {Cae::toDisplayString(field.type()), field.unit()});
            new QTreeWidgetItem(fieldItem, {tr("Min"), QString::number(field.minValue())});
            new QTreeWidgetItem(fieldItem, {tr("Max"), QString::number(field.maxValue())});
            new QTreeWidgetItem(fieldItem, {tr("Nodal Values"), QString::number(static_cast<qulonglong>(field.nodalValues().size()))});
            new QTreeWidgetItem(fieldItem, {tr("Source"), field.source()});
        }
    }
}
