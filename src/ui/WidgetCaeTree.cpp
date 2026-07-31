#include "WidgetCaeTree.h"

#include "cae/CaeProject.h"
#include "cae/CaeStudy.h"
#include "cae/CaeTypes.h"

#include <QFont>
#include <QMenu>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace {

enum class CaeTreeItemKind {
    None,
    Mesh,
    ResultField,
    NamedSelection,
    Material,
    BoundaryCondition
};

constexpr int ItemKindRole = Qt::UserRole;
constexpr int ResultFieldTypeRole = Qt::UserRole + 1;
constexpr int StudyIdRole = Qt::UserRole + 2;
constexpr int ItemNameRole = Qt::UserRole + 3;

void setActionData(
    QTreeWidgetItem* item,
    CaeTreeItemKind kind,
    const QUuid& studyId)
{
    item->setData(0, ItemKindRole, static_cast<int>(kind));
    item->setData(0, StudyIdRole, studyId);
}

} // namespace

CaeTreeWidget::CaeTreeWidget(QWidget* parent)
    : QWidget(parent)
{
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setColumnCount(2);
    m_treeWidget->setHeaderLabels({tr("CAE Item"), tr("State")});
    m_treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(
        m_treeWidget,
        &QTreeWidget::itemActivated,
        this,
        [this](QTreeWidgetItem* item, int) { activateItem(item); });
    connect(
        m_treeWidget,
        &QTreeWidget::customContextMenuRequested,
        this,
        &CaeTreeWidget::showContextMenu);

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
            addStudyItem(root, *study, project.activeStudy() == study.get());
        }
    }

    root->setExpanded(true);
    m_treeWidget->resizeColumnToContents(0);
}

void CaeTreeWidget::clearProject()
{
    m_treeWidget->clear();
}

void CaeTreeWidget::activateItem(QTreeWidgetItem* item)
{
    if (!item) {
        return;
    }

    const auto kind = static_cast<CaeTreeItemKind>(item->data(0, ItemKindRole).toInt());
    const QUuid studyId = item->data(0, StudyIdRole).toUuid();
    switch (kind) {
    case CaeTreeItemKind::Mesh:
        emit meshActivated(studyId);
        break;
    case CaeTreeItemKind::ResultField:
        emit resultFieldActivated(
            studyId,
            static_cast<Cae::ResultFieldType>(
                item->data(0, ResultFieldTypeRole).toInt()));
        break;
    case CaeTreeItemKind::BoundaryCondition:
    case CaeTreeItemKind::NamedSelection:
    case CaeTreeItemKind::Material:
    case CaeTreeItemKind::None:
        break;
    }
}

void CaeTreeWidget::showContextMenu(const QPoint& position)
{
    QTreeWidgetItem* item = m_treeWidget->itemAt(position);
    if (!item) {
        return;
    }

    const auto kind = static_cast<CaeTreeItemKind>(item->data(0, ItemKindRole).toInt());
    QMenu menu(this);
    QAction* removeAction = nullptr;
    switch (kind) {
    case CaeTreeItemKind::NamedSelection:
        removeAction = menu.addAction(tr("Delete Named Selection"));
        break;
    case CaeTreeItemKind::Material:
        removeAction = menu.addAction(tr("Delete Material"));
        break;
    case CaeTreeItemKind::BoundaryCondition:
        removeAction = menu.addAction(tr("Delete Boundary Condition"));
        break;
    default:
        return;
    }
    if (menu.exec(m_treeWidget->viewport()->mapToGlobal(position)) != removeAction) {
        return;
    }

    const QUuid studyId = item->data(0, StudyIdRole).toUuid();
    const QString name = item->data(0, ItemNameRole).toString();
    switch (kind) {
    case CaeTreeItemKind::NamedSelection:
        emit removeNamedSelectionRequested(studyId, name);
        break;
    case CaeTreeItemKind::Material:
        emit removeMaterialRequested(studyId, name);
        break;
    case CaeTreeItemKind::BoundaryCondition:
        emit removeBoundaryConditionRequested(studyId, name);
        break;
    default:
        break;
    }
}

