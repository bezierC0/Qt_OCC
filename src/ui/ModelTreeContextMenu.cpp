#include "ModelTreeContextMenu.h"
#include "ui_model_tree_context_menu.h"

#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QScreen>

ModelTreeContextMenu::ModelTreeContextMenu(QWidget* parent)
    : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint),
      ui(new Ui::ModelTreeContextMenu)
{
    ui->setupUi(this);

    // Enable transparency so QSS border-radius renders correctly
    setAttribute(Qt::WA_TranslucentBackground);

    connect(ui->btnPick,   &QPushButton::clicked, this, &ModelTreeContextMenu::onPickClicked);
    connect(ui->btnRemove, &QPushButton::clicked, this, &ModelTreeContextMenu::onRemoveClicked);

    // Close the menu when the user clicks outside it
    qApp->installEventFilter(this);
}

ModelTreeContextMenu::~ModelTreeContextMenu()
{
    qApp->removeEventFilter(this);
    delete ui;
}

void ModelTreeContextMenu::popup(const QPoint& globalPos, bool actionsEnabled)
{
    ui->btnPick->setEnabled(actionsEnabled);
    ui->btnRemove->setEnabled(actionsEnabled);

    // Clamp position so the menu stays within the available screen area
    QPoint pos = globalPos;
    if (QScreen* screen = QApplication::screenAt(globalPos)) {
        const QRect screenRect = screen->availableGeometry();
        if (pos.x() + width()  > screenRect.right())
            pos.setX(screenRect.right()  - width());
        if (pos.y() + height() > screenRect.bottom())
            pos.setY(screenRect.bottom() - height());
    }

    move(pos);
    show();
    raise();
}

bool ModelTreeContextMenu::eventFilter(QObject* watched, QEvent* event)
{
    if (isVisible() && event->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(event);
        // Close the menu when the click lands outside its geometry
        if (!geometry().contains(me->globalPos()))
            hide();
    }
    return QWidget::eventFilter(watched, event);
}

void ModelTreeContextMenu::onPickClicked()
{
    hide();
    emit pickRequested();
}

void ModelTreeContextMenu::onRemoveClicked()
{
    hide();
    emit removeRequested();
}
