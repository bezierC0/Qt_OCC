#pragma once

#include "cae/CaeTypes.h"

#include <QPoint>
#include <QUuid>
#include <QWidget>

class QTreeWidget;
class QTreeWidgetItem;

namespace Cae {
class CaeProject;
class CaeStudy;
}

class CaeTreeWidget : public QWidget {
    Q_OBJECT
public:
    explicit CaeTreeWidget(QWidget* parent = nullptr);

    void setProject(const Cae::CaeProject& project);
    void clearProject();

signals:
    void meshActivated(const QUuid& studyId);
    void resultFieldActivated(const QUuid& studyId, Cae::ResultFieldType fieldType);
    void removeBoundaryConditionRequested(const QUuid& studyId, const QString& name);

private:
    void addStudyItem(QTreeWidgetItem* parent, const Cae::CaeStudy& study, bool active);
    void activateItem(QTreeWidgetItem* item);
    void showContextMenu(const QPoint& position);

private:
    QTreeWidget* m_treeWidget{nullptr};
};
