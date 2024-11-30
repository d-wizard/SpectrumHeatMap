#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <list>
#include <memory>
#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <type_traits> // Used to determine if template type is floating point or not.
#include "fftHelper.h"
#include "hsvrgb.h"
#include "BitmapPlusPlus.hpp"
#include "fpng.h"

typedef struct tFileToHeatMapConfig
{
   std::string filePath = "";
   double sampleRate = 1.0;
   size_t fftSize = 1024;
   double timeBetweenFfts = 1.0;
   size_t numThreads = 1;
   int64_t startPosition = 0;
   int64_t endPosition = 0;
   bool normalizeHeatMap = false;
   double maxLevelDb = std::numeric_limits<double>::infinity(); // init to invalid value
   double rangeDb = 100.0;
} tFileToHeatMapConfig;   

template<typename tSampType>
class FileToHeatMap
{
public:
   // Public Constants
   static constexpr size_t COMPLEX_SAMP_SIZE = 2*sizeof(tSampType);

public:
   FileToHeatMap(const tFileToHeatMapConfig& config);
   virtual ~FileToHeatMap();

   void genHeatMap();

   void saveBmp(const std::string& savePath, bool rotate = false);
   void savePng(const std::string& savePath, bool rotate = false);

   void savePngSplit(const std::string& savePathNoExt, size_t maxNumFftsPerFile, bool rotate = false);

   size_t getFftSize(){return m_fftSize;}
   size_t getNumFfts(){return m_numFfts;}
   uint8_t* getRgb(){return m_rgb.data();}

private:
   // Make uncopyable
   FileToHeatMap();
   FileToHeatMap(FileToHeatMap const&);
   void operator=(FileToHeatMap const&);

private:
   /////////////////////////////////////////////////////////////////////////////
   // Types
   /////////////////////////////////////////////////////////////////////////////
   typedef struct tFftParam
   {
      std::vector<tSampType> iqSamples;
      std::vector<double> iSamples;
      std::vector<double> qSamples;
      std::vector<double> fftRe;
      std::vector<double> fftIm;
      double* fftWritePtr;

      std::thread fftThread;

      tFftParam(size_t fftSize): iqSamples(2*fftSize), iSamples(fftSize), qSamples(fftSize), fftRe(fftSize), fftIm(fftSize){}
   }tFftParam;
   typedef std::shared_ptr<tFftParam> tFftParamPtr;

   /////////////////////////////////////////////////////////////////////////////
   // Member Variables
   /////////////////////////////////////////////////////////////////////////////

   // Settings
   std::string m_filePath;
   double m_sampleRate = 1.0;
   size_t m_fftSize = 1;
   double m_timeBetweenFfts = 1.0;
   size_t m_numThreads = 1;

   size_t m_sampBetweenFfts = 1;
   size_t m_numFfts = 0;
   size_t m_numSamples = 0;

   std::ifstream m_fileStream;
   size_t m_fileSizeBytes = 0;
   size_t m_fileStartOffset = 0;

   // FFT Window
   std::vector<double> m_fftWindow;

   // FFT Results
   std::vector<double> m_fft_dB;
   std::vector<uint8_t> m_rgb;

   bool m_normalizeHeatMap = false;
   double m_fftToRgb_max_dB = 0; // Any dB value above this will be the max RGB value.
   double m_fftToRgb_range_dB;

   // Threading
   std::list<tFftParamPtr> m_fftThreadsAvailable;
   std::mutex m_threadMutex;
   std::condition_variable m_threadCondVar;

   // Stats
   bool m_fftMaxMinNeedInit = true;
   double m_fftMax_dB = 0;
   double m_fftMin_dB = 0;


   /////////////////////////////////////////////////////////////////////////////
   // Private Member Functions
   /////////////////////////////////////////////////////////////////////////////
   void readFromFile(std::shared_ptr<tFftParam> param, size_t fftNum);
   void doFft(std::shared_ptr<tFftParam> param);
   void fftToRgb(bool rotate);

};


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


