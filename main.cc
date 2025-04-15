// Author: Marc Comino 2020

#include <QApplication>
#include <QSurfaceFormat>
#include "./main_window.h"

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    QSurfaceFormat f;
    f.setVersion(3,3);
    f.setProfile(QSurfaceFormat::CoreProfile);
    QSurfaceFormat::setDefaultFormat(f);
  gui::MainWindow w;
  w.show();

  return a.exec();
}
