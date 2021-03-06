# LightDrops
Raw image calibration and processing for astrophotography, astrophysics, and solar physics images. 

![alt tag](screenshots/screenshot_Solar_M74.jpg?raw=true)

##  Installation and dependencies

This project uses Qt as the base framework with Qt Creator and Qt designer. 
The main development language is C++, with a little bit of C. 
To compile the code, you will need to install CFITSIO, Libraw, openCV 3.x, Exiv2, and ArrayFire:

- [CFITSIO](http://heasarc.gsfc.nasa.gov/fitsio/fitsio.html): A library to handles FITS files, a scientific data file format, used here as an image file format. If developping from a Mac, the easiest installation is with macports. 
- [Libraw](http://www.libraw.org/docs/Install-LibRaw-eng.html): A library for handling raw files from DSLRs. 
At the moment Lightdrops is tested with raw images from Canon DSLRs, but other DSLR models will be supported in the future. Use DLLs for Windows or follow compilation instructions for Mac/Linux/Unix. 
- [OpenCV 3](http://opencv.org): Image processing library. Make sure TBB is enabled in your CMake compilation options. 
- [ArrayFire](http://arrayfire.com): A general purpose parallel processing library. Used here for the openCL functions that enables highly parallel and fast image processing with the hundreds of cores of the Graphics Processing Unit (GPU). If you are developping from a Mac, I recommend the [alternate installation with CMake](https://github.com/arrayfire/arrayfire/wiki/Build-Instructions-for-OSX#building-arrayfire). You only need the openCL backend. No need for GLEW of GLFW. 
- [Exiv2](http://www.exiv2.org): a library to read the metadata and exif data in DSLRs files. Currently, you need to compile a  version that is slightly changed for lightdrops, enabling the reading of temperature values in some camera sensors (see [feature 1219](http://dev.exiv2.org/issues/1219)). In the Exiv2 source files, replace the __src/canonmn.cpp__ and __src/canonmn_int.hpp__ with the ones that are provided in this Lightdrops repository in __exiv2_update/__.

### Quick installation for windows

Alternatively to building the dependencies, you can follow these steps to get started quickly on Windows:

- Install Visual Studio 2015
- Install QT 5.7 for VS 2015 x64
- Unzip the file Windows/windown-x64-dependenceis-except ArrayFire.7z in c:/dev
- Install ArrayFire OpenCL in c:/dev
- Build the project file using QtCreator
- Copy the dlls in c:/dev/dlls and paste them in the same folder as the LightDrops binary
- Run the program

Thses steps have been tested on Windows 10

![](screenshots/windows-screen1.png?raw=true)

## Hardware requirements

- Graphics that supports at least openGL 3.3. This is generally the case with Intel integrated graphics or any dedicated graphic card released since 2010. 
- Multicore processor. This software is tested with Intel dual core processors and quad core processors (core i3, i5, i7). 
Any intel or AMD CPU of the last decade should do, although things are not tested on AMD CPUs.  

## Get Started

After you run the app, the main UI appears; just drag and drop your 2D FITS or DSLR image(s) in the main window. 
You can drag multiple files at once and use the player for viewing through the image series. For FITS files, the "header" button will show the FITS header's full list of keywords/values/comments. 

## Notes on DSLRs
For DSLR files, for now only Canon raw .CR2 file are supported. We will support Nikon and Sony DSLRs in the near future. 
