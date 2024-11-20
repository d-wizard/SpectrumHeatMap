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

class FileToHeatMap
{
public:
   FileToHeatMap(const std::string& filePath, double sampleRate, size_t fftSize, double timeBetweenFfts, size_t numThreads = 1);
   virtual ~FileToHeatMap();

   void genHeatMap();

   void saveBmp(const std::string& savePath, bool rotate = false);

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
      std::vector<int16_t> iqSamples;
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

   // FFT Results
   std::vector<double> m_fft_dB;
   std::vector<uint8_t> m_rgb;

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
   std::shared_ptr<tFftParam> getAvailableFftThread(std::unique_lock<std::mutex>& lock);

   void readFromFile(std::shared_ptr<tFftParam> param, size_t fftNum);
   void doFft(std::shared_ptr<tFftParam> param);
   void fftToRgb();

};

