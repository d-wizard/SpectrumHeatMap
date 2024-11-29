import os
import argparse
from datetime import datetime

################################################################################

def getAllFiles(startDir, numSubDirToReturn=-1):
   retVal = []
   if numSubDirToReturn < 0:
      for dirname, dirnames, filenames in os.walk(startDir):
         for filename in filenames:
            retVal.append(os.path.join(dirname, filename))
   else:
      for dirname, dirnames, filenames in os.walk(startDir):
         #determine the number of sub directories
         subDirFull = dirname[len(startDir):]
         numSubDir = string.count(subDirFull, os.sep)

         if numSubDir < numSubDirToReturn:
            for filename in filenames:
               retVal.append(os.path.join(dirname, filename))
         elif numSubDir == numSubDirToReturn:
            if dirname != '' and dirname[-1] == os.sep:
               dirname = dirname[:-1]
            
            if (dirname in retVal) == False:
               retVal.append(dirname)

            if len(dirnames) > 0:
               del dirnames[:]
         else:
            splittedAddPath = string.split(subDirFull, os.sep)[:numSubDirToReturn+1]
            if splittedAddPath[0] == '':
               splittedAddPath = splittedAddPath[1:]

            tempAddDir = os.path.join(startDir, string.join(splittedAddPath, os.sep))
            if tempAddDir != '' and tempAddDir[-1] == os.sep:
               tempAddDir = tempAddDir[:-1]
            
            if (tempAddDir in retVal) == False:
               retVal.append(tempAddDir)

            if len(dirnames) > 0:
               del dirnames[:]
               
   return retVal

################################################################################

def getUniqueFileNameTimeStr():
   return datetime.now().strftime("%y%m%d%H%M%S")

################################################################################

def main():
   parser = argparse.ArgumentParser()
   parser.add_argument("-a", "--app_path", help="Path to the Spectrum Heat Map app. If unspecified, the app will be looked for in common locations.")
   parser.add_argument("-i", "--input", required=True, help="File or directory to parse. If a directory all files in the directory and subdirectory will be parsed.")
   parser.add_argument("-o", "--output", help="Output file or directory to write the PNG files to. If not specified, the files will be written to the input folder.")
   parser.add_argument("-s", "--samp_rate", type=float, required=True, help="Sample rate of the data.")
   parser.add_argument("-f", "--fft_size", type=int, required=True, help="FFT size.")
   parser.add_argument("-t", "--time", type=float, required=True, help="Time between FFTs.")
   parser.add_argument("-y", "--format", required=True, help="Input Format (float, double, int16_t, etc).")
   parser.add_argument("-j", "--num_threads", type=int, help="Number of threads to uses.")
   parser.add_argument("-n", "--normalize", action='store_true', help="Use this to normalize max to the detected peak value.")
   parser.add_argument("-m", "--max_db", type=float, help="Max FFT bin value in dB.")
   parser.add_argument("-r", "--range_db", type=float, help="Range of the Heat Map in dB.")
   parser.add_argument("-M", "--max_ffts", type=int, help="If specified, the output will be split into multiple files.")
   args = parser.parse_args()

   # Get a unique time based str that can be used
   uniqueStr = getUniqueFileNameTimeStr()

   # Determine the application path
   app_path = None # init to invalid
   if args.app_path != None and os.path.isfile(args.app_path):
      app_path = args.app_path
   else:
      tryPaths = ['./.build/apps/FileToHeatMap', './build/apps/FileToHeatMap'] # Common build locations
      for tryPath in tryPaths:
         if os.path.isfile(tryPath):
            app_path = tryPath
            break
      if app_path == None:
         app_path = 'FileToHeatMap' # Maybe it is in a directory specifed in the PATH environment variable

   # Determine files to parse
   filesToParse = []
   if os.path.isfile(args.input):
      filesToParse.append(args.input)
   elif os.path.isdir(args.input):
      filesToParse = getAllFiles(args.input)
   if len(filesToParse) < 1:
      print("No files to parse, invalid input?")
      return
   
   # Define fixed command line args
   fixedArgs = ''
   fixedArgs += (' -s ' + str(args.samp_rate))
   fixedArgs += (' -f ' + str(args.fft_size))
   fixedArgs += (' -t ' + str(args.time))
   fixedArgs += (' -y ' + str(args.format))
   if args.num_threads != None:
      fixedArgs += (' -j ' + str(args.num_threads))
   if args.normalize == True:
      fixedArgs += (' -n')
   elif args.max_db != None: # Only can have normalize or max_db, not both
      fixedArgs += (' -m ' + str(args.max_db))
   if args.range_db != None:
      fixedArgs += (' -r ' + str(args.range_db))

   # Deterine max input size
   maxInputSize = 0 # init to invalid.
   if args.max_ffts != None:
      inTypeSize = 1 # Define to 1 byte per value
      if '64' in args.format or 'double' == args.format.lower():
         inTypeSize = 8
      elif '32' in args.format or 'float' == args.format.lower():
         inTypeSize = 4
      elif '16' in args.format:
         inTypeSize = 2
      fftSizeBytes = inTypeSize * args.fft_size * 2 # 2x because the app assumes complex samples
      maxInputSize = fftSizeBytes * args.max_ffts

   # Figure out base directory to store output files.
   outBaseDir = None
   outFileName = None # Only used if the input is a single file 
   if args.output != None: # Did the user specify an output location
      if os.path.isdir(args.output):
         outBaseDir = args.output
      else:
         # Check if up-a-directory exists. If so, this might be a new file or folder.
         try:
            upDir, splitDir = os.path.split(args.output)
            if os.path.isdir(upDir):
               outBaseDir = upDir
         except:
            pass
         if outBaseDir == None:
            print("Failed to find valid directory from command line args.")
            return
         elif os.path.isfile(args.input):
            # Single file input. This is the file name.
            outFileName = splitDir
         else:
            # Folder input. This is the name of a new folder.
            os.makedirs(args.output)
   else:
      # User didn't specify an output directory, it will be relative to the input file(s)
      pass # Nothing to do

   # Loop over the input files.
   for inFile in filesToParse:
      # Determine output directory
      outDir = os.path.dirname(inFile) if outBaseDir == None else outBaseDir
      if maxInputSize > 0:
         # Splitting into multiple files. Make a subdirectory with the input name.
         outDir = os.path.join(outDir, os.path.split(inFile)[1] + "_" + uniqueStr)
         os.makedirs(outDir)

         # Loop until no more bytes are left
         totalFileSize = os.path.getsize(inFile)
         inputOffset = 0
         outIndex = 0
         while inputOffset < totalFileSize:
            chunkSize = maxInputSize if (inputOffset + maxInputSize) <= totalFileSize else totalFileSize - inputOffset

            outFileName = os.path.split(inFile)[1] + "_" + str(outIndex) + '.png'

            cmdLine = app_path + ' -i ' + inFile + ' -o ' + os.path.join(outDir, outFileName) + fixedArgs + ' -S ' + str(inputOffset) + ' -E ' + str(inputOffset+chunkSize)
            print(cmdLine)
            inputOffset += chunkSize
            outIndex += 1

      else:
         # Single file. Not making a new directory here.
         pass


################################################################################
################################################################################
################################################################################

# Main start
if __name__== "__main__":
   main()