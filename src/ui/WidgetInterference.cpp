
#include "WidgetInterference.h"
#include "ViewManager.h"
#include "OCCView.h"
#include "SelectedEntity.h"
#include "TopoShapeUtil.h"
#include "common/ShapeLabelManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QCoreApplication>
#include <QEventLoop>
#include <QProgressBar>
#include <QCheckBox>
#include <QFormLayout>
#include <QDebug>
#include <algorithm>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <AIS_Shape.hxx>
#include <XCAFPrs_AISObject.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDataStd_Name.hxx> // Added for direct name access
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>


WidgetInterference::WidgetInterference(QWidget *parent) :
    QWidget(parent),
    m_mainLayout(nullptr),
    m_inputListWidget(nullptr),
    m_resultTreeWidget(nullptr),
    m_btnAdd(nullptr),
    m_btnRemove(nullptr),
    m_btnCheck(nullptr),
    m_btnAddAll(nullptr),
    m_progressBar(nullptr),
    m_resultOnTop(nullptr),
    m_objectAStyle(nullptr),
    m_objectBStyle(nullptr),
    m_showOnlyPair(nullptr)
{
    setupUi();
    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint);
    setWindowTitle(tr("Interference Check"));
    resize(400, 500);
}

WidgetInterference::~WidgetInterference()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (view) {
        view->clearInterference();
    }
}

void WidgetInterference::show()
{
    QWidget::show();
    // Optional: Auto-add selected when showing?
    // onAddClicked(); 
}

void WidgetInterference::setupUi()
{
    m_mainLayout = new QVBoxLayout(this);

    // Buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_btnAdd = new QPushButton(tr("Add"), this);
    m_btnRemove = new QPushButton(tr("Remove"), this);
    m_btnCheck = new QPushButton(tr("Start Check"), this);
    m_btnAddAll = new QPushButton(tr("Add All"), this);
    m_mainLayout->addWidget(m_btnCheck);
    
    btnLayout->addWidget(m_btnAdd);
    btnLayout->addWidget(m_btnAddAll);
    btnLayout->addWidget(m_btnRemove);
    m_mainLayout->addLayout(btnLayout);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);
    m_mainLayout->addWidget(m_progressBar);

    // Input List
    QGroupBox* grpInput = new QGroupBox(tr("Selected Parts"), this);
    QVBoxLayout* grpInputLayout = new QVBoxLayout(grpInput);
    m_inputListWidget = new QListWidget(this);
    m_inputListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    grpInputLayout->addWidget(m_inputListWidget);
    m_mainLayout->addWidget(grpInput);

    // Result List
    QGroupBox* grpResult = new QGroupBox(tr("Interference Results"), this);
    QVBoxLayout* grpResultLayout = new QVBoxLayout(grpResult);

    QGroupBox* displayOptions = new QGroupBox(tr("Result Display"), grpResult);
    QFormLayout* displayOptionsLayout = new QFormLayout(displayOptions);
    m_resultOnTop = new QCheckBox(tr("Display result on top"), displayOptions);
    m_resultOnTop->setChecked(true);
    displayOptionsLayout->addRow(m_resultOnTop);

    m_showOnlyPair = new QCheckBox(tr("Only display collision pair"), displayOptions);
    displayOptionsLayout->addRow(m_showOnlyPair);

    m_objectAStyle = new QPushButton(tr("Solid"), displayOptions);
    m_objectBStyle = new QPushButton(tr("Solid"), displayOptions);
    displayOptionsLayout->addRow(tr("Object A:"), m_objectAStyle);
    displayOptionsLayout->addRow(tr("Object B:"), m_objectBStyle);
    grpResultLayout->addWidget(displayOptions);

    m_resultTreeWidget = new QTreeWidget(this);
    m_resultTreeWidget->setHeaderLabels(
        {tr("Object A"), tr("Object B"), tr("Details"), tr("Bounding Box")});
    m_resultTreeWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    grpResultLayout->addWidget(m_resultTreeWidget);
    m_mainLayout->addWidget(grpResult);

    // Connects
    connect(m_btnAdd, &QPushButton::clicked, this, &WidgetInterference::onAddClicked);
    connect(m_btnRemove, &QPushButton::clicked, this, &WidgetInterference::onRemoveClicked);
    connect(m_btnCheck, &QPushButton::clicked, this, &WidgetInterference::onCheckClicked);
    connect(m_btnAddAll, &QPushButton::clicked, this, &WidgetInterference::onAddAllClicked);
    connect(m_resultTreeWidget, &QTreeWidget::itemClicked,
            this, &WidgetInterference::onResultClicked);
    connect(m_resultOnTop, &QCheckBox::toggled,
            this, [this]() { applyResultDisplayOptions(); });
    connect(m_objectAStyle, &QPushButton::clicked, this, [this]() {
        cycleObjectStyle(m_objectAStyle, m_objectAStyleIndex);
    });
    connect(m_objectBStyle, &QPushButton::clicked, this, [this]() {
        cycleObjectStyle(m_objectBStyle, m_objectBStyleIndex);
    });
    connect(m_showOnlyPair, &QCheckBox::toggled,
            this, [this]() { applyResultDisplayOptions(); });
}

