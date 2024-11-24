# Spectrum Heat Map

This will take in files that contain IQ samples and generate Heat Map Spectrum images.

## Setup
These instructions are for Linux builds. FFTW is being used and it does things differently for Windows. It shouldn't be too hard to get this working for Windows builds.

* Get submodules
```
cd SpectrumHeatMap
git submodule update --init --recursive
```

* Get FFTW - The source code is hosted on their website in a tarball.
```
cd SpectrumHeatMap
wget https://www.fftw.org/fftw-3.3.10.tar.gz
tar -xzf fftw-3.3.10.tar.gz
rm fftw-3.3.10.tar.gz
```

## Build
```
cd SpectrumHeatMap
cmake -S . -B .build
cmake --build .build
```
