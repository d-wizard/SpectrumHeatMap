/* Copyright 2013 - 2019, 2021 - 2022, 2024 Dan Williams. All Rights Reserved.
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
#ifndef HelperTypes_h
#define HelperTypes_h

#include <vector>

#include <limits>  // std::numeric_limits
#include <cmath>   // std::isfinite

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

typedef std::vector<double> dubVect;

//////////////////////////////////////////////
/////////////// Type Helpers /////////////////
//////////////////////////////////////////////
#if 1
#define isDoubleValid std::isfinite
#else
// Alternate version. Keeping it around just incase std::isfinite isn't exactly the same.
inline bool isDoubleValid(double value)
{
   // According to the IEEE standard, NaN values have the odd property that
   // comparisons involving them are always false. That is, for a float
   // f, f != f will be true only if f is NaN
   if (value != value)
   {
      return false;
   }
   else if (value > std::numeric_limits<double>::max())
   {
      return false;
   }
   else if (value < -std::numeric_limits<double>::max())
   {
      return false;
   }
   else
   {
      return true;
   }
}
#endif




#endif

