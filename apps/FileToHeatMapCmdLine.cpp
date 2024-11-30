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
void GenHeatMap(tFileToHeatMapConfig& config, const std::string& outPath, uint32_t maxFileSize)
{
   FileToHeatMap<tSampType> f2hm(config);
   f2hm.genHeatMap();
   if(maxFileSize == 0)
      f2hm.savePng(outPath + ".png", true);
   else
      f2hm.savePngSplit(outPath, maxFileSize, false); // savePngSplit doesnt' support rotated for now.
}

int main(int argc, char *argv[])
{
   tFileToHeatMapConfig config;
   config.sampleRate = 0;
   config.fftSize = 0;
   config.timeBetweenFfts = 0;
   config.numThreads = 1;
   config.startPosition = 0;
   config.endPosition = 0;
   std::string outPath;
   std::string inputFormat;
   uint32_t maxFileSize = 0; // 0 means don't split into smaller files.

   const char* argStr = "i:o:s:f:t:j:y:nm:r:S:E:M:h";
   int option = -1;
   while((option = getopt(argc, argv, argStr)) != -1)
   {
      switch(option)
      {
      case 'i':
         config.filePath = std::string(optarg);
      break;
      case 'o':
         outPath = std::string(optarg);
      break;
      case 's':
         config.sampleRate = strtod(optarg, nullptr);
      break;
      case 'f':
         config.fftSize = strtoul(optarg, nullptr, 10);
      break;
      case 't':
         config.timeBetweenFfts = strtod(optarg, nullptr);
      break;
      case 'j':
         config.numThreads = strtoul(optarg, nullptr, 10);
      break;
      case 'y':
         inputFormat = std::string(optarg);
      break;
      case 'n':
         config.normalizeHeatMap = true;
      break;
      case 'm':
         config.maxLevelDb = strtod(optarg, nullptr);
      break;
      case 'r':
         config.rangeDb = strtod(optarg, nullptr);
      break;
      case 'S':
         config.startPosition = strtoll(optarg, nullptr, 10);
      break;
      case 'E':
         config.endPosition = strtoll(optarg, nullptr, 10);
      break;
      case 'M':
         maxFileSize = strtoul(optarg, nullptr, 10);
      break;
      case 'h':
         printf("Help:\n -i : input file\n -o : output file (extension will be added)\n -s : sample rate\n -f : FFT Size\n -t : Time Between FFTs\n"
             " -y : Input Format (float, double, int16_t, etc)\n -j : Num Threads\n" 
             " -n : Use this to normalize max to the detected peak value.\n -m : Max FFT bin value in dB\n -r : Range of the Heat Map in dB\n"
             " -S : In File Start Position\n -E : In File End Position\n -M : Max number of FFTs per file (this will split Heat Map into multiple files)\n" );
         exit(0);
      break;
      default:
         // invalid arg
      break;
      }
   }

   if(config.filePath != "" && config.sampleRate > 0 && config.fftSize > 0 && config.timeBetweenFfts > 0 && outPath != "")
   {
           if(inputFormat == "int8_t")   {GenHeatMap<int8_t>  (config, outPath, maxFileSize);}
      else if(inputFormat == "int16_t")  {GenHeatMap<int16_t> (config, outPath, maxFileSize);}
      else if(inputFormat == "int32_t")  {GenHeatMap<int32_t> (config, outPath, maxFileSize);}
      else if(inputFormat == "int64_t")  {GenHeatMap<int64_t> (config, outPath, maxFileSize);}
      else if(inputFormat == "uint8_t")  {GenHeatMap<uint8_t> (config, outPath, maxFileSize);}
      else if(inputFormat == "uint16_t") {GenHeatMap<uint16_t>(config, outPath, maxFileSize);}
      else if(inputFormat == "uint32_t") {GenHeatMap<uint32_t>(config, outPath, maxFileSize);}
      else if(inputFormat == "uint64_t") {GenHeatMap<uint64_t>(config, outPath, maxFileSize);}
      else if(inputFormat == "float")    {GenHeatMap<float>   (config, outPath, maxFileSize);}
      else if(inputFormat == "double")   {GenHeatMap<double>  (config, outPath, maxFileSize);}
      else{printf("Invalid Input Format\n");}
   }
   else
   {
      printf("Invalid input config\n");
   }

}

