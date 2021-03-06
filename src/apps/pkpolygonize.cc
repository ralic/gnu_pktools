/**********************************************************************
pkpolygonize.cc: program to make vector file from raster image
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
#include "cpl_string.h"
#include "gdal_priv.h"
#include "gdal.h"
#include "imageclasses/ImgRasterGdal.h"
#include "imageclasses/ImgWriterOgr.h"
#include "base/Optionpk.h"
#include "ogrsf_frmts.h"
extern "C" {
#include "gdal_alg.h"
#include "ogr_api.h"
}

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/******************************************************************************/
/*! \page pkpolygonize pkpolygonize
 program to make vector file from raster image
## SYNOPSIS

<code>
  Usage: pkpolygonize -i input [-m mask] -o output
</code>

<code>
  
  Options: [-f format] [-b band] [-n fieldname] [-nodata value]

</code>

\section pkpolygonize_description Description

The utility pkpolygonize converts a raster to a vector dataset. All pixels in the mask band with a value other than zero will be considered suitable for collection as polygons. Use the same input file as mask to remove the background polygon (recommended).
\section pkpolygonize_options Options
 - use either `-short` or `--long` options (both `--long=value` and `--long value` are supported)
 - short option `-h` shows basic options only, long option `--help` shows all options
|short|long|type|default|description|
|-----|----|----|-------|-----------|
 | i      | input                | std::string |       |Input image file | 
 | m      | mask                 | std::string |       |All pixels in the mask band with a value other than zero will be considered suitable for collection as polygons. Use input file as mask to remove background polygon!  | 
 | o      | output               | std::string |       |Output vector file | 
 | f      | f                    | std::string | SQLite |Output OGR file format | 
 | b      | band                 | int  | 0     |the band to be used from input file | 
 | nodata | nodata               | double |       |Disgard this nodata value when creating polygons. | 
 | n      | name                 | std::string | DN    |the field name of the output layer | 
  | 
Usage: pkpolygonize -i input [-m mask] -o output


Examples
========
Some examples how to use pkpolygonize can be found \ref examples_pkpolygonize "here"
**/

using namespace std;

int main(int argc,char **argv) {
  Optionpk<string> input_opt("i", "input", "Input image file");
  Optionpk<string> mask_opt("m", "mask", "All pixels in the mask band with a value other than zero will be considered suitable for collection as polygons. Use input file as mask to remove background polygon! ");
  Optionpk<string> output_opt("o", "output", "Output vector file");
  Optionpk<string> ogrformat_opt("f", "f", "Output OGR file format","SQLite");
  Optionpk<int> band_opt("b", "band", "the band to be used from input file", 0);
  Optionpk<string> fname_opt("n", "name", "the field name of the output layer", "DN");
  Optionpk<double> nodata_opt("nodata", "nodata", "Disgard this nodata value when creating polygons.");
  Optionpk<short> verbose_opt("v", "verbose", "verbose mode if > 0", 0,2);

  bool doProcess;//stop process when program was invoked with help option (-h --help)
  try{
    doProcess=input_opt.retrieveOption(argc,argv);
    mask_opt.retrieveOption(argc,argv);
    output_opt.retrieveOption(argc,argv);
    ogrformat_opt.retrieveOption(argc,argv);
    band_opt.retrieveOption(argc,argv);
    nodata_opt.retrieveOption(argc,argv);
    fname_opt.retrieveOption(argc,argv);
    verbose_opt.retrieveOption(argc,argv);
  }
  catch(string predefinedString){
    std::cout << predefinedString << std::endl;
    exit(0);
  }
  if(!doProcess){
    cout << endl;
    cout << "Usage: pkpolygonize -i input [-m mask] -o output" << endl;
    cout << endl;
    std::cout << "short option -h shows basic options only, use long option --help to show all options" << std::endl;
    exit(0);//help was invoked, stop processing
  }
  if(input_opt.empty()){
    std::cerr << "No input file provided (use option -i). Use --help for help information";
      exit(0);
  }
  if(output_opt.empty()){
    std::cerr << "No output file provided (use option -o). Use --help for help information";
      exit(0);
  }

  GDALAllRegister();

  double dfComplete=0.0;
  const char* pszMessage;
  void* pProgressArg=NULL;
  GDALProgressFunc pfnProgress=GDALTermProgress;
  pfnProgress(dfComplete,pszMessage,pProgressArg);
  

  ImgRasterGdal maskReader;
  GDALRasterBand *maskBand=NULL;
  if(mask_opt.size()){
    if(verbose_opt[0])
      cout << "opening mask file " << mask_opt[0] << endl;
    maskReader.open(mask_opt[0]);
    maskBand = maskReader.getRasterBand(0);
  }

  ImgRasterGdal inputReader(input_opt[0]);
  GDALRasterBand  *inputBand;
  inputBand=inputReader.getRasterBand(0);
  if(nodata_opt.size())
    inputBand->SetNoDataValue(nodata_opt[0]);
  ImgWriterOgr ogrWriter(output_opt[0],ogrformat_opt[0]);
  OGRLayer* theLayer=ogrWriter.createLayer(output_opt[0].substr(output_opt[0].rfind('/')+1), inputReader.getProjectionRef());
  if(verbose_opt[0])
    cout << "projection: " << inputReader.getProjection() << endl;
  ogrWriter.createField(fname_opt[0],OFTInteger);

  OGRLayerH hOutLayer=(OGRLayerH)ogrWriter.getLayer();
  if(verbose_opt[0])
    cout << "GDALPolygonize started..." << endl;

  int index=theLayer->GetLayerDefn()->GetFieldIndex(fname_opt[0].c_str());
  if(GDALPolygonize((GDALRasterBandH)inputBand, (GDALRasterBandH)maskBand, hOutLayer,index,NULL,pfnProgress,pProgressArg)!=CE_None)
    cerr << CPLGetLastErrorMsg() << endl;
  else{
    dfComplete=1.0;
    pfnProgress(dfComplete,pszMessage,pProgressArg);
  }
  cout << "number of features: " << OGR_L_GetFeatureCount(hOutLayer,TRUE) << endl;
  
  inputReader.close();
  if(mask_opt.size())
    maskReader.close();
  ogrWriter.close();
}

