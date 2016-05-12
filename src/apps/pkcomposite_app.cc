/**********************************************************************
pkcomposite_app.cc: program to mosaic and composite geo-referenced images
Copyright (C) 2008-2016 Pieter Kempeneers

This file is part of pktools

pktools is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

pktools is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with pktools.  If not, see <http://www.gnu.org/licenses/>.
***********************************************************************/
#include <algorithm>
#include <vector>
#include <iostream>
#include <string>
#include "imageclasses/ImgReaderGdal.h"
#include "imageclasses/ImgWriterGdal.h"
#include "imageclasses/ImgReaderOgr.h"
#include "base/Vector2d.h"
#include "base/Optionpk.h"
#include "algorithms/StatFactory.h"
#include "AppFactory.h"
/******************************************************************************/
/*! \page pkcomposite pkcomposite

 program to mosaic and composite geo-referenced images
## SYNOPSIS

<code>
  Usage: pkcomposite -i input [-i input]* -o output
</code>

<code>

  Options: [-b band]* [-dx xres] [-dy yres] [-e vector] [-ulx ULX -uly ULY -lrx LRX -lry LRY] [-cr rule] [-cb band] [-srcnodata value] [-bndnodata band] [-min value] [-max value] [-dstnodata value] [-r resampling_method] [-ot {Byte / Int16 / UInt16 / UInt32 / Int32 / Float32 / Float64 / CInt16 / CInt32 / CFloat32 / CFloat64}] [-of format] [-co NAME=VALUE]* [-a_srs epsg:number]

  Advanced options:
       [-file] [-w weight]* [-c class]* [-ct colortable] [-d description] [-align]
</code>

\section pkcomposite_description Description

The utility pkcomposite can be used to \em mosaic and \em composite multiple (georeferenced) raster datasets. A mosaic can merge images with different geographical extents into a single larger image. Compositing resolves the overlapping pixels according to some rule (e.g, the median of all overlapping pixels). This utility is complementary to GDAL, which currently does not support a composite step. Input datasets can have different bounding boxes and spatial resolutionsresolution.

\anchor pkcomposite_rules 
composite rule | composite output
------------- | -------------
overwrite | Overwrite overlapping pixels: the latter input image on the command line overrules the previous image
maxndvi | Create a maximum NDVI (normalized difference vegetation index) composite from multi-band input images. Use option -cb  to set the indexes of the red and near infrared bands respectively (e.g., -cb 0 -cb 1)
maxband | Select the pixel with a maximum value in the band specified by option -cb
minband | Select the pixel with a minimum value in the band specified by option -cb
mean | Calculate the mean (average) of overlapping pixels
stdev | Calculate the standard deviation of overlapping pixels
median | Calculate the median of overlapping pixels
mode | Select the mode of overlapping pixels (maximum voting): use for Byte images only
sum | Calculate the arithmetic sum of overlapping pixels
maxallbands | For each individual band, assign the maximum value found in all overlapping pixels. Unlike maxband, output band values cannot be attributed to a single (date) pixel in the input time series
minallbands | For each individual band, assign the minimum value found in all overlapping pixels. Unlike minband, output band values cannot be attributed to a single (date) pixel in the input time series

Example: Calculate the maximum NDVI composite of two multispectral input images (e.g., red is band 0 and near infrared is band 1)

\code
pkcomposite -i input1.tif -i input2.tif -o output.tif -cr maxndvi -cb 0 -cb 1
\endcode

Example: Calculate the minimum nadir composite of two input images, where the forth band (b=3) contains the view zenith angle

\code
pkcomposite -i input1.tif -i input2.tif -o minzenith.tif -cr minband -cb 3
\endcode

Example: Calculate the minimum of two input images in all bands

\code
pkcomposite -i input1.tif -i input2.tif -o minimum.tif -cr minallbands
\endcode


\section pkcomposite_options Options
 - use either `-short` or `--long` options (both `--long=value` and `--long value` are supported)
 - short option `-h` shows basic options only, long option `--help` shows all options

|short|long|type|default|description|
|-----|----|----|-------|-----------|
 | i      | input                | std::string |       |Input image file(s). If input contains multiple images, a multi-band output is created | 
 | o      | output               | std::string |       |Output image file | 
 | b      | band                 | int  |       |band index(es) to crop (leave empty if all bands must be retained) | 
 | dx     | dx                   | double |       |Output resolution in x (in meter) (empty: keep original resolution) | 
 | dy     | dy                   | double |       |Output resolution in y (in meter) (empty: keep original resolution) | 
 | e      | extent               | std::string |       |get boundary from extent from polygons in vector file | 
 | cut      | crop_to_cutline    | bool | false |Crop the extent of the target dataset to the extent of the cutline | 
 | eo       | eo                 | std::string |       |special extent options controlling rasterization: ATTRIBUTE|CHUNKYSIZE|ALL_TOUCHED|BURN_VALUE_FROM|MERGE_ALG, e.g., -eo ATTRIBUTE=fieldname |
 | m      | mask                 | std::string |       |Use the first band of the specified file as a validity mask (0 is nodata) | 
 | msknodata | msknodata            | float | 0     |Mask value not to consider for composite
 | mskband | mskband              | short | 0     |Mask band to read (0 indexed) | 
 | ulx    | ulx                  | double | 0     |Upper left x value bounding box | 
 | uly    | uly                  | double | 0     |Upper left y value bounding box | 
 | lrx    | lrx                  | double | 0     |Lower right x value bounding box | 
 | lry    | lry                  | double | 0     |Lower right y value bounding box | 
 | cr     | crule                | std::string | overwrite |Composite rule (overwrite, maxndvi, maxband, minband, mean, mode (only for byte images), median, sum, maxallbands, minallbands, stdev | 
 | cb     | cband                | int  | 0     |band index used for the composite rule (e.g., for ndvi, use --cband=0 --cband=1 with 0 and 1 indices for red and nir band respectively | 
 | srcnodata | srcnodata            | double |       |invalid value(s) for input raster dataset | 
 | bndnodata | bndnodata            | int  | 0     |Band(s) in input image to check if pixel is valid (used for srcnodata, min and max options) | 
 | min    | min                  | double |       |flag values smaller or equal to this value as invalid. | 
 | max    | max                  | double |       |flag values larger or equal to this value as invalid. | 
 | dstnodata | dstnodata            | double | 0     |nodata value to put in output raster dataset if not valid or out of bounds. | 
 | r      | resampling-method    | std::string | near  |Resampling method (near: nearest neighbor, bilinear: bi-linear interpolation). | 
 | ot     | otype                | std::string |       |Data type for output image ({Byte/Int16/UInt16/UInt32/Int32/Float32/Float64/CInt16/CInt32/CFloat32/CFloat64}). Empty string: inherit type from input image | 
 | of     | oformat              | std::string | GTiff |Output image format (see also gdal_translate).| 
 | co     | co                   | std::string |       |Creation option for output file. Multiple options can be specified. | 
 | a_srs  | a_srs                | std::string |       |Override the spatial reference for the output file (leave blank to copy from input file, use epsg:3035 to use European projection and force to European grid | 
 | file   | file                 | short | 0     |write number of observations (1) or sequence nr of selected file (2) for each pixels as additional layer in composite | 
 | w      | weight               | short | 1     |Weights (type: short) for the composite, use one weight for each input file in same order as input files are provided). Use value 1 for equal weights. | 
 | c      | class                | short | 0     |classes for multi-band output image: each band represents the number of observations for one specific class. Use value 0 for no multi-band output image. | 
 | ct     | ct                   | std::string |       |color table file with 5 columns: id R G B ALFA (0: transparent, 255: solid) | 
 | align  | align                | bool  |       |Align output bounding box to first input image | 
 | scale  | scale                | double |       |output=scale*input+offset | 
 | off    | offset               | double |       |output=scale*input+offset | 
 | mem    | mem                  | unsigned long int | 1000 |Buffer size (in MB) to read image data blocks in memory | 
 | d      | description          | std::string |       |Set image description | 

Examples
========
Some examples how to use pkcomposite can be found \ref examples_pkcomposite "here"

FAQ
===
Frequently asked questions on pkcomposite can be found \ref faq_pkcomposite "here
**/

