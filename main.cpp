#include "mainwindow.h"

#include <QApplication>

#include "smartPlotMessage.h"

int main(int argc, char *argv[])
{
   smartPlot_createFlushThread(100);

   QApplication a(argc, argv);
   MainWindow w;
   w.show();
   return a.exec();
}
