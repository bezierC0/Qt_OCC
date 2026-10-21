#include "WidgetPointCloud.h"

#include "pointcloud/PointCloudData.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QVBoxLayout>

namespace
{

QString formatNumber(std::uint64_t value)
{
    return QLocale().toString(static_cast<qulonglong>(value));
}

QString formatCoordinate(double value)
{
    return QLocale().toString(value, 'g', 8);
}

} // namespace

WidgetPointCloud::WidgetPointCloud(QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* informationGroup = new QGroupBox(tr("Point Cloud Information"), this);
    auto* informationLayout = new QFormLayout(informationGroup);
    m_fileNameValue = new QLabel(QStringLiteral("-"), informationGroup);
    m_fileSizeValue = new QLabel(QStringLiteral("-"), informationGroup);
    m_sourceRecordCountValue = new QLabel(QStringLiteral("-"), informationGroup);
    m_pointCountValue = new QLabel(QStringLiteral("-"), informationGroup);
    m_displayedPointCountValue = new QLabel(QStringLiteral("-"), informationGroup);
    m_colorValue = new QLabel(QStringLiteral("-"), informationGroup);
    m_boundsValue = new QLabel(QStringLiteral("-"), informationGroup);
    m_sizeValue = new QLabel(QStringLiteral("-"), informationGroup);
    m_invalidRecordValue = new QLabel(QStringLiteral("-"), informationGroup);
    m_boundsValue->setWordWrap(true);
    m_sizeValue->setWordWrap(true);
    informationLayout->addRow(tr("File"), m_fileNameValue);
    informationLayout->addRow(tr("File Size"), m_fileSizeValue);
    informationLayout->addRow(tr("Source Records"), m_sourceRecordCountValue);
    informationLayout->addRow(tr("Valid Points"), m_pointCountValue);
    informationLayout->addRow(tr("Displayed Points"), m_displayedPointCountValue);
    informationLayout->addRow(tr("Point Colors"), m_colorValue);
    informationLayout->addRow(tr("Bounds"), m_boundsValue);
    informationLayout->addRow(tr("Dimensions"), m_sizeValue);
    informationLayout->addRow(tr("Invalid Records"), m_invalidRecordValue);
    mainLayout->addWidget(informationGroup);

    auto* displayGroup = new QGroupBox(tr("Point Cloud Display"), this);
    auto* displayLayout = new QFormLayout(displayGroup);
    m_visibleCheckBox = new QCheckBox(tr("Visible"), displayGroup);
    m_visibleCheckBox->setChecked(true);
    m_pointSizeSpinBox = new QSpinBox(displayGroup);
    m_pointSizeSpinBox->setRange(1, 10);
    m_pointSizeSpinBox->setValue(2);
    m_colorModeComboBox = new QComboBox(displayGroup);
    m_colorModeComboBox->addItem(tr("Original RGB"), static_cast<int>(PointCloud::ColorMode::Original));
    m_colorModeComboBox->addItem(tr("Single Color"), static_cast<int>(PointCloud::ColorMode::Single));
    m_colorModeComboBox->addItem(tr("Height (Z)"), static_cast<int>(PointCloud::ColorMode::Height));
    m_colorButton = new QPushButton(tr("Choose Color"), displayGroup);
    updateColorButton();
    m_maximumPointsComboBox = new QComboBox(displayGroup);
    m_maximumPointsComboBox->addItem(tr("500,000"), QVariant::fromValue<qulonglong>(500000));
    m_maximumPointsComboBox->addItem(tr("1,000,000"), QVariant::fromValue<qulonglong>(1000000));
    m_maximumPointsComboBox->addItem(tr("2,000,000"), QVariant::fromValue<qulonglong>(2000000));
    m_maximumPointsComboBox->addItem(tr("5,000,000"), QVariant::fromValue<qulonglong>(5000000));
    m_maximumPointsComboBox->addItem(tr("All Points"), QVariant::fromValue<qulonglong>(0));
    m_maximumPointsComboBox->setCurrentIndex(2);
    displayLayout->addRow(m_visibleCheckBox);
    displayLayout->addRow(tr("Point Size"), m_pointSizeSpinBox);
    displayLayout->addRow(tr("Color Mode"), m_colorModeComboBox);
    displayLayout->addRow(tr("Single Color"), m_colorButton);
    displayLayout->addRow(tr("Maximum Display"), m_maximumPointsComboBox);
    mainLayout->addWidget(displayGroup);

    auto* buttonLayout = new QHBoxLayout();
    m_fitButton = new QPushButton(tr("Fit Point Cloud"), this);
    m_resetButton = new QPushButton(tr("Reset Display Settings"), this);
    m_removeButton = new QPushButton(tr("Remove Point Cloud"), this);
    buttonLayout->addWidget(m_fitButton);
    buttonLayout->addWidget(m_resetButton);
    buttonLayout->addWidget(m_removeButton);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addStretch();

    connect(m_visibleCheckBox, &QCheckBox::toggled, this, &WidgetPointCloud::visibilityChanged);
    connect(m_pointSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this](int value) { emit pointSizeChanged(static_cast<double>(value)); });
    connect(m_colorModeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                const int mode = m_colorModeComboBox->itemData(index).toInt();
                m_colorButton->setEnabled(mode == static_cast<int>(PointCloud::ColorMode::Single));
                emit colorModeChanged(mode);
            });
    connect(m_colorButton, &QPushButton::clicked, this, &WidgetPointCloud::chooseSingleColor);
    connect(m_maximumPointsComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int index) {
                emit maximumDisplayPointsChanged(m_maximumPointsComboBox->itemData(index).toULongLong());
            });
    connect(m_fitButton, &QPushButton::clicked, this, &WidgetPointCloud::fitRequested);
    connect(m_resetButton, &QPushButton::clicked, this, &WidgetPointCloud::resetDisplaySettings);
    connect(m_removeButton, &QPushButton::clicked, this, &WidgetPointCloud::removeRequested);

    clearPointCloud();
}

