#pragma once

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

private:
    void addStudyItem(QTreeWidgetItem* parent, const Cae::CaeStudy& study);

private:
    QTreeWidget* m_treeWidget{nullptr};
};