template<typename tSampType>
FileToHeatMap<tSampType>::FileToHeatMap(const tFileToHeatMapConfig& config)
   : m_filePath(config.filePath)
   , m_sampleRate(config.sampleRate)
   , m_fftSize(config.fftSize)
   , m_timeBetweenFfts(config.timeBetweenFfts)
   , m_numThreads(config.numThreads) 
{
   try
   {
      m_fileStream.open(m_filePath.c_str(), std::ios::binary);

      // Get the size of the file.
      m_fileStream.seekg(0, std::ios::end);
      m_fileSizeBytes = (size_t)m_fileStream.tellg();
      m_fileStream.seekg(0, std::ios::beg); // Set back to the beginning

      // Convert input postion Values to within range of the file (interpret as slicing indexes)
      bool validStartEndPos = true;
      int64_t fileSizeBytesSigned = int64_t(m_fileSizeBytes);
      auto startPosition = config.startPosition;
      auto endPosition = config.endPosition;
      if(validStartEndPos)
      {
         if(startPosition < 0)
         {
            startPosition += fileSizeBytesSigned;
            if(startPosition < 0)
               startPosition = 0; // Start at the beginning.
         }
         else if(startPosition >= fileSizeBytesSigned)
            validStartEndPos = false; // Not enough data in the file. Drop the entire file.
      }
      if(validStartEndPos)
      {
         if(endPosition <= 0)
         {
            endPosition += fileSizeBytesSigned;
            if(endPosition <= 0)
               validStartEndPos = false; // Invalid. Drop the entire file.
         }
         else if(endPosition > fileSizeBytesSigned)
            endPosition = fileSizeBytesSigned; // Read to the end of the file.
      }
      if(startPosition >= endPosition)
         validStartEndPos = false; // Not enough data in the file. Drop the entire file.

      if(validStartEndPos)
      {
         m_numSamples = (endPosition - startPosition) / COMPLEX_SAMP_SIZE;
         m_fileStartOffset = startPosition;
      }
      else
         m_numSamples = 0;

      m_sampBetweenFfts = size_t(m_sampleRate * m_timeBetweenFfts + 0.5);
      m_numFfts = m_numSamples / m_sampBetweenFfts;

      // Make sure FFTs don't extend beyond the end of the file.
      if(m_numFfts > 0)
      {
         size_t stopIndex = (m_numFfts-1) * m_sampBetweenFfts + m_fftSize;
         while(m_numFfts > 0 && stopIndex > m_numSamples)
         {
            --m_numFfts;
            stopIndex = (m_numFfts-1) * m_sampBetweenFfts + m_fftSize;
         }
      }

      m_rgb.resize(3*m_numFfts*m_fftSize);
      m_fft_dB.resize(m_numFfts*m_fftSize);

      // Determine Max FFT value
      m_normalizeHeatMap = config.normalizeHeatMap;
      if(std::isfinite(config.maxLevelDb))
         m_fftToRgb_max_dB = config.maxLevelDb; // Use the user specified value.
      else if(std::is_floating_point<tSampType>())
         m_fftToRgb_max_dB = 0; // Floating point values could be anything. Set max to 0 dB
      else
         m_fftToRgb_max_dB = 20.0 * log10(double(std::numeric_limits<tSampType>::max())); // Set to max

      m_fftToRgb_range_dB = config.rangeDb;
      if(!std::isfinite(m_fftToRgb_range_dB) || m_fftToRgb_range_dB <= 0 || m_fftToRgb_range_dB > 1000) // Make sure the value makes sense.
      {
         m_fftToRgb_range_dB = 100;
      }

      // Generate Window Coefs
      m_fftWindow.resize(m_fftSize);
      genWindowCoef(m_fftWindow.data(), m_fftSize, true);

      // Create Worker Thread Params
      if(m_numThreads <= 0){m_numThreads = 1;}
      for(size_t i = 0; i < m_numThreads; ++i)
      {
         m_fftThreadsAvailable.emplace_back(std::make_shared<tFftParam>(m_fftSize));
      }
   }
   catch(...)
   {
      m_fftSize = 0;
      m_numFfts = 0;
   }
}

