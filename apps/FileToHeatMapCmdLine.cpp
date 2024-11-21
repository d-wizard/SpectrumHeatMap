/* Copyright 2024 Dan Williams. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
 * to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#include <unistd.h>
#include "FileToHeatMap.h"


template<typename tSampType>
void GenHeatMap(const std::string& filePath, double sampleRate, size_t fftSize, double timeBetweenFfts, size_t numThreads, int64_t startPosition, int64_t endPosition, const std::string& outPath)
{
   FileToHeatMap<tSampType> f2hm(filePath, sampleRate, fftSize, timeBetweenFfts, numThreads, startPosition, endPosition);
   f2hm.genHeatMap();
   f2hm.saveBmp(outPath, true);
}

int main(int argc, char *argv[])
{
   std::string inPath;
   double sampleRate = 0;
   size_t fftSize = 0;
   double timeBetweenFfts = 0;
   size_t numThreads = 1;
   std::string outPath;
   std::string inputFormat;
   int64_t startPosition = 0;
   int64_t endPosition = 0;

   const char* argStr = "i:o:s:f:t:j:y:S:E:h";
   int option = -1;
   while((option = getopt(argc, argv, argStr)) != -1)
   {
      switch(option)
      {
      case 'i':
         inPath = std::string(optarg);
      break;
      case 'o':
         outPath = std::string(optarg);
      break;
      case 's':
         sampleRate = strtod(optarg, nullptr);
      break;
      case 'f':
         fftSize = strtoul(optarg, nullptr, 10);
      break;
      case 't':
         timeBetweenFfts = strtod(optarg, nullptr);
      break;
      case 'j':
         numThreads = strtoul(optarg, nullptr, 10);
      break;
      case 'y':
         inputFormat = std::string(optarg);
      break;
      case 'S':
         startPosition = strtoll(optarg, nullptr, 10);
      break;
      case 'E':
         endPosition = strtoll(optarg, nullptr, 10);
      break;
      case 'h':
         printf("Help:\n -i : input file\n -o : output file\n -s : sample rate\n -f : FFT Size\n -t : Time Between FFTs\n -y : Input Format (float, double, int16_t, etc)\n -j : Num Threads\n -S : In File Start Position\n -E : In File End Position\n");
         exit(0);
      break;
      default:
         // invalid arg
      break;
      }
   }

   if(inPath != "" && sampleRate > 0 && fftSize > 0 && timeBetweenFfts > 0 && outPath != "")
   {
           if(inputFormat == "int8_t")   {GenHeatMap<int8_t>  (inPath, sampleRate, fftSize, timeBetweenFfts, numThreads, startPosition, endPosition, outPath);}
      else if(inputFormat == "int16_t")  {GenHeatMap<int16_t> (inPath, sampleRate, fftSize, timeBetweenFfts, numThreads, startPosition, endPosition, outPath);}
      else if(inputFormat == "int32_t")  {GenHeatMap<int32_t> (inPath, sampleRate, fftSize, timeBetweenFfts, numThreads, startPosition, endPosition, outPath);}
      else if(inputFormat == "int64_t")  {GenHeatMap<int64_t> (inPath, sampleRate, fftSize, timeBetweenFfts, numThreads, startPosition, endPosition, outPath);}
      else if(inputFormat == "uint8_t")  {GenHeatMap<uint8_t> (inPath, sampleRate, fftSize, timeBetweenFfts, numThreads, startPosition, endPosition, outPath);}
      else if(inputFormat == "uint16_t") {GenHeatMap<uint16_t>(inPath, sampleRate, fftSize, timeBetweenFfts, numThreads, startPosition, endPosition, outPath);}
      else if(inputFormat == "uint32_t") {GenHeatMap<uint32_t>(inPath, sampleRate, fftSize, timeBetweenFfts, numThreads, startPosition, endPosition, outPath);}
      else if(inputFormat == "uint64_t") {GenHeatMap<uint64_t>(inPath, sampleRate, fftSize, timeBetweenFfts, numThreads, startPosition, endPosition, outPath);}
      else if(inputFormat == "float")    {GenHeatMap<float>   (inPath, sampleRate, fftSize, timeBetweenFfts, numThreads, startPosition, endPosition, outPath);}
      else if(inputFormat == "double")   {GenHeatMap<double>  (inPath, sampleRate, fftSize, timeBetweenFfts, numThreads, startPosition, endPosition, outPath);}
      else{printf("Invalid Input Format\n");}
   }
   else
   {
      printf("Invalid input config\n");
   }

}