namespace crule{
  enum CRULE_TYPE {overwrite=0, maxndvi=1, maxband=2, minband=3, validband=4, mean=5, mode=6, median=7,sum=8,minallbands=9,maxallbands=10,stdev=11};
}

using namespace std;

int main(int argc, char *argv[])
{
  Optionpk<string>  input_opt("i", "input", "Input image file(s). If input contains multiple images, a multi-band output is created");
  Optionpk<string>  output_opt("o", "output", "Output image file");
  Optionpk<int>  band_opt("b", "band", "band index(es) to crop (leave empty if all bands must be retained)");
  Optionpk<double>  dx_opt("dx", "dx", "Output resolution in x (in meter) (empty: keep original resolution)");
  Optionpk<double>  dy_opt("dy", "dy", "Output resolution in y (in meter) (empty: keep original resolution)");
  Optionpk<string>  extent_opt("e", "extent", "get boundary from extent from polygons in vector file");
  Optionpk<bool> cut_opt("cut", "crop_to_cutline", "Crop the extent of the target dataset to the extent of the cutline.",false);
  Optionpk<string> eoption_opt("eo","eo", "special extent options controlling rasterization: ATTRIBUTE|CHUNKYSIZE|ALL_TOUCHED|BURN_VALUE_FROM|MERGE_ALG, e.g., -eo ATTRIBUTE=fieldname");
  Optionpk<string> mask_opt("m", "mask", "Use the first band of the specified file as a validity mask (0 is nodata).");
  Optionpk<float> msknodata_opt("msknodata", "msknodata", "Mask value not to consider for composite.", 0);
  Optionpk<short> mskband_opt("mskband", "mskband", "Mask band to read (0 indexed)", 0);
  Optionpk<double>  ulx_opt("ulx", "ulx", "Upper left x value bounding box", 0.0);
  Optionpk<double>  uly_opt("uly", "uly", "Upper left y value bounding box", 0.0);
  Optionpk<double>  lrx_opt("lrx", "lrx", "Lower right x value bounding box", 0.0);
  Optionpk<double>  lry_opt("lry", "lry", "Lower right y value bounding box", 0.0);
  Optionpk<string> crule_opt("cr", "crule", "Composite rule (overwrite, maxndvi, maxband, minband, mean, mode (only for byte images), median, sum, maxallbands, minallbands, stdev", "overwrite");
  Optionpk<int> ruleBand_opt("cb", "cband", "band index used for the composite rule (e.g., for ndvi, use --cband=0 --cband=1 with 0 and 1 indices for red and nir band respectively", 0);
  Optionpk<double> srcnodata_opt("srcnodata", "srcnodata", "invalid value(s) for input raster dataset");
  Optionpk<int> bndnodata_opt("bndnodata", "bndnodata", "Band(s) in input image to check if pixel is valid (used for srcnodata, min and max options)", 0);
  Optionpk<double> minValue_opt("min", "min", "flag values smaller or equal to this value as invalid.");
  Optionpk<double> maxValue_opt("max", "max", "flag values larger or equal to this value as invalid.");
  Optionpk<double>  dstnodata_opt("dstnodata", "dstnodata", "nodata value to put in output raster dataset if not valid or out of bounds.", 0);
  Optionpk<string>  resample_opt("r", "resampling-method", "Resampling method (near: nearest neighbor, bilinear: bi-linear interpolation).", "near");
  Optionpk<string>  otype_opt("ot", "otype", "Data type for output image ({Byte/Int16/UInt16/UInt32/Int32/Float32/Float64/CInt16/CInt32/CFloat32/CFloat64}). Empty string: inherit type from input image", "");
  Optionpk<string>  oformat_opt("of", "oformat", "Output image format (see also gdal_translate).","GTiff");
  Optionpk<string> option_opt("co", "co", "Creation option for output file. Multiple options can be specified.");
  Optionpk<string>  projection_opt("a_srs", "a_srs", "Override the spatial reference for the output file (leave blank to copy from input file, use epsg:3035 to use European projection and force to European grid");
  Optionpk<short> file_opt("file", "file", "write number of observations (1) or sequence nr of selected file (2) for each pixels as additional layer in composite", 0);
  Optionpk<short> weight_opt("w", "weight", "Weights (type: short) for the composite, use one weight for each input file in same order as input files are provided). Use value 1 for equal weights.", 1);
  Optionpk<short> class_opt("c", "class", "classes for multi-band output image: each band represents the number of observations for one specific class. Use value 0 for no multi-band output image.", 0);
  Optionpk<string>  colorTable_opt("ct", "ct", "color table file with 5 columns: id R G B ALFA (0: transparent, 255: solid)");
  Optionpk<string>  description_opt("d", "description", "Set image description");
  Optionpk<bool>  align_opt("align", "align", "Align output bounding box to input image",false);
  Optionpk<double> scale_opt("scale", "scale", "output=scale*input+offset");
  Optionpk<double> offset_opt("offset", "offset", "output=scale*input+offset");
  Optionpk<unsigned long int>  memory_opt("mem", "mem", "Buffer size (in MB) to read image data blocks in memory",1000,1);
  Optionpk<short>  verbose_opt("v", "verbose", "verbose", 0,2);

  extent_opt.setHide(1);
  cut_opt.setHide(1);
  eoption_opt.setHide(1);
  mask_opt.setHide(1);
  msknodata_opt.setHide(1);
  mskband_opt.setHide(1);
  option_opt.setHide(1);
  file_opt.setHide(1);
  weight_opt.setHide(1);
  class_opt.setHide(1);
  colorTable_opt.setHide(1);
  description_opt.setHide(1);
  scale_opt.setHide(1);
  offset_opt.setHide(1);
  memory_opt.setHide(1);

  bool doProcess;//stop process when program was invoked with help option (-h --help)
  try{
    doProcess=input_opt.retrieveOption(argc,argv);
    output_opt.retrieveOption(argc,argv);
    band_opt.retrieveOption(argc,argv);
    dx_opt.retrieveOption(argc,argv);
    dy_opt.retrieveOption(argc,argv);
    extent_opt.retrieveOption(argc,argv);
    cut_opt.retrieveOption(argc,argv);
    eoption_opt.retrieveOption(argc,argv);
    mask_opt.retrieveOption(argc,argv);
    msknodata_opt.retrieveOption(argc,argv);
    mskband_opt.retrieveOption(argc,argv);
    ulx_opt.retrieveOption(argc,argv);
    uly_opt.retrieveOption(argc,argv);
    lrx_opt.retrieveOption(argc,argv);
    lry_opt.retrieveOption(argc,argv);
    crule_opt.retrieveOption(argc,argv);
    ruleBand_opt.retrieveOption(argc,argv);
    srcnodata_opt.retrieveOption(argc,argv);
    bndnodata_opt.retrieveOption(argc,argv);
    minValue_opt.retrieveOption(argc,argv);
    maxValue_opt.retrieveOption(argc,argv);
    dstnodata_opt.retrieveOption(argc,argv);
    resample_opt.retrieveOption(argc,argv);
    otype_opt.retrieveOption(argc,argv);
    oformat_opt.retrieveOption(argc,argv);
    option_opt.retrieveOption(argc,argv);
    projection_opt.retrieveOption(argc,argv);
    file_opt.retrieveOption(argc,argv);
    weight_opt.retrieveOption(argc,argv);
    class_opt.retrieveOption(argc,argv);
    colorTable_opt.retrieveOption(argc,argv);
    description_opt.retrieveOption(argc,argv);
    align_opt.retrieveOption(argc,argv);
    scale_opt.retrieveOption(argc,argv);
    offset_opt.retrieveOption(argc,argv);
    memory_opt.retrieveOption(argc,argv);
    verbose_opt.retrieveOption(argc,argv);
  }
  catch(string predefinedString){
    std::cout << predefinedString << std::endl;
    exit(0);
  }
  if(!doProcess){
    cout << endl;
    cout << "Usage: pkcomposite -i input [-i input]* -o output" << endl;
    cout << endl;
    std::cout << "short option -h shows basic options only, use long option --help to show all options" << std::endl;
    exit(0);//help was invoked, stop processing
  }

  //todo: set all the arguments in app
  appfactory::AppFactory app(memory_opt[0]);
  app.setOptions(argc,argv);
  vector<ImgReaderGdal> imgReader(input_opt.size());
  for(int ifile=0;ifile<input_opt.size();++ifile){
    if(verbose_opt[0])
      cout << "opening file " << input_opt[ifile] << endl;
    try{
      imgReader[ifile].open(input_opt[ifile],GA_ReadOnly,memory_opt[0]);
      // imgReader[ifile].open(input_opt[ifile],GA_ReadOnly);
      for(int iband=0;iband<scale_opt.size();++iband)
        imgReader[ifile].setScale(scale_opt[iband],iband);
      for(int iband=0;iband<offset_opt.size();++iband)
        imgReader[ifile].setOffset(offset_opt[iband],iband);
    }
    catch(string errorstring){
      cerr << errorstring << " " << input_opt[ifile] << endl;
    }
  }
  //test
  cout << "constructor imgWriter" << endl;
  ImgWriterGdal imgWriter;//AppFactory has been instantiated with memory_opt so we can set mem in ImgWriter 
  //test
  cout << "calling app.pkcomposite" << endl;
  app.pkcomposite(imgReader,imgWriter);
  //test
  cout << "after app.pkcomposite" << endl;
  for(int ifile=0;ifile<imgReader.size();++ifile){
    imgReader[ifile].close();
  }
  imgWriter.setFile(output_opt[0],oformat_opt[0]);

  for(int iband=0;iband<imgWriter.nrOfBand();++iband)
    imgWriter.GDALSetNoDataValue(dstnodata_opt[0],iband);

  imgWriter.close();
  return 0;
}
  