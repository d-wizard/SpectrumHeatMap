#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>

class FileToHeatMap
{
public:
   FileToHeatMap(const std::string& filePath, double sampleRate, size_t fftSize, double timeBetweenFfts);
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
   typedef struct tFftParam
   {
      std::vector<int16_t> iqSamples;
      std::vector<double> iSamples;
      std::vector<double> qSamples;
      std::vector<double> fftRe;
      std::vector<double> fftIm;

      double* fftWritePtr;
      double fftMax;
      double fftMin;

      tFftParam(size_t fftSize): iqSamples(2*fftSize), iSamples(fftSize), qSamples(fftSize), fftRe(fftSize), fftIm(fftSize){}
   }tFftParam;

   std::string m_filePath;
   double m_sampleRate = 1.0;
   size_t m_fftSize = 1;
   double m_timeBetweenFfts = 1.0;
   size_t m_sampBetweenFfts = 1;
   size_t m_numFfts = 0;
   size_t m_numSamples = 0;

   std::ifstream m_fileStream;
   size_t m_fileSizeBytes = 0;

   std::vector<double> m_fft_dB;

   std::vector<uint8_t> m_rgb;

   void readFromFile(std::shared_ptr<tFftParam> param, size_t fftNum);
   void doFft(std::shared_ptr<tFftParam> param);
   void fftToRgb();

};

