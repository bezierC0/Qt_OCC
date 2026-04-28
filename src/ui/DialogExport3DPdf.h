#pragma once

#include <QDialog>
#include <TopoDS_Compound.hxx>
#include <Quantity_Color.hxx>

namespace Ui {
class DialogExport3DPdf;
}

class DialogExport3DPdf : public QDialog
{
    Q_OBJECT

public:
    explicit DialogExport3DPdf(const TopoDS_Compound& compound, const Quantity_Color& bgColor, QWidget* parent = nullptr);
    ~DialogExport3DPdf() override;

private slots:
    void onExportClicked();

private:
    Ui::DialogExport3DPdf *ui;
    TopoDS_Compound m_compound;
    Quantity_Color m_bgColor;
};
