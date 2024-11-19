#include <algorithm>
#include "FileToHeatMap.h"
#include "fftHelper.h"
#include "hsvrgb.h"
#include "BitmapPlusPlus.hpp"

// #define FORCE_HUE_FFT


FileToHeatMap::FileToHeatMap(const std::string& filePath, double sampleRate, size_t fftSize, double timeBetweenFfts)
   : m_filePath(filePath)
   , m_sampleRate(sampleRate)
   , m_fftSize(fftSize)
   , m_timeBetweenFfts(timeBetweenFfts)
{
   try
   {
      m_fileStream.open(m_filePath.c_str(), std::ios::binary);

      // Get the size of the file.
      m_fileStream.seekg(0, std::ios::end);
      m_fileSizeBytes = (size_t)m_fileStream.tellg();
      m_fileStream.seekg(0, std::ios::beg); // Set back to the beginning

      m_numSamples = m_fileSizeBytes / 4;

      m_sampBetweenFfts = size_t(m_sampleRate * m_timeBetweenFfts + 0.5);
      m_numFfts = m_numSamples / m_sampBetweenFfts;

      m_rgb.resize(3*m_numFfts*m_fftSize);
      m_fft_dB.resize(m_numFfts*m_fftSize);
   }
   catch(...)
   {
      m_fftSize = 0;
      m_numFfts = 0;
   }
}

FileToHeatMap::~FileToHeatMap()
{
   
}

void FileToHeatMap::genHeatMap()
{
   auto fftParam = std::make_shared<tFftParam>(m_fftSize);
   for(size_t fftNum = 0; fftNum < m_numFfts; ++fftNum)
   {
      readFromFile(fftParam, fftNum);
      doFft(fftParam);
   }
}

void FileToHeatMap::readFromFile(std::shared_ptr<tFftParam> param, size_t fftNum)
{
   m_fileStream.seekg(4*fftNum*m_sampBetweenFfts, std::ios::beg);
   m_fileStream.read(reinterpret_cast<char*>(param->iqSamples.data()), 4*m_fftSize);
   param->fftWritePtr = &m_fft_dB[fftNum*m_fftSize];
}

void FileToHeatMap::doFft(std::shared_ptr<tFftParam> param)
{
   // De-interleave / convert to double.
   for(size_t i = 0; i < m_fftSize; ++i)
   {
      param->iSamples[i] = param->iqSamples[2*i+0];
      param->qSamples[i] = param->iqSamples[2*i+1];
   }

   // Run the FFT.
   complexFFT(param->iSamples, param->qSamples, param->fftRe, param->fftIm);

   // Store FFT Magnatude information.
   double* fftRe = param->fftRe.data();
   double* fftIm = param->fftIm.data();
   double* fftDbPtr = param->fftWritePtr;

   // Fill in index 0 outside of the for loop so we can initialize min / max.
   fftDbPtr[0] = 10.0 * log10(fftRe[0] * fftRe[0] + fftIm[0] * fftIm[0]);
   param->fftMax = fftDbPtr[0];
   param->fftMin = fftDbPtr[0];

   for(size_t i = 0; i < m_fftSize; ++i)
   {
      fftDbPtr[i] = 10.0 * log10(fftRe[i] * fftRe[i] + fftIm[i] * fftIm[i]);
      if(fftDbPtr[i] > param->fftMax)
         param->fftMax = fftDbPtr[i];
      else if(fftDbPtr[i] < param->fftMin)
         param->fftMin = fftDbPtr[i];
   }
}

void FileToHeatMap::fftToRgb()
{

   constexpr double MAX_DB_FS_VAL = 90;
   constexpr double MIN_DB_FS_VAL = -30;
   constexpr double DELTA_DB_FS_VAL = MAX_DB_FS_VAL - MIN_DB_FS_VAL;

#ifdef FORCE_HUE_FFT
   size_t fftBin = 0;
#endif

   uint8_t* rgbWritePtr = m_rgb.data();
   for(size_t i = 0; i < m_fftSize*m_numFfts; ++i)
   {
      HsvColor hsv = {0,0xff,0xff};
      double normVal = (m_fft_dB[i] - MIN_DB_FS_VAL) / DELTA_DB_FS_VAL;
      if(normVal > 1.0){normVal = 1.0;}
      if(normVal < 0.0){normVal = 0.0;}

      hsv.h = uint8_t((1.0-normVal)*255.0);

#ifdef FORCE_HUE_FFT
      hsv.h = uint8_t(double(fftBin)*255.0/double(m_fftSize));
#endif

      hsv.v = 255-hsv.h; // Set brightness inverse to hue (i.e. brightness goes down when FFT mag goes down).

      auto rgb = HsvToRgb(hsv);
      rgbWritePtr[3*i+0] = rgb.r;
      rgbWritePtr[3*i+1] = rgb.g;
      rgbWritePtr[3*i+2] = rgb.b;

#ifdef FORCE_HUE_FFT
      if(++fftBin >= m_fftSize){fftBin=0;}
#endif
   }
}

void FileToHeatMap::saveBmp(const std::string& savePath, bool rotate)
{
   fftToRgb();
   if(rotate)
   {
      // X Axis is Time, Y Axis is Frequency
      bmp::Bitmap image(m_numFfts, m_fftSize);
      size_t fftBinIndex = 0;
      size_t fftIndex = 0;
      for (bmp::Pixel &pixel: image)
      {
         size_t i = m_fftSize*fftIndex+fftBinIndex;
         pixel.r = m_rgb[3*i+0];
         pixel.g = m_rgb[3*i+1];
         pixel.b = m_rgb[3*i+2];
         if(++fftIndex == m_numFfts)
         {
            fftIndex = 0;
            ++fftBinIndex;
         }
      }
      image.save(savePath);
   }
   else
   {
      // X Axis is Frequency, Y Axis is Time
      bmp::Bitmap image(m_fftSize, m_numFfts);
      size_t i = 0;
      for (bmp::Pixel &pixel: image)
      {
         pixel.r = m_rgb[3*i+0];
         pixel.g = m_rgb[3*i+1];
         pixel.b = m_rgb[3*i+2];
         ++i;
      }
      image.save(savePath);
   }
}