void WidgetInterference::onAddClicked()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;

    const auto& selected = view->getSelectedObjects();
    if (selected.empty()) return;

    bool added = false;
    for (const auto& entity : selected) {
        if (!entity || entity->GetSelectedShape().IsNull()) continue;

        // Check duplication
        bool exists = false;
        for (const auto& existing : m_inputObjects) {
            // Compare by Selected Entity content (Shape)
             if (existing->GetSelectedShape() == entity->GetSelectedShape()) {
                exists = true;
                break;
            }
        }

        if (!exists) {
            m_inputObjects.push_back(entity);
            added = true;
        }
    }

    if (added) {
        updateInputList();
    }
}

void WidgetInterference::onRemoveClicked()
{
    auto selectedItems = m_inputListWidget->selectedItems();
    if (selectedItems.isEmpty()) return;

    // Map rows to remove
    std::vector<int> rows;
    for (auto item : selectedItems) {
        rows.push_back(m_inputListWidget->row(item));
    }
    // Sort descending to remove safely
    std::sort(rows.rbegin(), rows.rend());

    for (int row : rows) {
        if (row >= 0 && row < m_inputObjects.size()) {
            m_inputObjects.erase(m_inputObjects.begin() + row);
        }
    }
    updateInputList();
}

void WidgetInterference::updateInputList()
{
    m_inputListWidget->clear();
    for (const auto& entity : m_inputObjects) {
        QString name = tr("Unknown");
        if (entity) {
            // Priority 1: Use Label Name from SelectedEntity
            if (!entity->m_label.IsNull()) {
                Handle(TDataStd_Name) attrName;
                if (entity->m_label.FindAttribute(TDataStd_Name::GetID(), attrName)) {
                     TCollection_ExtendedString extName = attrName->Get();
                     if (!extName.IsEmpty()) {
                        name = QString::fromUtf16(extName.ToExtString());
                    }
                }
            }
            
            // Priority 2: Use Shape Type
            if (name == tr("Unknown") && !entity->GetSelectedShape().IsNull()) {
                 const TopoDS_Shape& shape = entity->GetSelectedShape()->Shape();
                 name = QString::fromStdString(Util::TopoShape::GetShapeTypeString(shape));
            }
        }
        m_inputListWidget->addItem(name);
    }
}

void WidgetInterference::onAddAllClicked()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;

    bool added = false;
    for (const Handle(AIS_InteractiveObject)& object : view->getShapeObjects()) {
        if (object.IsNull()) continue;

        const Handle(AIS_Shape) shape = Handle(AIS_Shape)::DownCast(object);
        if (shape.IsNull() || shape->Shape().IsNull()) continue;

        const bool exists = std::any_of(
            m_inputObjects.cbegin(), m_inputObjects.cend(),
            [&object](const std::shared_ptr<View::SelectedEntity>& entity) {
                return entity && entity->GetParentInteractiveObject() == object;
            });
        if (exists) continue;

        TDF_Label label;
        const Handle(XCAFPrs_AISObject) xcafObject =
            Handle(XCAFPrs_AISObject)::DownCast(object);
        if (!xcafObject.IsNull()) label = xcafObject->GetLabel();

        m_inputObjects.push_back(
            std::make_shared<View::SelectedEntity>(object, shape, label));
        added = true;
    }

    if (added) updateInputList();
}

