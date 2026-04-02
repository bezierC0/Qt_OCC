#include "WidgetBoolean.h"
#include "ViewerWidget.h"
#include "ViewManager.h"
#include "OCCView.h"
#include "SelectedEntity.h"
#include "TopoShapeUtil.h"
#include "common/ShapeLabelManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QGroupBox>
#include <QCheckBox>
#include <QMessageBox>
#include <QDebug>
#include <QComboBox>
#include <QLabel>
#include <TopoDS.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TDataStd_Name.hxx>
#include <Quantity_Color.hxx>

WidgetBoolean::WidgetBoolean(ViewerWidget *parent) :
    QWidget(parent),
    m_mainLayout(nullptr),
    m_inputListWidget(nullptr),
    m_btnAdd(nullptr),
    m_btnRemove(nullptr),
    m_btnApply(nullptr),
    m_parentViewer(parent)
{
    setupUi();
    setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint);
    setWindowTitle(tr("Boolean Operation"));
    resize(300, 400);
}

WidgetBoolean::~WidgetBoolean()
{
}

void WidgetBoolean::show()
{
    QWidget::show();
}

void WidgetBoolean::setupUi()
{
    m_mainLayout = new QVBoxLayout(this);

    // Buttons
    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_btnAdd = new QPushButton(tr("Add"), this);
    m_btnRemove = new QPushButton(tr("Remove"), this);
    m_btnApply = new QPushButton(tr("Apply"), this);
    
    btnLayout->addWidget(m_btnAdd);
    btnLayout->addWidget(m_btnRemove);
    btnLayout->addWidget(m_btnApply);
    m_mainLayout->addLayout(btnLayout);

    // Operation Type
    QHBoxLayout* opTypeLayout = new QHBoxLayout();
    m_cmbOperationType = new QComboBox(this);
    m_cmbOperationType->addItem(tr("Union"));
    m_cmbOperationType->addItem(tr("Intersection"));
    m_cmbOperationType->addItem(tr("Difference"));
    opTypeLayout->addWidget(new QLabel(tr("Operation Type:")));
    opTypeLayout->addWidget(m_cmbOperationType);
    m_mainLayout->addLayout(opTypeLayout);

    // Input List
    QGroupBox* grpInput = new QGroupBox(tr("Target Shapes"), this);
    QVBoxLayout* grpInputLayout = new QVBoxLayout(grpInput);
    m_inputListWidget = new QListWidget(this);
    m_inputListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    grpInputLayout->addWidget(m_inputListWidget);
    m_mainLayout->addWidget(grpInput);

    // Options
    m_chkDeleteOriginal = new QCheckBox(tr("Delete original solids"), this);
    m_chkDeleteOriginal->setChecked(true);
    m_mainLayout->addWidget(m_chkDeleteOriginal);

    // Connects
    connect(m_btnAdd, &QPushButton::clicked, this, &WidgetBoolean::onAddClicked);
    connect(m_btnRemove, &QPushButton::clicked, this, &WidgetBoolean::onRemoveClicked);
    connect(m_btnApply, &QPushButton::clicked, this, &WidgetBoolean::onApplyClicked);
}

void WidgetBoolean::setOperationType(int type)
{
    if (m_cmbOperationType) {
        m_cmbOperationType->setCurrentIndex(type);
    }
}

void WidgetBoolean::onAddClicked()
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

void WidgetBoolean::onRemoveClicked()
{
    auto selectedItems = m_inputListWidget->selectedItems();
    if (selectedItems.isEmpty()) return;

    std::vector<int> rows;
    for (auto item : selectedItems) {
        rows.push_back(m_inputListWidget->row(item));
    }
    std::sort(rows.rbegin(), rows.rend());

    for (int row : rows) {
        if (row >= 0 && row < (int)m_inputObjects.size()) {
            m_inputObjects.erase(m_inputObjects.begin() + row);
        }
    }
    updateInputList();
}

