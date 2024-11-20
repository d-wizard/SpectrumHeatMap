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

int main(int argc, char *argv[])
{
   std::string inPath;
   double sampleRate = 0;
   size_t fftSize = 0;
   double timeBetweenFfts = 0;
   size_t numThreads = 1;
   std::string outPath;

   const char* argStr = "i:o:s:f:t:j:h";
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
      case 'h':
         printf("Help:\n -i : input file\n -o : output file\n -s : sample rate\n -f : FFT Size\n -t : Time Between FFTs\n -j : Num Threads\n");
         exit(0);
      break;
      default:
         // invalid arg
      break;
      }
   }

   if(inPath != "" && sampleRate > 0 && fftSize > 0 && timeBetweenFfts > 0 && outPath != "")
   {
      FileToHeatMap f2hm(inPath, sampleRate, fftSize, timeBetweenFfts, numThreads);
      f2hm.genHeatMap();
      f2hm.saveBmp(outPath, true);
   }
   else
   {
      printf("Invalid input config\n");
   }

}

