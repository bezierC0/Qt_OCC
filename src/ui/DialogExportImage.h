#pragma once

#include <QDialog>
#include <V3d_View.hxx>


class QSpinBox;
class QCheckBox;
class Image_PixMap;
/*
mayo\src\app\dialog_save_image_view.cpp
*/
class DialogExportImage : public QDialog
{
    Q_OBJECT

public:
    explicit DialogExportImage(const Handle(V3d_View)& view, QWidget* parent = nullptr);
    ~DialogExportImage() override;

private slots:
    void onSaveClicked();
    void onCopyClicked();
    void onPreviewClicked();

private:
    bool createImageView(Image_PixMap& img) const;

private:
    Handle(V3d_View) m_view;
    QSpinBox* m_spinWidth;
    QSpinBox* m_spinHeight;
    QCheckBox* m_checkKeepRatio;
};
