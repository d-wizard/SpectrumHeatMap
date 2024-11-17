/* Copyright 2013 - 2015, 2017, 2019 Dan Williams. All Rights Reserved.
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
#include <fftw3.h>
#include <math.h>
#include "fftHelper.h"

// Overwrite NaN samples at the beginning with 0's
// There are many reasons why samples at the beginning might be NaN values:
// Scroll mode, FM Demod, etc...
static void fixStartNanComplex(fftw_complex* in, unsigned int N)
{
   for(unsigned int i = 0; i < N; ++i)
   {
      bool goodRe = true;
      bool goodIm = true;
      if(!isDoubleValid(in[i][0])) // Check if Real Value is NaN
      {
         goodRe = false;
         in[i][0] = 0;
      }
      if(!isDoubleValid(in[i][1])) // Check if Imaginary Value is NaN
      {
         goodIm = false;
         in[i][1] = 0;
      }

      if(goodRe && goodIm)
      {
         break;
      }
   }
}

// Overwrite NaN samples at the beginning with 0's
// There are many reasons why samples at the beginning might be NaN values:
// Scroll mode, FM Demod, etc...
static void fixStartNanReal(fftw_complex* in, unsigned int N)
{
   for(unsigned int i = 0; i < N; ++i)
   {
      bool good = true;
      if(!isDoubleValid(in[i][0])) // Check if Real Value is NaN (note: imaginary value is the same as real value).
      {
         good = false;
         in[i][0] = 0;
         in[i][1] = 0;
      }

      if(good)
      {
         break;
      }
   }
}

void complexFFT(const dubVect& inRe, const dubVect& inIm, dubVect& outRe, dubVect& outIm, double *windowCoef)
{
   fftw_complex *in, *out;
   fftw_plan p;

   unsigned int N = std::min(inRe.size(), inIm.size());

   if(N > 0)
   {
       in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
       out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);

       if(windowCoef == NULL)
       {
          for(unsigned int i = 0; i < N; ++i)
          {
             in[i][0] = inRe[i];
             in[i][1] = inIm[i];
          }
       }
       else
       {
          for(unsigned int i = 0; i < N; ++i)
          {
             in[i][0] = inRe[i] * windowCoef[i];
             in[i][1] = inIm[i] * windowCoef[i];
          }
       }

       // Overwrite NaN samples at the beginning with 0's
       fixStartNanComplex(in, N);

       p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

       fftw_execute(p);

       fftw_destroy_plan(p);

       outRe.resize(N);
       outIm.resize(N);

       // Swap FFT result to put DC in the center.
       unsigned int numEndFftPointsToSwap = N >> 1; // round down
       unsigned int numBeginFftPointsToSwap = N - numEndFftPointsToSwap;

       for(unsigned int i = 0; i < numEndFftPointsToSwap; ++i)
       {
          outRe[i] = out[i+numBeginFftPointsToSwap][0] / (double)N;
          outIm[i] = out[i+numBeginFftPointsToSwap][1] / (double)N;
       }
       for(unsigned int i = numEndFftPointsToSwap; i < N; ++i)
       {
          outRe[i] = out[i-numEndFftPointsToSwap][0] / (double)N;
          outIm[i] = out[i-numEndFftPointsToSwap][1] / (double)N;
       }

       fftw_free(in);
       fftw_free(out);
    }
   else
   {
       outRe.clear();
       outIm.clear();
   }
}

void realFFT(const dubVect& inRe, dubVect& outRe, double* windowCoef)
{
   fftw_complex *in, *out;
   fftw_plan p;

   unsigned int N = inRe.size();

   if(N > 0)
   {
       unsigned int halfN = N >> 1;

       in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);
       out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N);

       if(windowCoef == NULL)
       {
          for(unsigned int i = 0; i < N; ++i)
          {
             in[i][0] = inRe[i];
             in[i][1] = inRe[i];
          }
       }
       else
       {
          for(unsigned int i = 0; i < N; ++i)
          {
             in[i][0] = inRe[i] * windowCoef[i];
             in[i][1] = in[i][0]; // Set Imaginary value to the same as real.
          }
       }

       // Overwrite NaN samples at the beginning with 0's
       fixStartNanReal(in, N);

       p = fftw_plan_dft_1d(N, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

       fftw_execute(p);

       fftw_destroy_plan(p);

       outRe.resize(halfN);
       outRe[0] = fabs(out[0][0] / (double)N);
       for(unsigned int i = 1; i < halfN; ++i)
       {
          outRe[i] = (fabs(out[i][0]) + fabs(out[N-i][0])) / (double)N;
       }

       fftw_free(in);
       fftw_free(out);
   }
   else
   {
       outRe.clear();
   }
}

void getFFTXAxisValues_real(dubVect& xAxis, unsigned int numPoints, double& min, double& max, double sampleRate)
{
   if(numPoints > 0)
   {
      if(xAxis.size() != numPoints)
      {
         xAxis.resize(numPoints);
      }
      if(sampleRate == 0.0)
      {
         for(unsigned int i = 0; i < numPoints; ++i)
         {
            xAxis[i] = (double)i;
         }
      }
      else
      {
         // Real FFTs go from 0 to Fs/2.
         double hzPerBin = sampleRate / ((double)numPoints * (double)2.0);
         for(unsigned int i = 0; i < numPoints; ++i)
         {
            xAxis[i] = (double)i * hzPerBin;
         }

      }
      min = xAxis[0];
      max = xAxis[numPoints-1];
   }
   else
   {
       xAxis.clear();
       min = 0.0;
       max = 0.0;
   }

}

void getFFTXAxisValues_complex(dubVect& xAxis, unsigned int numPoints, double& min, double& max, double sampleRate)
{
    if(numPoints > 0)
    {
        // Complex FFTs go from -Fs/2 to Fs/2.
        int halfPoints = (int)(numPoints >> 1);
        int start = -halfPoints;

        if(xAxis.size() != numPoints)
        {
           xAxis.resize(numPoints);
        }

        if(sampleRate == 0.0)
        {
           for(int i = 0; i < (int)numPoints; ++i)
           {
              xAxis[i] = i + start;
           }
        }
        else
        {
           double hzPerBin = sampleRate / (double)numPoints;
           for(int i = 0; i < (int)numPoints; ++i)
           {
               xAxis[i] = (double)(i + start) * hzPerBin;
           }
        }
        min = xAxis[0];
        max = xAxis[numPoints-1];
    }
    else
    {
        xAxis.clear();
        min = 0.0;
        max = 0.0;
    }
}

void genWindowCoef(double* outSamp, unsigned int numSamp, bool scale)
{
   double linearScaleFactor = 1.0;
#if 1
   // Blackman-Harris Window.
   double denom = numSamp-1;
   double twoPi  =  6.2831853071795864769252867665590057683943387987502116419;
   double fourPi = 12.5663706143591729538505735331180115367886775975004232839;
   double sixPi  = 18.8495559215387594307758602996770173051830163962506349258;

   double cos1 = twoPi  / denom;
   double cos2 = fourPi / denom;
   double cos3 = sixPi  / denom;

   for(unsigned int i = 0; i < numSamp; ++i)
   {
      double sampIndex = i;
      outSamp[i] =   0.35875
                   - 0.48829 * cos(cos1*sampIndex)
                   + 0.14128 * cos(cos2*sampIndex)
                   - 0.01168 * cos(cos3*sampIndex);
   }
   linearScaleFactor = 1.969124795; // Measured value. TODO: Calculate this value based on window coef.
#else
   // Standard Blackman Window.
   for(unsigned int i = 0; i < numSamp; ++i)
   {
      outSamp[i] = ( 0.42 - 0.5 * cos (2.0*M_PI*(double)i/(double)(numSamp-1))
         + 0.08 * cos (4.0*M_PI*(double)i/(double)(numSamp-1)) );
   }
   linearScaleFactor = 1.812030468; // Measured value. TODO: Calculate this value based on window coef.
#endif

   if(scale)
   {
      for(unsigned int i = 0; i < numSamp; ++i)
      {
         outSamp[i] *= linearScaleFactor;
      }
   }
}