void WidgetPointCloud::setPointCloud(const PointCloud::Data& data, quint64 displayedPointCount)
{
    m_fileNameValue->setText(QFileInfo(data.sourcePath).fileName());
    m_fileNameValue->setToolTip(data.sourcePath);
    m_fileSizeValue->setText(QLocale().formattedDataSize(static_cast<qint64>(data.fileSize)));
    m_sourceRecordCountValue->setText(formatNumber(data.sourceRecordCount));
    m_pointCountValue->setText(formatNumber(data.points.size()));
    m_displayedPointCountValue->setText(formatNumber(displayedPointCount));
    m_colorValue->setText(data.hasColors() ? tr("RGB") : tr("No"));
    m_invalidRecordValue->setText(formatNumber(data.invalidRecordCount));

    if (data.bounds.valid) {
        m_boundsValue->setText(
            tr("X: %1 .. %2\nY: %3 .. %4\nZ: %5 .. %6")
                .arg(formatCoordinate(data.bounds.xMin), formatCoordinate(data.bounds.xMax),
                     formatCoordinate(data.bounds.yMin), formatCoordinate(data.bounds.yMax),
                     formatCoordinate(data.bounds.zMin), formatCoordinate(data.bounds.zMax)));
        m_sizeValue->setText(
            tr("%1 x %2 x %3")
                .arg(formatCoordinate(data.bounds.xMax - data.bounds.xMin),
                     formatCoordinate(data.bounds.yMax - data.bounds.yMin),
                     formatCoordinate(data.bounds.zMax - data.bounds.zMin)));
    } else {
        m_boundsValue->setText(QStringLiteral("-"));
        m_sizeValue->setText(QStringLiteral("-"));
    }

    {
        const QSignalBlocker blocker(m_visibleCheckBox);
        m_visibleCheckBox->setChecked(true);
    }
    setControlsEnabled(true);
    m_colorButton->setEnabled(
        m_colorModeComboBox->currentData().toInt() == static_cast<int>(PointCloud::ColorMode::Single));
}

void WidgetPointCloud::clearPointCloud()
{
    m_fileNameValue->setText(QStringLiteral("-"));
    m_fileNameValue->setToolTip({});
    m_fileSizeValue->setText(QStringLiteral("-"));
    m_sourceRecordCountValue->setText(QStringLiteral("-"));
    m_pointCountValue->setText(QStringLiteral("-"));
    m_displayedPointCountValue->setText(QStringLiteral("-"));
    m_colorValue->setText(QStringLiteral("-"));
    m_boundsValue->setText(QStringLiteral("-"));
    m_sizeValue->setText(QStringLiteral("-"));
    m_invalidRecordValue->setText(QStringLiteral("-"));
    setControlsEnabled(false);
    m_maximumPointsComboBox->setEnabled(true);
}

void WidgetPointCloud::chooseSingleColor()
{
    const QColor color = QColorDialog::getColor(m_singleColor, this, tr("Point Cloud Color"));
    if (!color.isValid()) {
        return;
    }
    m_singleColor = color;
    updateColorButton();
    emit singleColorChanged(color);
}

void WidgetPointCloud::resetDisplaySettings()
{
    const QSignalBlocker visibleBlocker(m_visibleCheckBox);
    const QSignalBlocker sizeBlocker(m_pointSizeSpinBox);
    const QSignalBlocker modeBlocker(m_colorModeComboBox);
    const QSignalBlocker maximumBlocker(m_maximumPointsComboBox);

    m_visibleCheckBox->setChecked(true);
    m_pointSizeSpinBox->setValue(2);
    m_colorModeComboBox->setCurrentIndex(
        m_colorModeComboBox->findData(static_cast<int>(PointCloud::ColorMode::Original)));
    m_maximumPointsComboBox->setCurrentIndex(
        m_maximumPointsComboBox->findData(QVariant::fromValue<qulonglong>(2000000)));
    m_singleColor = QColor(Qt::lightGray);
    updateColorButton();
    m_colorButton->setEnabled(false);
    emit displaySettingsReset();
}

void WidgetPointCloud::updateColorButton()
{
    m_colorButton->setStyleSheet(
        QStringLiteral("QPushButton { background-color: %1; }").arg(m_singleColor.name()));
}

void WidgetPointCloud::setControlsEnabled(bool enabled)
{
    m_visibleCheckBox->setEnabled(enabled);
    m_pointSizeSpinBox->setEnabled(enabled);
    m_colorModeComboBox->setEnabled(enabled);
    m_colorButton->setEnabled(enabled);
    m_fitButton->setEnabled(enabled);
    m_resetButton->setEnabled(enabled);
    m_removeButton->setEnabled(enabled);
}
