


#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QLabel>
#include <QImage>
#include <QPixmap>
#include <math.h>

#include "fftHelper.h"
#include "hsvrgb.h"

void makeComplexSineWave(double sampRate, double waveFreq, size_t numSamp, dubVect& re, dubVect& im)
{
   re.resize(numSamp);
   im.resize(numSamp);
   double phaseIncr = 2.0 * M_PI * waveFreq / sampRate;
   double phase = 0;
   for(size_t i = 0; i < numSamp; ++i)
   {
      re[i] = cos(phase);
      im[i] = sin(phase);
      phase += phaseIncr;
   }
}

void complexPowerFFT(dubVect& re, dubVect& im, dubVect& out)
{
   size_t numSamp = std::min(re.size(), im.size());
   dubVect fftRe(numSamp);
   dubVect fftIm(numSamp);
   complexFFT(re, im, fftRe, fftIm);
   out.resize(numSamp);
   for(size_t i = 0; i < numSamp; ++i)
   {
      out[i] = 10.0 * log10(fftRe[i] * fftRe[i] + fftIm[i] * fftIm[i]);
   }
}

void fftToRgb(dubVect& fft, char* rgbWritePtr)
{
   size_t numSamp = fft.size();
   HsvColor hsv = {0,0,0};

   constexpr double MAX_DB_FS_VAL = 0;
   constexpr double MIN_DB_FS_VAL = -90;
   constexpr double DELTA_DB_FS_VAL = MAX_DB_FS_VAL - MIN_DB_FS_VAL;

   for(size_t i = 0; i < numSamp; ++i)
   {
      double normVal = (fft[i] - MIN_DB_FS_VAL) / DELTA_DB_FS_VAL;
      if(normVal > 1.0){normVal = 1.0;}
      if(normVal < 0.0){normVal = 0.0;}
      hsv.h = uint8_t(normVal*255.0);
      auto rgb = HsvToRgb(hsv);
      rgbWritePtr[3*i+0] = rgb.r;
      rgbWritePtr[3*i+1] = rgb.g;
      rgbWritePtr[3*i+2] = rgb.b;
   }
}

void MainWindow::displayTestSquare()
{
   int height = 256;
   int width = 768;
   QString header = QString("P6 %1 %2 255 ").arg(width).arg(height);
   QByteArray pixData(header.toStdString().c_str(), header.size());
   auto headerSize = pixData.size();
   pixData.resize(headerSize+height*width*3);

   for(int i = 0; i < (height*width); ++i)
   {
      pixData[headerSize+3*i+0] = 0xff;
      pixData[headerSize+3*i+1] = 0xff;
      pixData[headerSize+3*i+2] = 0;
   }

   QPixmap pixmap(height, width);
   pixmap.loadFromData((uchar *)pixData.data(), pixData.size());
   ui->label->setPixmap(pixmap);
}

void MainWindow::displayHeatMap()
{
   int height = 256; // FFT Size
   int width = 768;

   // Write the header
   QString header = QString("P6 %1 %2 255 ").arg(width).arg(height);
   QByteArray pixData(header.toStdString().c_str(), header.size());

   // Allocate all the memory.
   auto headerSize = pixData.size();
   pixData.resize(headerSize+height*width*3);

   // Set the pointer.
   char* pixelWritePtr = pixData.data()+headerSize;
   dubVect re;
   dubVect im;
   dubVect fft;
   makeComplexSineWave(64e3, 8e3, height, re, im);
   complexPowerFFT(re, im, fft);
   for(int i = 0; i < width; ++i)
   {
      fftToRgb(fft, pixelWritePtr);
      pixelWritePtr += (3*height);
   }

   QPixmap pixmap(height, width);
   pixmap.loadFromData((uchar *)pixData.data(), pixData.size());
   ui->label->setPixmap(pixmap);
}

MainWindow::MainWindow(QWidget *parent)
   : QMainWindow(parent)
   , ui(new Ui::MainWindow)
{
   ui->setupUi(this);
   //displayTestSquare();
   displayHeatMap();
}

MainWindow::~MainWindow()
{
   delete ui;
}

