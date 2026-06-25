#include "DialogExport3DPdf.h"
#include "ui_DialogExport3DPdf.h"
#include "../core_api/ExportApi.h"

#include <QFileDialog>
#include <QTemporaryDir>
#include <QProcess>
#include <QMessageBox>
#include <QTextStream>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>

DialogExport3DPdf::DialogExport3DPdf(const TopoDS_Compound& compound, const Quantity_Color& bgColor, QWidget* parent)
    : QDialog(parent), ui(new Ui::DialogExport3DPdf), m_compound(compound), m_bgColor(bgColor)
{
    ui->setupUi(this);

    // Method 2 is only available when compiled with USE_LIBPRC
#ifdef USE_LIBPRC
    ui->m_radioPRC->setEnabled(true);
    ui->m_radioPRC->setText(tr("Method 2: OCCT -> OSG -> libPRC -> 3D PDF"));
#else
    ui->m_radioPRC->setEnabled(false);
    ui->m_radioPRC->setToolTip(tr("Rebuild with EXPORT_3DPDF_USE_LIBPRC=ON to enable this method."));
#endif

    connect(ui->m_btnExport, &QPushButton::clicked, this, &DialogExport3DPdf::onExportClicked);
    connect(ui->m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

DialogExport3DPdf::~DialogExport3DPdf()
{
    delete ui;
}

void DialogExport3DPdf::onExportClicked()
{
#ifdef USE_LIBPRC
    if (ui->m_radioPRC->isChecked()) {
        exportViaPRC();
        return;
    }
#else
    if (ui->m_radioPRC->isChecked()) {
        QMessageBox::information(this, tr("Info"),
            tr("Method 2 requires recompiling with EXPORT_3DPDF_USE_LIBPRC=ON."));
        return;
    }
#endif

    exportViaU3D();
}

void DialogExport3DPdf::exportViaU3D()
{
    ui->m_btnExport->setEnabled(false);
    ui->m_progressBar->setValue(10);
    ui->m_lblStatus->setText(tr("Selecting file..."));

    // 1. Ask the user where to save the PDF file
    QString pdfPath = QFileDialog::getSaveFileName(
        this, tr("Save as 3D PDF File"), QString(), tr("PDF Files (*.pdf)"));
    if (pdfPath.isEmpty()) {
        ui->m_btnExport->setEnabled(true);
        return;
    }

    // 2. Create a temporary directory for the intermediate files (STL, U3D, TEX)
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        QMessageBox::critical(this, tr("Export 3D PDF"), tr("Cannot create temporary directory."));
        reject();
        return;
    }

    const QString tempStlName = QStringLiteral("model.stl");
    const QString tempU3dName = QStringLiteral("model.u3d");
    const QString tempTexName = QStringLiteral("model.tex");
    
    const QString tempStlPath = tempDir.filePath(tempStlName);
    const QString tempU3dPath = tempDir.filePath(tempU3dName);
    const QString tempTexPath = tempDir.filePath(tempTexName);

    // 3. Export to STL using CoreApi
    ui->m_progressBar->setValue(30);
    ui->m_lblStatus->setText(tr("Exporting to STL..."));
    if (!CoreApi::ExportApi::ExportToStl(m_compound, tempStlPath.toStdString())) {
        QMessageBox::critical(this, tr("Export 3D PDF"), tr("Failed to export shape to STL."));
        reject();
        return;
    }

    // 4. Convert STL to U3D using PyMeshLab
    ui->m_progressBar->setValue(60);
    ui->m_lblStatus->setText(tr("Converting STL to U3D via PyMeshLab..."));

    QProcess meshlabProcess;
    QString pythonCmd = QStringLiteral("python");
    QStringList pythonArgs;
    pythonArgs << "-c" 
               << QString("import pymeshlab\nms = pymeshlab.MeshSet()\nms.load_new_mesh(r'%1')\nms.save_current_mesh(r'%2')")
                  .arg(QDir::toNativeSeparators(tempStlPath))
                  .arg(QDir::toNativeSeparators(tempU3dPath));

    meshlabProcess.start(pythonCmd, pythonArgs);
    if (!meshlabProcess.waitForFinished(60000)) { // 60 seconds timeout
        meshlabProcess.kill();
        QMessageBox::critical(this, tr("Export 3D PDF"), 
            tr("PyMeshLab conversion timed out or could not be started.\n"
               "Please ensure Python and PyMeshLab are installed and available in PATH."));
        reject();
        return;
    }
    if (meshlabProcess.exitCode() != 0 || !QFileInfo::exists(tempU3dPath)) {
        QString stderrOut = QString::fromLocal8Bit(meshlabProcess.readAllStandardError());
        QString extraTip = stderrOut.contains("No module named 'pymeshlab'") 
                            ? tr("\n\nTip: You need to install pymeshlab. Run 'pip install pymeshlab' in your command prompt.")
                            : "";
        QMessageBox::critical(this, tr("Export 3D PDF"), 
            tr("PyMeshLab conversion failed. Error code: %1\n\nStderr: %2%3")
            .arg(meshlabProcess.exitCode())
            .arg(stderrOut)
            .arg(extraTip));
        reject();
        return;
    }

    // 5. Generate LaTeX file with media9 package
    ui->m_progressBar->setValue(80);
    ui->m_lblStatus->setText(tr("Compiling LaTeX to PDF..."));

    // Calculate bounding box for 3Dcoo and 3Droo to enable proper pan/zoom in Acrobat
    Bnd_Box bbox;
    BRepBndLib::Add(m_compound, bbox);
    Standard_Real xmin = 0, ymin = 0, zmin = 0, xmax = 0, ymax = 0, zmax = 0;
    if (!bbox.IsVoid()) {
        bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    }
    Standard_Real cx = (xmin + xmax) / 2.0;
    Standard_Real cy = (ymin + ymax) / 2.0;
    Standard_Real cz = (zmin + zmax) / 2.0;
    Standard_Real dx = xmax - xmin;
    Standard_Real dy = ymax - ymin;
    Standard_Real dz = zmax - zmin;
    Standard_Real radius = std::sqrt(dx*dx + dy*dy + dz*dz);
    if (radius < 1e-3) radius = 100.0; // fallback

    QFile texFile(tempTexPath);
    if (!texFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Export 3D PDF"), tr("Cannot create temporary LaTeX file."));
        reject();
        return;
    }
    QTextStream out(&texFile);
    out << "\\documentclass{article}\n";
    out << "\\usepackage[utf8]{inputenc}\n";
    out << "\\usepackage{media9}\n";
    out << "\\usepackage{graphicx}\n";
    out << "\\begin{document}\n";
    out << "\\begin{center}\n";
    // We use a dummy text instead of preview.png to simplify dependencies
    out << "\\includemedia[\n";
    out << "  width=0.9\\linewidth,\n";
    out << "  height=0.9\\linewidth,\n";
    out << "  activate=pageopen,\n";
    out << "  3Dmenu,\n";
    out << "  3Dtoolbar,\n";
    out << "  3Dbg=" << m_bgColor.Red() << " " << m_bgColor.Green() << " " << m_bgColor.Blue() << ",\n";
    out << "  3Dcoo=" << cx << " " << cy << " " << cz << ",\n";
    out << "  3Droo=" << radius << "\n";
    out << "]{\\fbox{Click to activate 3D model}}{" << tempU3dName << "}\n";
    out << "\\end{center}\n";
    out << "\\end{document}\n";
    texFile.close();

    // 6. Compile LaTeX to PDF using pdflatex
    const QString pdflatexExe = QStringLiteral("C:/Program Files/MiKTeX/miktex/bin/x64/miktex-pdflatex");
    QProcess pdflatexProcess;
    pdflatexProcess.setWorkingDirectory(tempDir.path());
    // Run pdflatex in non-interactive mode
    pdflatexProcess.start(pdflatexExe, QStringList() << "-interaction=nonstopmode" << tempTexName);
    if (!pdflatexProcess.waitForFinished(60000)) { // 60 seconds timeout
        pdflatexProcess.kill();
        QMessageBox::critical(this, tr("Export 3D PDF"), 
            tr("pdflatex compilation timed out or could not be started.\n"
               "Please ensure MiKTeX/TeX Live is installed and 'pdflatex' is in your system PATH."));
        reject();
        return;
    }
    
    const QString tempPdfPath = tempDir.filePath(QStringLiteral("model.pdf"));
    if (!QFileInfo::exists(tempPdfPath)) {
        QMessageBox::critical(this, tr("Export 3D PDF"), 
            tr("pdflatex compilation failed.\n\nStderr: %1\nStdout: %2")
            .arg(QString::fromLocal8Bit(pdflatexProcess.readAllStandardError()))
            .arg(QString::fromLocal8Bit(pdflatexProcess.readAllStandardOutput())));
        reject();
        return;
    }

    // 7. Move PDF to the target path
    ui->m_progressBar->setValue(95);
    ui->m_lblStatus->setText(tr("Saving final PDF..."));

    if (QFileInfo::exists(pdfPath)) {
        QFile::remove(pdfPath);
    }
    if (!QFile::rename(tempPdfPath, pdfPath)) {
        QFile::copy(tempPdfPath, pdfPath);
        QFile::remove(tempPdfPath);
    }

    ui->m_progressBar->setValue(100);
    ui->m_lblStatus->setText(tr("Done."));
    ui->m_btnExport->setEnabled(true);
    QMessageBox::information(this, tr("Export Successful"), tr("📄 3D PDF file saved to:\n%1").arg(pdfPath));
    accept();
}

// ---------------------------------------------------------------------------
// Method 2: OSG + libPRC + libHaru
// ---------------------------------------------------------------------------
#ifdef USE_LIBPRC
void DialogExport3DPdf::exportViaPRC()
{
    ui->m_btnExport->setEnabled(false);
    ui->m_progressBar->setValue(10);
    ui->m_lblStatus->setText(tr("Selecting save path..."));

    QString pdfPath = QFileDialog::getSaveFileName(
        this, tr("Save as 3D PDF File"), QString(), tr("PDF Files (*.pdf)"));
    if (pdfPath.isEmpty()) {
        ui->m_btnExport->setEnabled(true);
        return;
    }

    ui->m_progressBar->setValue(30);
    ui->m_lblStatus->setText(tr("Building OSG scene and writing PRC..."));
    QCoreApplication::processEvents(); // keep UI responsive

    std::string errMsg;
    bool ok = CoreApi::ExportApi::ExportToPrcPdf(
        m_compound,
        pdfPath.toStdString(),
        m_bgColor,
        errMsg);

    if (!ok) {
        ui->m_btnExport->setEnabled(true);
        QMessageBox::critical(this, tr("Export 3D PDF"),
            tr("Method 2 (PRC) export failed:\n%1").arg(QString::fromStdString(errMsg)));
        return;
    }

    ui->m_progressBar->setValue(100);
    ui->m_lblStatus->setText(tr("Done."));
    ui->m_btnExport->setEnabled(true);
    QMessageBox::information(this, tr("Export Successful"),
        tr("📄 3D PDF (PRC) saved to:\n%1").arg(pdfPath));
    accept();
}
#endif // USE_LIBPRC