void WidgetInterference::onCheckClicked()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;
    
    if (m_inputObjects.size() < 2) {
        QMessageBox::warning(this, tr("Warning"), tr("Please add at least 2 parts to check."));
        return;
    }

    // Call OCCView check with extracted shapes
    std::vector<Handle(AIS_InteractiveObject)> objectsToCheck;
    for (const auto& entity : m_inputObjects) {
        if (!entity) continue;
        const Handle(AIS_InteractiveObject) parent = entity->GetParentInteractiveObject();
        objectsToCheck.push_back(parent.IsNull() ? entity->GetSelectedShape() : parent);
    }
    const std::size_t totalPairs = objectsToCheck.size() < 2
        ? 0
        : objectsToCheck.size() * (objectsToCheck.size() - 1) / 2;
    m_progressBar->setRange(0, static_cast<int>(totalPairs));
    m_progressBar->setValue(0);
    m_progressBar->setFormat(tr("Checking: %v / %m"));
    m_progressBar->setVisible(true);

    for (QPushButton* button : {m_btnAdd, m_btnAddAll, m_btnRemove, m_btnCheck}) {
        button->setEnabled(false);
    }
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

    auto results = view->checkInterference(
        objectsToCheck,
        [this](std::size_t completed, std::size_t total) {
            m_progressBar->setMaximum(static_cast<int>(total));
            m_progressBar->setValue(static_cast<int>(completed));
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        });

    m_progressBar->setFormat(tr("Completed: %v / %m"));
    for (QPushButton* button : {m_btnAdd, m_btnAddAll, m_btnRemove, m_btnCheck}) {
        button->setEnabled(true);
    }
    
    m_resultTreeWidget->clear();
    m_resultContexts.clear();

    auto boundingBoxText = [](const TopoDS_Shape& shape) {
        Bnd_Box box;
        BRepBndLib::Add(shape, box);
        if (box.IsVoid()) return QStringLiteral("-");

        Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
        box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
        return QStringLiteral("%1 x %2 x %3")
            .arg(xmax - xmin, 0, 'f', 2)
            .arg(ymax - ymin, 0, 'f', 2)
            .arg(zmax - zmin, 0, 'f', 2);
    };

    for (const auto& res : results) {
        QString nameA = tr("Unknown");
        QString nameB = tr("Unknown");
        
        auto getName = [](const Handle(AIS_InteractiveObject)& obj) -> QString {
            if (obj.IsNull()) return tr("Unknown");
            const Handle(XCAFPrs_AISObject) xcafObject =
                Handle(XCAFPrs_AISObject)::DownCast(obj);
            if (!xcafObject.IsNull()) {
                Handle(TDataStd_Name) attrName;
                if (xcafObject->GetLabel().FindAttribute(TDataStd_Name::GetID(), attrName)
                    && !attrName->Get().IsEmpty()) {
                    return QString::fromUtf16(attrName->Get().ToExtString());
                }
            }
            Handle(AIS_Shape) aisShape = Handle(AIS_Shape)::DownCast(obj);
            if (!aisShape.IsNull()) {
                const TopoDS_Shape& shape = aisShape->Shape();
                TDF_Label label = ShapeLabelManager::GetInstance().GetLabel(shape);
                if (!label.IsNull()) {
                    Handle(TDataStd_Name) attrName;
                    if (label.FindAttribute(TDataStd_Name::GetID(), attrName)) {
                        TCollection_ExtendedString extName = attrName->Get();
                        if (!extName.IsEmpty()) {
                            return QString::fromUtf16(extName.ToExtString());
                        }
                    }
                }
            }
            // Fallback to AIS Name or Type
             TCollection_ExtendedString n = Util::Ais::GetNameFromAISObject(obj);
             if (!n.IsEmpty()) return QString::fromUtf16(n.ToExtString());
             return tr("Unknown");
        };

        nameA = getName(res.objA);
        nameB = getName(res.objB);

        // Analyze Intersection Shape
        TopoDS_Shape intersection = res.intersection;
        if (intersection.IsNull()) continue;

        // Count disjoint solids
        int solidCount = 0;
        TopExp_Explorer exp(intersection, TopAbs_SOLID);
        for (; exp.More(); exp.Next()) {
            solidCount++;
        }

        if (solidCount > 1) {
            // Multiple intersections
            int idx = 1;
            for (TopExp_Explorer e2(intersection, TopAbs_SOLID); e2.More(); e2.Next()) {
                const int resultIndex = static_cast<int>(m_resultContexts.size());
                m_resultContexts.push_back({e2.Current(), res.objA, res.objB});
                QTreeWidgetItem* item = new QTreeWidgetItem(m_resultTreeWidget);
                item->setText(0, nameA);
                item->setText(1, nameB);
                item->setText(2, QString("Intersection %1").arg(idx++));
                item->setText(3, boundingBoxText(e2.Current()));
                item->setData(0, Qt::UserRole, resultIndex);
            }
        } else {
            // Single intersection (or non-solid intersection)
            const int resultIndex = static_cast<int>(m_resultContexts.size());
            m_resultContexts.push_back({intersection, res.objA, res.objB});
            QTreeWidgetItem* item = new QTreeWidgetItem(m_resultTreeWidget);
            item->setText(0, nameA);
            item->setText(1, nameB);
            item->setText(3, boundingBoxText(intersection));
            item->setData(0, Qt::UserRole, resultIndex);
             if (solidCount == 1) {
                item->setText(2, tr("1 Solid Intersection"));
             } else {
                 // Check faces
                 int faceCount = 0;
                 for (TopExp_Explorer ef(intersection, TopAbs_FACE); ef.More(); ef.Next()) faceCount++;
                 if (faceCount > 0)
                     item->setText(2, QString("%1 Face Intersections").arg(faceCount));
                 else 
                     item->setText(2, tr("Intersection"));
             }
        }
    }

    if (m_resultTreeWidget->topLevelItemCount() > 0) {
        m_resultTreeWidget->setCurrentItem(m_resultTreeWidget->topLevelItem(0));
        applyResultDisplayOptions();
    }
}

