#include <QtWidgets/QWidget>
// Widget panel dedicated to exploding of assemblies within a GuiDocument object
class WidgetExplodeAssembly : public QWidget {
    Q_OBJECT
public:
    WidgetExplodeAssembly(QWidget* parent = nullptr);
    ~WidgetExplodeAssembly();

private slots:
    void onSliderValueChanged(int value);
    void onSpinBoxValueChanged(int value);

private :
    void explode();

private:
    class Ui_WidgetExplodeAssembly* m_ui= nullptr;
    int m_explodeValue;
};
