#pragma once

#include <QColor>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QSpinBox;

namespace PointCloud
{
struct Data;
}

class WidgetPointCloud : public QWidget
{
    Q_OBJECT

public:
    explicit WidgetPointCloud(QWidget* parent = nullptr);

    void setPointCloud(const PointCloud::Data& data, quint64 displayedPointCount);
    void clearPointCloud();

signals:
    void visibilityChanged(bool visible);
    void pointSizeChanged(double size);
    void colorModeChanged(int mode);
    void singleColorChanged(const QColor& color);
    void maximumDisplayPointsChanged(qulonglong maximumPointCount);
    void displaySettingsReset();
    void fitRequested();
    void removeRequested();

private:
    void chooseSingleColor();
    void resetDisplaySettings();
    void updateColorButton();
    void setControlsEnabled(bool enabled);

private:
    QLabel* m_fileNameValue{};
    QLabel* m_fileSizeValue{};
    QLabel* m_sourceRecordCountValue{};
    QLabel* m_pointCountValue{};
    QLabel* m_displayedPointCountValue{};
    QLabel* m_colorValue{};
    QLabel* m_boundsValue{};
    QLabel* m_sizeValue{};
    QLabel* m_invalidRecordValue{};
    QCheckBox* m_visibleCheckBox{};
    QSpinBox* m_pointSizeSpinBox{};
    QComboBox* m_colorModeComboBox{};
    QPushButton* m_colorButton{};
    QComboBox* m_maximumPointsComboBox{};
    QPushButton* m_fitButton{};
    QPushButton* m_resetButton{};
    QPushButton* m_removeButton{};
    QColor m_singleColor{Qt::lightGray};
};