void WidgetInterference::onResultClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);
    if (!item) return;

    const QVariant resultData = item->data(0, Qt::UserRole);
    if (!resultData.isValid()) return;
    const int resultIndex = resultData.toInt();
    if (resultIndex < 0 || resultIndex >= static_cast<int>(m_resultContexts.size())) return;

    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;
    applyResultDisplayOptions();
    view->fitShape(m_resultContexts.at(resultIndex).shape);
}

void WidgetInterference::applyResultDisplayOptions()
{
    auto view = ViewManager::getInstance().getActiveView();
    if (!view) return;

    view->setInterferenceResultOnTop(m_resultOnTop->isChecked());
    QTreeWidgetItem* item = m_resultTreeWidget->currentItem();
    if (!item) return;

    const QVariant resultData = item->data(0, Qt::UserRole);
    if (!resultData.isValid()) return;
    const int resultIndex = resultData.toInt();
    if (resultIndex < 0 || resultIndex >= static_cast<int>(m_resultContexts.size())) return;

    auto styleFromIndex = [](int index) {
        if (index == 1) return View::InterferenceObjectStyle::Transparent;
        if (index == 2) return View::InterferenceObjectStyle::Hidden;
        return View::InterferenceObjectStyle::Solid;
    };
    const ResultContext& result = m_resultContexts.at(resultIndex);
    view->setInterferenceObjectDisplay(
        result.objectA, result.objectB,
        styleFromIndex(m_objectAStyleIndex),
        styleFromIndex(m_objectBStyleIndex),
        m_showOnlyPair->isChecked());
}

void WidgetInterference::cycleObjectStyle(QPushButton* button, int& styleIndex)
{
    styleIndex = (styleIndex + 1) % 3;
    if (styleIndex == 1) {
        button->setText(tr("Transparent (30%)"));
    } else if (styleIndex == 2) {
        button->setText(tr("Hidden"));
    } else {
        button->setText(tr("Solid"));
    }
    applyResultDisplayOptions();
}