void CaeTreeWidget::addStudyItem(
    QTreeWidgetItem* parent,
    const Cae::CaeStudy& study,
    bool active)
{
    auto* studyItem = new QTreeWidgetItem(parent, {study.name(), Cae::toDisplayString(study.state())});
    QFont studyFont = studyItem->font(0);
    studyFont.setBold(active);
    studyItem->setFont(0, studyFont);
    if (active) {
        studyItem->setToolTip(0, tr("Active CAE study"));
    }
    studyItem->setExpanded(true);

    new QTreeWidgetItem(studyItem, {tr("Geometry"), study.state() == Cae::StudyState::Empty ? tr("Not selected") : tr("Ready")});
    auto* namedSelectionsItem = new QTreeWidgetItem(
        studyItem,
        {tr("Named Selections"), study.namedSelections().empty() ? tr("Pending") : tr("%1 defined").arg(static_cast<int>(study.namedSelections().size()))});
    for (const auto& namedSelection : study.namedSelections()) {
        auto* selectionItem = new QTreeWidgetItem(
            namedSelectionsItem,
            {namedSelection.name(), Cae::toDisplayString(namedSelection.scope())});
        if (namedSelection.scope() != Cae::NamedSelectionScope::Geometry) {
            setActionData(selectionItem, CaeTreeItemKind::NamedSelection, study.id());
            selectionItem->setData(0, ItemNameRole, namedSelection.name());
            selectionItem->setToolTip(0, tr("Right-click to delete this named selection."));
        }
    }

    auto* materialsItem = new QTreeWidgetItem(
        studyItem,
        {tr("Materials"), study.materials().empty() ? tr("Pending") : tr("%1 assigned").arg(static_cast<int>(study.materials().size()))});
    for (const auto& material : study.materials()) {
        auto* materialItem = new QTreeWidgetItem(materialsItem, {material.name(), tr("Target: %1").arg(material.targetName())});
        setActionData(materialItem, CaeTreeItemKind::Material, study.id());
        materialItem->setData(0, ItemNameRole, material.name());
        materialItem->setToolTip(0, tr("Right-click to delete this material."));
        new QTreeWidgetItem(materialItem, {tr("Young's Modulus"), tr("%1 MPa").arg(material.youngModulus())});
        new QTreeWidgetItem(materialItem, {tr("Poisson Ratio"), QString::number(material.poissonRatio())});
    }

    auto* boundaryItem = new QTreeWidgetItem(
        studyItem,
        {tr("Boundary Conditions"), study.boundaryConditions().empty() ? tr("Pending") : tr("%1 defined").arg(static_cast<int>(study.boundaryConditions().size()))});
    for (const auto& boundaryCondition : study.boundaryConditions()) {
        auto* conditionItem = new QTreeWidgetItem(
            boundaryItem,
            {boundaryCondition.name(), boundaryCondition.summary()});
        setActionData(conditionItem, CaeTreeItemKind::BoundaryCondition, study.id());
        conditionItem->setData(0, ItemNameRole, boundaryCondition.name());
        conditionItem->setToolTip(0, tr("Right-click to delete this boundary condition."));
    }

    const bool hasMesh = study.state() == Cae::StudyState::Meshed ||
                         study.state() == Cae::StudyState::Solving ||
                         study.state() == Cae::StudyState::Solved;
    auto* meshItem = new QTreeWidgetItem(studyItem, {tr("Mesh"), hasMesh ? tr("Ready") : tr("Pending")});
    if (study.mesh()) {
        setActionData(meshItem, CaeTreeItemKind::Mesh, study.id());
        meshItem->setToolTip(0, tr("Double-click to display the mesh."));
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
    resultsItem->setExpanded(study.result().has_value());
    if (study.result()) {
        for (const auto& field : study.result()->fields()) {
            auto* fieldItem = new QTreeWidgetItem(resultsItem, {Cae::toDisplayString(field.type()), field.unit()});
            setActionData(fieldItem, CaeTreeItemKind::ResultField, study.id());
            fieldItem->setData(0, ResultFieldTypeRole, static_cast<int>(field.type()));
            fieldItem->setToolTip(0, tr("Double-click to display this result."));
            new QTreeWidgetItem(fieldItem, {tr("Min"), QString::number(field.minValue())});
            new QTreeWidgetItem(fieldItem, {tr("Max"), QString::number(field.maxValue())});
            new QTreeWidgetItem(fieldItem, {tr("Nodal Values"), QString::number(static_cast<qulonglong>(field.nodalValues().size()))});
            new QTreeWidgetItem(fieldItem, {tr("Source"), field.source()});
        }
    }
}
