
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
#include <QDebug>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <AIS_Shape.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDataStd_Name.hxx> // Added for direct name access


WidgetInterference::WidgetInterference(QWidget *parent) :
    QWidget(parent),
    m_mainLayout(nullptr),
    m_inputListWidget(nullptr),
    m_resultTreeWidget(nullptr),
    m_btnAdd(nullptr),
    m_btnRemove(nullptr),
    m_btnCheck(nullptr)
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
    m_btnCheck = new QPushButton(tr("Check"), this);
    
    btnLayout->addWidget(m_btnAdd);
    btnLayout->addWidget(m_btnRemove);
    btnLayout->addWidget(m_btnCheck);
    m_mainLayout->addLayout(btnLayout);

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
    m_resultTreeWidget = new QTreeWidget(this);
    m_resultTreeWidget->setHeaderLabels({tr("Collision Pair"), tr("Details")});
    m_resultTreeWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    grpResultLayout->addWidget(m_resultTreeWidget);
    m_mainLayout->addWidget(grpResult);

    // Connects
    connect(m_btnAdd, &QPushButton::clicked, this, &WidgetInterference::onAddClicked);
    connect(m_btnRemove, &QPushButton::clicked, this, &WidgetInterference::onRemoveClicked);
    connect(m_btnCheck, &QPushButton::clicked, this, &WidgetInterference::onCheckClicked);
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
    for(const auto& entity : m_inputObjects) {
        objectsToCheck.push_back(entity->GetSelectedShape());
    }
    auto results = view->checkInterference(objectsToCheck);
    
    m_resultTreeWidget->clear();

    for (const auto& res : results) {
        QString nameA = tr("Unknown");
        QString nameB = tr("Unknown");
        
        auto getName = [](const Handle(AIS_InteractiveObject)& obj) -> QString {
            if (obj.IsNull()) return tr("Unknown");
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
                QTreeWidgetItem* item = new QTreeWidgetItem(m_resultTreeWidget);
                item->setText(0, QString("%1 - %2").arg(nameA, nameB));
                item->setText(1, QString("Intersection %1").arg(idx++));
            }
        } else {
            // Single intersection (or non-solid intersection)
            QTreeWidgetItem* item = new QTreeWidgetItem(m_resultTreeWidget);
            item->setText(0, QString("%1 - %2").arg(nameA, nameB));
             if (solidCount == 1) {
                item->setText(1, tr("1 Solid Intersection"));
             } else {
                 // Check faces
                 int faceCount = 0;
                 for (TopExp_Explorer ef(intersection, TopAbs_FACE); ef.More(); ef.Next()) faceCount++;
                 if (faceCount > 0)
                     item->setText(1, QString("%1 Face Intersections").arg(faceCount));
                 else 
                     item->setText(1, tr("Intersection"));
             }
        }
    }
}
