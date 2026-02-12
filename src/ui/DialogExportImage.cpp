#include "DialogExportImage.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QCheckBox>
#include <QLabel>
#include <QFileDialog>
#include <QClipboard>
#include <QApplication>
#include <QMessageBox>
#include <QImageWriter>
#include <QBuffer>

#include <Image_PixMap.hxx>
#include <Aspect_Window.hxx>
#include <Graphic3d_BufferType.hxx>

namespace {
    static QImage qtImageTemp(const Image_PixMap& occImg)
    {
        // occImg.Data() is the raw buffer.
        // We assume Format_RGBA8888 for Graphic3d_BT_RGBA
        const QImage img(occImg.Data(),
                         int(occImg.Width()),
                         int(occImg.Height()),
                         int(occImg.SizeRowBytes()),
                         QImage::Format_RGBA8888);
        
        // QImage holds a pointer to existing data, so we must make a deep copy if we want to use it outside
        // But here we return a QImage wrapping the data. The caller must ensure occImg lives as long as QImage is used,
        // OR make a copy immediately (like img.copy()).
        return img;
    }
}

DialogExportImage::DialogExportImage(const Handle(V3d_View)& view, QWidget* parent)
    : QDialog(parent), m_view(view)
{
    setWindowTitle(tr("Export Image"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    auto* mainLayout = new QVBoxLayout(this);
    auto* formLayout = new QFormLayout();

    // Defaults
    Standard_Integer w = 800, h = 600;
    if (!m_view.IsNull() && !m_view->Window().IsNull()) {
        const auto size = m_view->Window()->Dimensions();
        w = size.x();
        h = size.y();
    }

    m_spinWidth = new QSpinBox(this);
    m_spinWidth->setRange(1, 16384);
    m_spinWidth->setValue(w);

    m_spinHeight = new QSpinBox(this);
    m_spinHeight->setRange(1, 16384);
    m_spinHeight->setValue(h);

    m_checkKeepRatio = new QCheckBox(tr("Keep Aspect Ratio"), this);
    m_checkKeepRatio->setChecked(true);

    formLayout->addRow(tr("Width:"), m_spinWidth);
    formLayout->addRow(tr("Height:"), m_spinHeight);
    formLayout->addRow(tr("Ratio:"), m_checkKeepRatio);

    mainLayout->addLayout(formLayout);

    // Buttons
    auto* btnLayout = new QHBoxLayout();
    auto* btnSave = new QPushButton(tr("SaveFile"), this);
    auto* btnCopy = new QPushButton(tr("CopyClipboard"), this);
    auto* btnPreview = new QPushButton(tr("Preview"), this);
    auto* btnClose = new QPushButton(tr("Close"), this);

    btnLayout->addWidget(btnSave);
    btnLayout->addWidget(btnCopy);
    btnLayout->addWidget(btnPreview);
    btnLayout->addWidget(btnClose);

    mainLayout->addLayout(btnLayout);

    connect(btnSave, &QPushButton::clicked, this, &DialogExportImage::onSaveClicked);
    connect(btnCopy, &QPushButton::clicked, this, &DialogExportImage::onCopyClicked);
    connect(btnPreview, &QPushButton::clicked, this, &DialogExportImage::onPreviewClicked);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
}

DialogExportImage::~DialogExportImage()
{
}

void DialogExportImage::onSaveClicked()
{
    // Gather supported formats
    QHash<QString, QByteArray> mapFilterFormat;
    QStringList listFormat;
    for (const QByteArray& name : QImageWriter::supportedImageFormats()) {
        const QString strName = QString::fromLatin1(name);
        listFormat.append(tr("%1 files(*.%2)").arg(strName.toUpper(), strName.toLower()));
        mapFilterFormat.insert(listFormat.back(), name);
    }

    QString selectedFormat;
    const QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Select image file"),
        QString(),
        listFormat.join(QLatin1String(";;")),
        &selectedFormat);

    if (!fileName.isEmpty()) {
        auto itFound = mapFilterFormat.find(selectedFormat);
        const char* format = (itFound != mapFilterFormat.end()) ? itFound.value().constData() : nullptr;

        Image_PixMap occPix;
        if (createImageView(occPix)) {
            const QImage img = qtImageTemp(occPix);
            if (!img.save(fileName, format)) {
                QMessageBox::critical(this, tr("Error"), tr("Failed to save image '%1'").arg(fileName));
            }
        } else {
             QMessageBox::warning(this, tr("Error"), tr("Failed to dump view to pixmap"));
        }
    }
}

void DialogExportImage::onCopyClicked()
{
    Image_PixMap occPix;
    if (createImageView(occPix)) {
        // Must copy because QClipboard expects independent data, and occPix will be destroyed
        const QImage img = qtImageTemp(occPix).copy();
        QApplication::clipboard()->setImage(img);
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Failed to dump view to pixmap"));
    }
}

void DialogExportImage::onPreviewClicked()
{
    Image_PixMap occPix;
    if (createImageView(occPix)) {
        const QImage img = qtImageTemp(occPix).copy(); // copy to be safe regarding occPix lifetime
        auto label = new QLabel(nullptr, Qt::Window);
        label->setAttribute(Qt::WA_DeleteOnClose);
        label->setPixmap(QPixmap::fromImage(img));
        label->setWindowTitle(tr("Preview %1x%2").arg(img.width()).arg(img.height()));
        label->show();
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Failed to dump view to pixmap"));
    }
}

bool DialogExportImage::createImageView(Image_PixMap& img) const
{
    if (m_view.IsNull()) return false;

    img.SetTopDown(true); // QImage is TopDown
    return m_view->ToPixMap(
        img,
        m_spinWidth->value(),
        m_spinHeight->value(),
        Graphic3d_BT_RGBA,
        m_checkKeepRatio->isChecked()
    );
}
