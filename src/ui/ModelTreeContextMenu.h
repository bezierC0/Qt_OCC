#pragma once

#include <QWidget>
#include <QPoint>

namespace Ui {
class ModelTreeContextMenu;
}

/**
 * @brief Context menu View layer for the Model Tree.
 *
 * A pure UI class that contains no business logic.
 * External callers control visibility and enabled state via popup().
 * User actions are reported outward through signals.
 */
class ModelTreeContextMenu : public QWidget
{
    Q_OBJECT
public:
    explicit ModelTreeContextMenu(QWidget* parent = nullptr);
    ~ModelTreeContextMenu() override;

    /**
     * @brief Display the menu at the given screen position.
     * @param globalPos      Screen coordinates (typically from viewport()->mapToGlobal()).
     * @param actionsEnabled Whether the Pick / Remove buttons should be enabled.
     */
    void popup(const QPoint& globalPos, bool actionsEnabled);

signals:
    void pickRequested();
    void removeRequested();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onPickClicked();
    void onRemoveClicked();

private:
    Ui::ModelTreeContextMenu* ui;
};