////////////////////////////////////////////////////////////////////////////////

template<typename tSampType>
FileToHeatMap<tSampType>::~FileToHeatMap()
{
   
}

////////////////////////////////////////////////////////////////////////////////

template<typename tSampType>
void FileToHeatMap<tSampType>::genHeatMap()
{
   std::unique_lock<std::mutex> lock(m_threadMutex);
   for(size_t fftNum = 0; fftNum < m_numFfts; ++fftNum)
   {
      // Get the next available thread.
      while(m_fftThreadsAvailable.size() == 0)
      {
         m_threadCondVar.wait(lock);
      }
      auto fftParam = m_fftThreadsAvailable.front();
      m_fftThreadsAvailable.pop_front();
    
      // Read the bytes out of the file, then activate a thread for processing.
      readFromFile(fftParam, fftNum);
      fftParam->fftThread = std::thread(&FileToHeatMap::doFft, this, fftParam);
      fftParam->fftThread.detach();
   }
   
   // Wait until all thread have finished before returning.
   while(m_fftThreadsAvailable.size() < m_numThreads)
   {
      m_threadCondVar.wait(lock);
   }
}

////////////////////////////////////////////////////////////////////////////////

template<typename tSampType>
void FileToHeatMap<tSampType>::readFromFile(std::shared_ptr<tFftParam> param, size_t fftNum)
{
   m_fileStream.seekg(COMPLEX_SAMP_SIZE*fftNum*m_sampBetweenFfts+m_fileStartOffset, std::ios::beg);
   m_fileStream.read(reinterpret_cast<char*>(param->iqSamples.data()), COMPLEX_SAMP_SIZE*m_fftSize);
   param->fftWritePtr = &m_fft_dB[fftNum*m_fftSize];
}

////////////////////////////////////////////////////////////////////////////////

template<typename tSampType>
void FileToHeatMap<tSampType>::doFft(std::shared_ptr<tFftParam> param)
{
   // De-interleave / convert to double.
   for(size_t i = 0; i < m_fftSize; ++i)
   {
      param->iSamples[i] = param->iqSamples[2*i+0];
      param->qSamples[i] = param->iqSamples[2*i+1];
   }

   // Run the FFT.
   complexFFT(m_threadMutex, param->iSamples, param->qSamples, param->fftRe, param->fftIm, m_fftWindow.data());

   // Store FFT Magnitude information.
   double* fftRe = param->fftRe.data();
   double* fftIm = param->fftIm.data();
   double* fftDbPtr = param->fftWritePtr;
   double fftMax = 0;
   double fftMin = 0;

   // Fill in index 0 outside of the for loop so we can initialize min / max.
   fftDbPtr[0] = 10.0 * log10(fftRe[0] * fftRe[0] + fftIm[0] * fftIm[0]);
   fftMax = fftDbPtr[0];
   fftMin = fftDbPtr[0];

   for(size_t i = 0; i < m_fftSize; ++i)
   {
      fftDbPtr[i] = 10.0 * log10(fftRe[i] * fftRe[i] + fftIm[i] * fftIm[i]);
      if(fftDbPtr[i] > fftMax)
         fftMax = fftDbPtr[i];
      else if(fftDbPtr[i] < fftMin)
         fftMin = fftDbPtr[i];
   }

   // Store stats and put the thread in the finish list.
   std::lock_guard<std::mutex> lock(m_threadMutex);
   if(m_fftMaxMinNeedInit)
   {
      m_fftMaxMinNeedInit = false;
      m_fftMax_dB = fftMax;
      m_fftMin_dB = fftMin;
   }
   else
   {
      if(fftMax > m_fftMax_dB)
         m_fftMax_dB = fftMax;
      else if(fftMin < m_fftMin_dB)
         m_fftMin_dB = fftMin;
   }
   m_fftThreadsAvailable.push_back(param);
   m_threadCondVar.notify_all();
}

////////////////////////////////////////////////////////////////////////////////

