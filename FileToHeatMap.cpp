#include "FileToHeatMap.h"
#include "fftHelper.h"
#include "hsvrgb.h"


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

      m_sampBetweenFfts = size_t(m_sampleRate / m_timeBetweenFfts + 0.5);
      m_numFfts = m_numSamples / m_sampBetweenFfts;

      m_rgb.resize(3*m_numFfts*m_fftSize);
   }
   catch(...)
   {
   }
}

FileToHeatMap::~FileToHeatMap()
{
   
}

void FileToHeatMap::run()
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
   param->rgbPtr = &m_rgb[3*fftNum*m_fftSize];
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

   // Convert to RGB.
   HsvColor hsv = {0,0xff,0xff};

   constexpr double MAX_DB_FS_VAL = 90;
   constexpr double MIN_DB_FS_VAL = 0;
   constexpr double DELTA_DB_FS_VAL = MAX_DB_FS_VAL - MIN_DB_FS_VAL;

   double* fftRe = param->fftRe.data();
   double* fftIm = param->fftIm.data();
   uint8_t* rgbWritePtr = param->rgbPtr;

   for(size_t i = 0; i < m_fftSize; ++i)
   {
      double fftDb = 10.0 * log10(fftRe[i] * fftRe[i] + fftIm[i] * fftIm[i]);
      double normVal = (fftDb - MIN_DB_FS_VAL) / DELTA_DB_FS_VAL;
      if(normVal > 1.0){normVal = 1.0;}
      if(normVal < 0.0){normVal = 0.0;}
      hsv.h = uint8_t(normVal*100.0+155.0);
      auto rgb = HsvToRgb(hsv);
      rgbWritePtr[3*i+0] = rgb.r;
      rgbWritePtr[3*i+1] = rgb.g;
      rgbWritePtr[3*i+2] = rgb.b;
   }
   
}