void WidgetBoolean::updateInputList()
{
    m_inputListWidget->clear();
    for (const auto& entity : m_inputObjects) {
        QString name = tr("Unknown");
        if (entity) {
            if (!entity->m_label.IsNull()) {
                Handle(TDataStd_Name) attrName;
                if (entity->m_label.FindAttribute(TDataStd_Name::GetID(), attrName)) {
                     TCollection_ExtendedString extName = attrName->Get();
                     if (!extName.IsEmpty()) {
                        name = QString::fromUtf16(extName.ToExtString());
                    }
                }
            }
            if (name == tr("Unknown") && !entity->GetSelectedShape().IsNull()) {
                 const TopoDS_Shape& shape = entity->GetSelectedShape()->Shape();
                 name = QString::fromStdString(Util::TopoShape::GetShapeTypeString(shape));
            }
        }
        m_inputListWidget->addItem(name);
    }
}

void WidgetBoolean::onApplyClicked()
{
    if (m_inputObjects.size() < 2) {
        QMessageBox::warning(this, tr("Warning"), tr("Please add at least 2 shapes to fuse."));
        return;
    }

    bool allColorsMatch = true;
    Quantity_Color firstColor;
    bool hasFirstColor = false;

    for (const auto& entity : m_inputObjects) {
        if (!entity || entity->GetSelectedShape().IsNull()) continue;
        auto aisShape = entity->GetSelectedShape();
        Quantity_Color c;
        if (aisShape->HasColor()) {
            aisShape->Color(c);
            if (!hasFirstColor) {
                firstColor = c;
                hasFirstColor = true;
            } else {
                if (firstColor.Red() != c.Red() || 
                    firstColor.Green() != c.Green() || 
                    firstColor.Blue() != c.Blue()) {
                    allColorsMatch = false;
                }
            }
        } else {
            allColorsMatch = false;
        }
    }

    TopoDS_Shape resultShape = m_inputObjects[0]->GetSelectedShape()->Shape();
    for(size_t i = 1; i < m_inputObjects.size(); ++i) {
        TopoDS_Shape shapeB = m_inputObjects[i]->GetSelectedShape()->Shape();
        if (m_cmbOperationType->currentIndex() == 0) {
            BRepAlgoAPI_Fuse booleanFun(resultShape, shapeB);
            if (booleanFun.IsDone()) {
                resultShape = booleanFun.Shape();
            } else {
                QMessageBox::warning(this, tr("Boolean Operation Failed"), tr("Boolean Union operation failed at item %1").arg(i+1));
                return;
            }
        } else if (m_cmbOperationType->currentIndex() == 1) {
            BRepAlgoAPI_Common booleanFun(resultShape, shapeB);
            if (booleanFun.IsDone()) {
                resultShape = booleanFun.Shape();
            } else {
                QMessageBox::warning(this, tr("Boolean Operation Failed"), tr("Boolean Intersection operation failed at item %1").arg(i+1));
                return;
            }
        } else if (m_cmbOperationType->currentIndex() == 2) {
            BRepAlgoAPI_Cut booleanFun(resultShape, shapeB);
            if (booleanFun.IsDone()) {
                resultShape = booleanFun.Shape();
            } else {
                QMessageBox::warning(this, tr("Boolean Operation Failed"), tr("Boolean Difference operation failed at item %1").arg(i+1));
                return;
            }
        }
    }

    if (m_parentViewer) {
        if (m_chkDeleteOriginal->isChecked()) {
            for (const auto& entity : m_inputObjects) {
                if (!entity->GetSelectedShape().IsNull()) {
                    m_parentViewer->removeShape(entity->GetSelectedShape()->Shape());
                }
            }
        }
        
        double r = 1.0, g = 0.0, b = 1.0; // Default color
        if (hasFirstColor && allColorsMatch) {
            r = firstColor.Red();
            g = firstColor.Green();
            b = firstColor.Blue();
        }
        
        m_parentViewer->displayShape(resultShape, r, g, b);
    }

    auto view = ViewManager::getInstance().getActiveView();
    if (view) {
        view->clearSelectedObjects();
    }
    
    // Clear list
    m_inputObjects.clear();
    updateInputList();
}