template<typename tSampType>
void FileToHeatMap<tSampType>::fftToRgb(bool rotate)
{
   const double MAX_DB_FS_VAL = m_normalizeHeatMap ? m_fftMax_dB : m_fftToRgb_max_dB;
   const double MIN_DB_FS_VAL = MAX_DB_FS_VAL - m_fftToRgb_range_dB;
   const double DELTA_DB_FS_VAL = MAX_DB_FS_VAL - MIN_DB_FS_VAL;

   uint8_t* rgbWritePtr = m_rgb.data();
   size_t fftBinIndex = 0;
   size_t fftIndex = 0;
   size_t numPixels = m_numFfts*m_fftSize;
   for(size_t outIndex = 0; outIndex < numPixels; ++outIndex)
   {
      size_t inIndex = rotate ? m_fftSize*fftIndex+fftBinIndex : outIndex;
      double normVal = (m_fft_dB[inIndex] - MIN_DB_FS_VAL) / DELTA_DB_FS_VAL;
      if(normVal > 1.0){normVal = 1.0;}
      if(normVal < 0.0){normVal = 0.0;}
      uint8_t fftNormVal = uint8_t((1.0-normVal)*255.0);

      // Lookup table based
      extern RgbColor LevelToRgbLookup[256];
      rgbWritePtr[3*outIndex+0] = LevelToRgbLookup[fftNormVal].r;
      rgbWritePtr[3*outIndex+1] = LevelToRgbLookup[fftNormVal].g;
      rgbWritePtr[3*outIndex+2] = LevelToRgbLookup[fftNormVal].b;
      
      if(rotate)
      {
         if(++fftIndex == m_numFfts)
         {
            fftIndex = 0;
            ++fftBinIndex;
         }
      }
   }
}

////////////////////////////////////////////////////////////////////////////////

template<typename tSampType>
void FileToHeatMap<tSampType>::saveBmp(const std::string& savePath, bool rotate)
{
   fftToRgb(rotate);
   size_t height = rotate ? m_fftSize : m_numFfts;
   size_t width  = rotate ? m_numFfts : m_fftSize;
   bmp::Bitmap image(width, height);
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

////////////////////////////////////////////////////////////////////////////////

template<typename tSampType>
void FileToHeatMap<tSampType>::savePng(const std::string& savePath, bool rotate)
{
   fftToRgb(rotate);
   fpng::fpng_init();
   size_t height = rotate ? m_fftSize : m_numFfts;
   size_t width  = rotate ? m_numFfts : m_fftSize;
   fpng::fpng_encode_image_to_file(savePath.c_str(), m_rgb.data(), width, height, 3, fpng::FPNG_ENCODE_SLOWER);

}

////////////////////////////////////////////////////////////////////////////////

template<typename tSampType>
void FileToHeatMap<tSampType>::savePngSplit(const std::string& savePathNoExt, size_t maxNumFftsPerFile, bool rotate)
{
   if(rotate)
      return; // Doesn't work for now.
   fftToRgb(rotate);
   fpng::fpng_init();

   size_t fileIndex = 0;
   size_t fftIndex = 0;
   while(fftIndex < m_numFfts)
   {
      // Determine how many FFTs to put in this file (i.e. the last file might be smaller than maxNumFftsPerFile)
      size_t numFftsInThisFile = (fftIndex+maxNumFftsPerFile) <= m_numFfts ? maxNumFftsPerFile : (m_numFfts-fftIndex);

      // Determine the image file parameters and save the file.
      size_t height = rotate ? m_fftSize : numFftsInThisFile;
      size_t width  = rotate ? numFftsInThisFile : m_fftSize;
      std::string savePath = savePathNoExt + "_" + std::to_string(fileIndex) + ".png";
      fpng::fpng_encode_image_to_file(savePath.c_str(), &m_rgb[3*fftIndex*m_fftSize], width, height, 3, fpng::FPNG_ENCODE_SLOWER);

      // Update loop parameters.
      fftIndex += numFftsInThisFile;
      ++fileIndex;
   }
}

