// Author: Imanol Munoz-Pandiella 2023 based on Marc Comino 2020

#include <main_window.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QColorDialog>
#include "./ui_main_window.h"

namespace gui {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::show() { QMainWindow::show(); }

void MainWindow::on_actionQuit_triggered() { close(); }

void MainWindow::on_actionLoad_triggered() {
  QString filename;

  filename = QFileDialog::getOpenFileName(this, tr("Load model"), "./",
                                          tr("3D Files ( *.ply *.obj )"));
  if (!filename.isNull()) {
    if (!ui->glwidget->LoadModel(filename))
      QMessageBox::warning(this, tr("Error"),
                           tr("The file could not be opened"));
  }
}

void MainWindow::on_actionLoad_Specular_triggered() {
  QString dir =
      QFileDialog::getExistingDirectory(this, "Specular CubeMap folder.", "./");
  if (!dir.isEmpty()) {
    if (!ui->glwidget->LoadSpecularMap(dir))
      QMessageBox::warning(this, tr("Error"),
                           tr("The file could not be opened"));
  }
}

void MainWindow::on_actionLoad_WeightedSpecular_triggered() {
  QString dir =
      QFileDialog::getExistingDirectory(this, "Weighted Specular CubeMap folder.", "./");
  if (!dir.isEmpty()) {
    if (!ui->glwidget->LoadWeightedSpecularMap(dir))
      QMessageBox::warning(this, tr("Error"),
                           tr("The file could not be opened"));
  }
}

void MainWindow::on_actionLoad_BrdfLUT_triggered() {
  QString file =
      QFileDialog::getOpenFileName(this, "BRDF LUT texture.", "./");
  if (!file.isEmpty()) {
    if (!ui->glwidget->LoadBRDFLUTMap(file))
      QMessageBox::warning(this, tr("Error"),
                           tr("The file could not be opened"));
  }
}

void MainWindow::on_actionLoad_Diffuse_triggered() {
  QString dir =
      QFileDialog::getExistingDirectory(this, "Diffuse CubeMap folder.", "./");
  if (!dir.isEmpty()) {
    if (!ui->glwidget->LoadDiffuseMap(dir))
      QMessageBox::warning(this, tr("Error"),
                           tr("The file could not be opened"));
  }
}

void MainWindow::on_actionLoad_Color_triggered()
{
    QString file =
        QFileDialog::getOpenFileName(this, "Color texture.", "./");
    if (!file.isEmpty()) {
      if (!ui->glwidget->LoadColorMap(file))
        QMessageBox::warning(this, tr("Error"),
                             tr("The file could not be opened"));
    }
}

void MainWindow::on_actionLoad_Roughness_triggered()
{
    QString file =
        QFileDialog::getOpenFileName(this, "Color texture.", "./");
    if (!file.isEmpty()) {
      if (!ui->glwidget->LoadRoughnessMap(file))
        QMessageBox::warning(this, tr("Error"),
                             tr("The file could not be opened"));
    }
}

void MainWindow::on_actionLoad_Metalness_triggered()
{
    QString file =
        QFileDialog::getOpenFileName(this, "Color texture.", "./");
    if (!file.isEmpty()) {
      if (!ui->glwidget->LoadMetalnessMap(file))
        QMessageBox::warning(this, tr("Error"),
                             tr("The file could not be opened"));
    }
}

void gui::MainWindow::on_button_Albedo_Color_clicked() {
  QColor color = QColorDialog::getColor(Qt::white, this, "Select Albedo Color");
  
  if (color.isValid()) {
      double r = color.redF();   // Convert to 0.0-1.0 range
      double g = color.greenF(); // Convert to 0.0-1.0 range
      double b = color.blueF();  // Convert to 0.0-1.0 range
      
      ui->glwidget->SetAlbedo(r, g, b);
  }
}

}  //  namespace gui
