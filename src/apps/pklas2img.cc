/**********************************************************************
pklas2img.cc: Rasterize LAS/LAZ point clouds with filtering/compositing options
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
#include <iostream>
#include "base/Optionpk.h"
#include "imageclasses/ImgRasterGdal.h"
#include "imageclasses/ImgReaderOgr.h"
#include "lasclasses/FileReaderLas.h"
#include "algorithms/StatFactory.h"
#include "algorithms/Filter2d.h"

/******************************************************************************/
/*! \page pklas2img pklas2img
 Rasterize LAS/LAZ point clouds with filtering/compositing options
## SYNOPSIS

<code>
  Usage: pklas2img -i input.las -o output.tif
</code>

<code>
  
  Options: [-n attribute] [-comp method] [-fir type] [-a_srs] [-ulx value -uly value -lrx value -lry value] [-dx value -dy value] [-ot type] [-of format] [-ret value]* [-class number]* 

  Advanced options: [-nbin value] [-nodata value] [-co option]* [-ct colortable] 

</code>

\section pklas2img_description Description

The utility pklas2img converts a las/laz point cloud into a gridded raster dataset. The implementation is based on <a href="http://www.liblas.org">liblas</a> API. You can define the bounding box, grid cell size and spatial reference set. The composite rule for multiple returns within a single grid cell can be set with the option -comp. The default attribute is z (heiht), but can also be intensity (if available), scan angle rank (-n angle), the return number (-n return) or the total number of returns in that grid cell (-n nreturn). To select specific returns only, set the option -fir (first, last, single, multiple, or all).
\section pklas2img_options Options
 - use either `-short` or `--long` options (both `--long=value` and `--long value` are supported)
 - short option `-h` shows basic options only, long option `--help` shows all options
|short|long|type|default|description|
|-----|----|----|-------|-----------|
 | i      | input                | std::string |       |Input las file | 
 | n      | name                 | std::string | z     |names of the attribute to select: intensity, angle, return, nreturn, spacing, z | 
 | ret    | ret                  | unsigned short |       |number(s) of returns to include | 
 | class  | class                | unsigned short |       |classes to keep: 0 (created, never classified), 1 (unclassified), 2 (ground), 3 (low vegetation), 4 (medium vegetation), 5 (high vegetation), 6 (building), 7 (low point, noise), 8 (model key-point), 9 (water), 10 (reserved), 11 (reserved), 12 (overlap) | 
 | comp   | comp                 | std::string | last  |composite for multiple points in cell (min, max, absmin, absmax, median, mean, var, stdev, sum, first, last, profile (percentile height values), percentile, number (point density)). Last: overwrite cells with latest point | 
 | fir    | filter               | std::string | all   |filter las points (first,last,single,multiple,all). | 
 | angle_min  | angle_min        | unsigned short |    |minimum scan angle to read points. | 
 | angle_max  | angle_max        | unsigned short |    |maximum scan angle to read points. | 
 | o      | output               | std::string |       |Output image file | 
 | a_srs  | a_srs                | std::string |       |assign the projection for the output file in epsg code, e.g., epsg:3035 for European LAEA projection | 
 | ulx    | ulx                  | double | 0     |Upper left x value bounding box (in geocoordinates if georef is true). 0 is read from input file | 
 | uly    | uly                  | double | 0     |Upper left y value bounding box (in geocoordinates if georef is true). 0 is read from input file | 
 | lrx    | lrx                  | double | 0     |Lower right x value bounding box (in geocoordinates if georef is true). 0 is read from input file | 
 | lry    | lry                  | double | 0     |Lower right y value bounding box (in geocoordinates if georef is true). 0 is read from input file | 
 | ot     | otype                | std::string | Byte  |Data type for output image ({Byte/Int16/UInt16/UInt32/Int32/Float32/Float64/CInt16/CInt32/CFloat32/CFloat64}). Empty string: inherit type from input image | 
 | of     | oformat              | std::string | GTiff |Output image format (see also gdal_translate).| 
 | dx     | dx                   | double | 1     |Output resolution in x (in meter) | 
 | dy     | dy                   | double | 1     |Output resolution in y (in meter) | 
 | nbin   | nbin                 | short | 10    |Number of percentile bins for calculating percentile height value profile (=number of output bands) | 
 | perc   | perc                 | double | 95    |Percentile value used for rule percentile | 
 | nodata | nodata               | short | 0     |nodata value to put in image | 
 | co     | co                   | std::string |       |Creation option for output file. Multiple options can be specified. | 
 | ct     | ct                   | std::string |       |color table (file with 5 columns: id R G B ALFA (0: transparent, 255: solid) | 

pklas2img -i lasfile -o output


**/

using namespace std;

int main(int argc,char **argv) {
  Optionpk<string> input_opt("i", "input", "Input las file");
  Optionpk<string> attribute_opt("n", "name", "names of the point attribute to select: intensity, angle, return, nreturn, spacing, z", "z");
  // Optionpk<bool> disc_opt("circ", "circular", "circular disc kernel for dilation and erosion", false);
  // Optionpk<double> maxSlope_opt("s", "maxSlope", "Maximum slope used for morphological filtering", 0.0);
  // Optionpk<double> hThreshold_opt("ht", "maxHeight", "initial and maximum height threshold for progressive morphological filtering (e.g., -ht 0.2 -ht 2.5)", 0.2);
  // Optionpk<short> maxIter_opt("maxit", "maxit", "Maximum number of iterations in post filter", 5);
  Optionpk<unsigned short> returns_opt("ret", "ret", "number(s) of returns to include");
  Optionpk<unsigned short> classes_opt("class", "class", "classes to keep: 0 (created, never classified), 1 (unclassified), 2 (ground), 3 (low vegetation), 4 (medium vegetation), 5 (high vegetation), 6 (building), 7 (low point, noise), 8 (model key-point), 9 (water), 10 (reserved), 11 (reserved), 12 (overlap)");
  Optionpk<string> composite_opt("comp", "comp", "composite for multiple points in cell (min, max, absmin, absmax, median, mean, var, stdev, sum, first, last, profile (percentile height values), percentile, number (point density)). Last: overwrite cells with latest point", "last");
  Optionpk<string> filter_opt("fir", "filter", "filter las points (first,last,single,multiple,all).", "all");
  Optionpk<short> angle_min_opt("angle_min", "angle_min", "Minimum scan angle to read points.");
  Optionpk<short> angle_max_opt("angle_max", "angle_max", "Maximum scan angle to read points.");
  // Optionpk<string> postFilter_opt("pf", "pfilter", "post processing filter (etew_min,promorph (progressive morphological filter),bunting (adapted promorph),open,close,none).", "none");
  // Optionpk<short> dimx_opt("dimx", "dimx", "Dimension X of postFilter", 3);
  // Optionpk<short> dimy_opt("dimy", "dimy", "Dimension Y of postFilter", 3);
  Optionpk<string> output_opt("o", "output", "Output image file");
  Optionpk<string> projection_opt("a_srs", "a_srs", "assign the projection for the output file in epsg code, e.g., epsg:3035 for European LAEA projection");
  Optionpk<double> ulx_opt("ulx", "ulx", "Upper left x value bounding box (in geocoordinates if georef is true). 0 is read from input file", 0.0);
  Optionpk<double> uly_opt("uly", "uly", "Upper left y value bounding box (in geocoordinates if georef is true). 0 is read from input file", 0.0);
  Optionpk<double> lrx_opt("lrx", "lrx", "Lower right x value bounding box (in geocoordinates if georef is true). 0 is read from input file", 0.0);
  Optionpk<double> lry_opt("lry", "lry", "Lower right y value bounding box (in geocoordinates if georef is true). 0 is read from input file", 0.0);
  Optionpk<string> otype_opt("ot", "otype", "Data type for output image ({Byte/Int16/UInt16/UInt32/Int32/Float32/Float64/CInt16/CInt32/CFloat32/CFloat64}). Empty string: inherit type from input image", "Byte");
  Optionpk<string> oformat_opt("of", "oformat", "Output image format (see also gdal_translate).", "GTiff");
  Optionpk<double> dx_opt("dx", "dx", "Output resolution in x (in meter)", 1.0);
  Optionpk<double> dy_opt("dy", "dy", "Output resolution in y (in meter)", 1.0);
  Optionpk<short> nbin_opt("nbin", "nbin", "Number of percentile bins for calculating percentile height value profile (=number of output bands)", 10.0);
  Optionpk<double> percentile_opt("perc","percentile","Percentile value used for rule percentile",95);
  Optionpk<short> nodata_opt("nodata", "nodata", "nodata value to put in image", 0);
  Optionpk<string> option_opt("co", "co", "Creation option for output file. Multiple options can be specified.");
  Optionpk<string> colorTable_opt("ct", "ct", "color table (file with 5 columns: id R G B ALFA (0: transparent, 255: solid)");
  Optionpk<short> verbose_opt("v", "verbose", "verbose mode", 0,2);

  nbin_opt.setHide(1);
  percentile_opt.setHide(1);
  nodata_opt.setHide(1);
  option_opt.setHide(1);
  colorTable_opt.setHide(1);

  bool doProcess;//stop process when program was invoked with help option (-h --help)
  try{
    doProcess=input_opt.retrieveOption(argc,argv);
    attribute_opt.retrieveOption(argc,argv);
    returns_opt.retrieveOption(argc,argv);
    classes_opt.retrieveOption(argc,argv);
    composite_opt.retrieveOption(argc,argv);
    filter_opt.retrieveOption(argc,argv);
    angle_min_opt.retrieveOption(argc,argv);
    angle_max_opt.retrieveOption(argc,argv);
    output_opt.retrieveOption(argc,argv);
    projection_opt.retrieveOption(argc,argv);
    ulx_opt.retrieveOption(argc,argv);
    uly_opt.retrieveOption(argc,argv);
    lrx_opt.retrieveOption(argc,argv);
    lry_opt.retrieveOption(argc,argv);
    otype_opt.retrieveOption(argc,argv);
    oformat_opt.retrieveOption(argc,argv);
    dx_opt.retrieveOption(argc,argv);
    dy_opt.retrieveOption(argc,argv);
    nbin_opt.retrieveOption(argc,argv);
    percentile_opt.retrieveOption(argc,argv);
    nodata_opt.retrieveOption(argc,argv);
    option_opt.retrieveOption(argc,argv);
    colorTable_opt.retrieveOption(argc,argv);
    verbose_opt.retrieveOption(argc,argv);
  }
  catch(string predefinedString){
    std::cout << predefinedString << std::endl;
    exit(0);
  }

  if(!doProcess){
    cout << endl;
    cout << "pklas2img -i lasfile -o output" << endl;
    cout << endl;
    std::cout << "short option -h shows basic options only, use long option --help to show all options" << std::endl;
    exit(0);//help was invoked, stop processing
  }
  //todo: is this needed?
  GDALAllRegister();

  const char* pszMessage;
  void* pProgressArg=NULL;
  GDALProgressFunc pfnProgress=GDALTermProgress;
  double progress=0;

  Vector2d<vector<double> > inputData;//row,col,point

   
  ImgRasterGdal maskReader;
  ImgRasterGdal outputWriter;
  GDALDataType theType=GDT_Unknown;
  if(verbose_opt[0])
    cout << "possible output data types: ";
  for(int iType = 0; iType < GDT_TypeCount; ++iType){
    if(verbose_opt[0])
      cout << " " << GDALGetDataTypeName((GDALDataType)iType);
    if( GDALGetDataTypeName((GDALDataType)iType) != NULL
        && EQUAL(GDALGetDataTypeName((GDALDataType)iType),
                 otype_opt[0].c_str()))
      theType=(GDALDataType) iType;
  }
  if(attribute_opt[0]=="spacing"){
    if(theType!=GDT_Float32||theType!=GDT_Float64)
      theType=GDT_Float32;
  }
  if(verbose_opt[0]){
    if(theType==GDT_Unknown)
      cout << "Unknown output pixel type: " << otype_opt[0] << endl;
    else
      cout << "Output pixel type:  " << GDALGetDataTypeName(theType) << endl;
  }

  double maxLRX=0;
  double maxULY=0;
  double minULX=0;
  double minLRY=0;

  unsigned long int totalPoints=0;
  unsigned long int nPoints=0;
  unsigned long int ipoint=0;
  for(int iinput=0;iinput<input_opt.size();++iinput){
    // assert(input_opt[iinput].find(".las")!=string::npos);
    FileReaderLas lasReader;
    try{
      lasReader.open(input_opt[iinput]);
    }
    catch(string errorString){
      cerr << errorString << endl;
      exit(1);
    }
    catch(...){
      cerr << "Error opening input " << input_opt[iinput] << endl;
      exit(2);
    }
    nPoints=lasReader.getPointCount();
    totalPoints+=nPoints;

    if(ulx_opt[0]>=lrx_opt[0]||uly_opt[0]<=lry_opt[0]){
      double ulx,uly,lrx,lry;
      lasReader.getExtent(ulx,uly,lrx,lry);
      lrx+=dx_opt[0];//pixel coordinates are referenced to upper left corner (las coordinates are centres)
      lry-=dy_opt[0];//pixel coordinates are referenced to upper left corner (las coordinates are centres)
      if(ulx>=lrx){
        ulx=ulx-dx_opt[0]/2.0;
        lrx=ulx+dx_opt[0]/2.0;
      }
      if(uly<=lry){
        uly=lry+dy_opt[0]/2.0;
        lry=lry-dy_opt[0]/2.0;
      }
      if(maxLRX>minULX){
        maxLRX=(lrx>maxLRX)?lrx:maxLRX;
        maxULY=(uly>maxULY)?uly:maxULY;
        minULX=(ulx<minULX)?ulx:minULX;
        minLRY=(lry<minLRY)?lry:minLRY;
      }
      else{//initialize
        maxLRX=lrx;
        maxULY=uly;
        minULX=ulx;
        minLRY=lry;
      }        
    }
    else{
      maxLRX=lrx_opt[0];
      maxULY=uly_opt[0];
      minULX=ulx_opt[0];
      minLRY=lry_opt[0];
    }
    lasReader.close();
  }
  if(verbose_opt[0]){
    std::cout << setprecision(12) << "--ulx=" << minULX << " --uly=" << maxULY << " --lrx=" << maxLRX << " --lry=" << minLRY << std::endl;
    std::cout << "total number of points before filtering: " << totalPoints << std::endl;
    std::cout << "filter set to " << filter_opt[0] << std::endl;
    if(angle_min_opt.size())
      std::cout << "minimum scan angle: " << angle_min_opt[0] << std::endl;
    if(angle_max_opt.size())
      std::cout << "maximum scan angle: " << angle_max_opt[0] << std::endl;
    // std::cout << "postFilter set to " << postFilter_opt[0] << std::endl;
  }
  int ncol=ceil(maxLRX-minULX)/dx_opt[0];//number of columns in outputGrid
  int nrow=ceil(maxULY-minLRY)/dy_opt[0];//number of rows in outputGrid
  //todo: multiple bands
  int nband=(composite_opt[0]=="profile")? nbin_opt[0] : 1;
  if(!output_opt.size()){
    cerr << "Error: no output file defined" << endl;
    exit(1);
  }
  if(verbose_opt[0])
    cout << "opening output file " << output_opt[0] << endl;
  outputWriter.open(output_opt[0],ncol,nrow,nband,theType,oformat_opt[0],option_opt);
  outputWriter.GDALSetNoDataValue(nodata_opt[0]);
  //set projection
  double gt[6];
  gt[0]=minULX;
  gt[1]=dx_opt[0];
  gt[2]=0;
  gt[3]=maxULY;
  gt[4]=0;
  gt[5]=-dy_opt[0];
  outputWriter.setGeoTransform(gt);
  if(projection_opt.size())
    outputWriter.setProjectionProj4(projection_opt[0]);
  if(!outputWriter.isGeoRef())
    cout << "Warning: output image " << output_opt[0] << " is not georeferenced!" << endl;
  if(colorTable_opt.size())
    outputWriter.setColorTable(colorTable_opt[0]);

  inputData.clear();
  inputData.resize(nrow,ncol);
  Vector2d<double> outputData(nrow,ncol);
  for(int irow=0;irow<nrow;++irow)
    for(int icol=0;icol<ncol;++icol)
      outputData[irow][icol]=0;

  cout << "Reading " << input_opt.size() << " point cloud files" << endl;
  pfnProgress(progress,pszMessage,pProgressArg);
  for(int iinput=0;iinput<input_opt.size();++iinput){
    FileReaderLas lasReader;
    try{
      lasReader.open(input_opt[iinput]);
    }
    catch(string errorString){
      cout << errorString << endl;
      exit(1);
    }
    if(verbose_opt[0]){
      if(lasReader.isCompressed())
	cout << "Reading compressed point cloud " << input_opt[iinput]<< endl;
      else
	cout << "Reading uncompressed point cloud " << input_opt[iinput] << endl;
    }
    //set bounding filter
    // lasReader.addBoundsFilter(minULX,maxULY,maxLRX,minLRY);
    //set returns filter
    if(returns_opt.size())
      lasReader.addReturnsFilter(returns_opt);
    if(classes_opt.size())
      lasReader.addClassFilter(classes_opt);
    lasReader.setFilters();

    if(attribute_opt[0]!="z"){
      vector<boost::uint16_t> returnsVector;
      vector<string>::iterator ait=attribute_opt.begin();
      while(ait!=attribute_opt.end()){
        if(*ait=="intensity"){
          if(verbose_opt[0])
            std::cout << "writing intensity" << std::endl;
          ++ait;
        }
        else if(*ait=="angle"){
          if(verbose_opt[0])
            std::cout << "writing angle" << std::endl;
          ++ait;
        }
        else if(*ait=="return"){
          if(verbose_opt[0])
            std::cout << "writing return number" << std::endl;
          ++ait;
        }
        else if(*ait=="nreturn"){
          if(verbose_opt[0])
            std::cout << "writing number of returns" << std::endl;
          ++ait;
        }
        else if(*ait=="spacing"){
          if(verbose_opt[0])
            std::cout << "writing spacing" << std::endl;
          ++ait;
        }
        else
          attribute_opt.erase(ait);
      }
    }

    // liblas::Point thePoint(&(lasReader.getHeader()));
    // while(lasReader.readNextPoint(thePoint)){
    OGRSpatialReference projectionRef(outputWriter.getProjectionRef().c_str());
    OGRPoint ogrPoint;
    OGRPoint ogrCenter;
    if(attribute_opt[0]=="spacing"){
      ogrPoint.assignSpatialReference(&projectionRef);
      ogrCenter.assignSpatialReference(&projectionRef);
    }
    while(lasReader.getReader()->ReadNextPoint()){
      liblas::Point const& thePoint = lasReader.getReader()->GetPoint();
      // liblas::Point const& thePoint=lasReader.getPoint();
      progress=static_cast<float>(ipoint)/totalPoints;
      pfnProgress(progress,pszMessage,pProgressArg);
      double theX=thePoint.GetX();
      double theY=thePoint.GetY();
      if(verbose_opt[0]>1)
        cout << "reading point " << ipoint << endl;
      if(theX<minULX||theX>=maxLRX||theY>=maxULY||theY<minLRY)
        continue;
      if((filter_opt[0]=="single")&&(thePoint.GetNumberOfReturns()!=1))
        continue;
      if((filter_opt[0]=="multiple")&&(thePoint.GetNumberOfReturns()<2))
        continue;
      if((filter_opt[0]=="last")&&(thePoint.GetReturnNumber()!=thePoint.GetNumberOfReturns()))
	continue;
      if((filter_opt[0]=="first")&&(thePoint.GetReturnNumber()!=1))
	continue;
      if(angle_min_opt.size()){
	if(angle_min_opt[0]>thePoint.GetScanAngleRank())
	  continue;
      }
      if(angle_max_opt.size()){
	if(angle_max_opt[0]<thePoint.GetScanAngleRank())
	  continue;
      }
      double dcol,drow;
      outputWriter.geo2image(theX,theY,dcol,drow);
      int icol=static_cast<int>(dcol);
      int irow=static_cast<int>(drow);
      if(irow<0||irow>=nrow){
	// //test
	// cout << "Error: thePoint.GetX(),thePoint.GetY(),dcol,drow" << thePoint.GetX() << ", " << thePoint.GetY() << ", " << dcol << ", " << drow << endl;
	continue;
      }
      if(icol<0||icol>=ncol){
	// //test
	// cout << "Error: thePoint.GetX(),thePoint.GetY(),dcol,drow" << thePoint.GetX() << ", " << thePoint.GetY() << ", " << dcol << ", " << drow << endl;
	continue;
      }
      assert(irow>=0);
      assert(irow<nrow);
      assert(icol>=0);
      assert(icol<ncol);
      if(composite_opt[0]=="number")
        outputData[irow][icol]+=1;
      else if(attribute_opt[0]=="z")
        inputData[irow][icol].push_back(thePoint.GetZ());
      else if(attribute_opt[0]=="intensity")
        inputData[irow][icol].push_back(thePoint.GetIntensity());
      else if(attribute_opt[0]=="angle")
        inputData[irow][icol].push_back(thePoint.GetScanAngleRank());
      else if(attribute_opt[0]=="return")
        inputData[irow][icol].push_back(thePoint.GetReturnNumber());
      else if(attribute_opt[0]=="nreturn")
        inputData[irow][icol].push_back(thePoint.GetNumberOfReturns());
      else if(attribute_opt[0]=="spacing"){
	ogrPoint.setX(theX);
	ogrPoint.setY(theY);
	double centerX;
	double centerY;
	outputWriter.image2geo(icol,irow,centerX,centerY);
	ogrCenter.setX(centerX);
	ogrCenter.setY(centerY);
	inputData[irow][icol].push_back(ogrPoint.Distance(&ogrCenter));
      }
      else{
        std::string errorString="attribute not supported";
        throw(errorString);
      }
      ++ipoint;
    }
    if(verbose_opt[0])
      std::cout << "number of points: " << ipoint << std::endl;
    lasReader.close();
  }
  progress=1;
  pfnProgress(progress,pszMessage,pProgressArg);

  std::cout << "processing LiDAR points" << std::endl;
  progress=0;
  pfnProgress(progress,pszMessage,pProgressArg);
  statfactory::StatFactory stat;
  //fill inputData in outputData
  // if(composite_opt[0]=="profile"){
    // assert(postFilter_opt[0]=="none");
    // for(int iband=0;iband<nband;++iband)
      // outputProfile[iband].resize(nrow,ncol);
  // }
  for(int irow=0;irow<nrow;++irow){
    if(composite_opt[0]=="number")
      continue;//outputData already set
    Vector2d<double> outputProfile(nband,ncol);
    for(int icol=0;icol<ncol;++icol){
      std::vector<double> profile;
      if(!inputData[irow][icol].size())
        outputData[irow][icol]=(static_cast<double>((nodata_opt[0])));
      else{
        statfactory::StatFactory stat;
        if(composite_opt[0]=="min")
          outputData[irow][icol]=stat.mymin(inputData[irow][icol]);
        else if(composite_opt[0]=="max")
          outputData[irow][icol]=stat.mymax(inputData[irow][icol]);
        else if(composite_opt[0]=="absmin")
          outputData[irow][icol]=stat.absmin(inputData[irow][icol]);
        else if(composite_opt[0]=="absmax")
          outputData[irow][icol]=stat.absmax(inputData[irow][icol]);
        else if(composite_opt[0]=="median")
          outputData[irow][icol]=stat.median(inputData[irow][icol]);
        else if(composite_opt[0]=="percentile")
          outputData[irow][icol]=stat.percentile(inputData[irow][icol],inputData[irow][icol].begin(),inputData[irow][icol].end(),percentile_opt[0]);
        else if(composite_opt[0]=="mean")
          outputData[irow][icol]=stat.mean(inputData[irow][icol]);
        else if(composite_opt[0]=="var")
          outputData[irow][icol]=stat.var(inputData[irow][icol]);
        else if(composite_opt[0]=="stdev")
          outputData[irow][icol]=sqrt(stat.var(inputData[irow][icol]));
        else if(composite_opt[0]=="sum")
          outputData[irow][icol]=stat.sum(inputData[irow][icol]);
        else if(composite_opt[0]=="first")
          outputData[irow][icol]=inputData[irow][icol][0];
        else if(composite_opt[0]=="last")
          outputData[irow][icol]=inputData[irow][icol].back();
        else if(composite_opt[0]=="profile"){
          if(inputData[irow][icol].size()<2){
            for(int iband=0;iband<nband;++iband)
              outputProfile[iband][icol]=static_cast<double>(nodata_opt[0]);
            continue;
          }
          double min=0;
          double max=0;
          stat.minmax(inputData[irow][icol],inputData[irow][icol].begin(),inputData[irow][icol].end(),min,max);
          if(verbose_opt[0])
            std::cout << "min,max: " << min << "," << max << std::endl;
          if(max>min){
            stat.percentiles(inputData[irow][icol],inputData[irow][icol].begin(),inputData[irow][icol].end(),profile,nband,min,max);
            assert(profile.size()==nband);
            for(int iband=0;iband<nband;++iband)
              outputProfile[iband][icol]=profile[iband];
          }
          else{
            for(int iband=0;iband<nband;++iband)
              outputProfile[iband][icol]=max;
          }
        }
        else{
          std::cout << "Error: composite_opt " << composite_opt[0] << " not supported" << std::endl;
          exit(2);
        }
      }
    }
    if(composite_opt[0]=="profile"){
      for(int iband=0;iband<nband;++iband){
        // assert(outputProfile[iband].size()==outputWriter.nrOfRow());
        assert(outputProfile[iband].size()==outputWriter.nrOfCol());
        try{
          outputWriter.writeData(outputProfile[iband],irow,iband);
        }
        catch(std::string errorString){
          cout << errorString << endl;
          exit(1);
        }
      }
    }
    progress=static_cast<float>(irow)/outputWriter.nrOfRow();
    pfnProgress(progress,pszMessage,pProgressArg);
  }
  progress=1;
  pfnProgress(progress,pszMessage,pProgressArg);
  inputData.clear();//clean up memory
  //apply post filter
  // std::cout << "Applying post processing filter: " << postFilter_opt[0] << std::endl;
  // if(postFilter_opt[0]=="etew_min"){
  //   if(composite_opt[0]!="min")
  //     std::cout << "Warning: composite option is not set to min!" << std::endl;
  //   //Elevation Threshold with Expand Window (ETEW) Filter (p.73 frmo Airborne LIDAR Data Processing and Analysis Tools ALDPAT 1.0)
  //   //first iteration is performed assuming only minima are selected using options -fir all -comp min
  //   unsigned long int nchange=1;
  //   //increase cells and thresholds until no points from the previous iteration are discarded.
  //   int dimx=dimx_opt[0];
  //   int dimy=dimy_opt[0];
  //   filter2d::Filter2d morphFilter;
  //   // morphFilter.setNoValue(0);
  //   Vector2d<float> currentOutput=outputData;
  //   int iteration=1;
  //   while(nchange&&iteration<=maxIter_opt[0]){
  //     double hThreshold=maxSlope_opt[0]*dimx;
  //     Vector2d<float> newOutput;
  //     nchange=morphFilter.morphology(currentOutput,newOutput,"erode",dimx,dimy,disc_opt[0],hThreshold);
  //     currentOutput=newOutput;
  //     dimx+=2;//change from theory: originally double cellCize
  //     dimy+=2;//change from theory: originally double cellCize
  //     std::cout << "iteration " << iteration << ": " << nchange << " pixels changed" << std::endl;
  //     ++iteration;
  //   }
  //   outputData=currentOutput;
  // }    
  // else if(postFilter_opt[0]=="promorph"||postFilter_opt[0]=="bunting"){
  //   if(composite_opt[0]!="min")
  //     std::cout << "Warning: composite option is not set to min!" << std::endl;
  //   assert(hThreshold_opt.size()>1);
  //   //Progressive morphological filter tgrs2003_zhang vol41 pp 872-882
  //   //first iteration is performed assuming only minima are selected using options -fir all -comp min
  //   //increase cells and thresholds until no points from the previous iteration are discarded.
  //   int dimx=dimx_opt[0];
  //   int dimy=dimy_opt[0];
  //   filter2d::Filter2d theFilter;
  //   // theFilter.setNoValue(0);
  //   Vector2d<float> currentOutput=outputData;
  //   double hThreshold=hThreshold_opt[0];
  //   int iteration=1;
  //   while(iteration<=maxIter_opt[0]){
  //     std::cout << "iteration " << iteration << " with window size " << dimx << " and dh_max: " << hThreshold << std::endl;
  //     Vector2d<float> newOutput;
  //     try{
  //       theFilter.morphology(outputData,currentOutput,"erode",dimx,dimy,disc_opt[0],hThreshold);
  //       theFilter.morphology(currentOutput,outputData,"dilate",dimx,dimy,disc_opt[0],hThreshold);
  //       if(postFilter_opt[0]=="bunting"){
  //         theFilter.doit(outputData,currentOutput,"median",dimx,dimy,1,disc_opt[0]);
  //         outputData=currentOutput;
  //       }
  //     }
  //     catch(std::string errorString){
  //       cout << errorString << endl;
  //       exit(1);
  //     }
  //     int newdimx=(dimx==1)? 3: 2*(dimx-1)+1;
  //     int newdimy=(dimx==1)? 3: 2*(dimy-1)+1;//from PE&RS vol 71 pp313-324
  //     hThreshold=hThreshold_opt[0]+maxSlope_opt[0]*(newdimx-dimx)*dx_opt[0];
  //     dimx=newdimx;
  //     dimy=newdimy;
  //     if(hThreshold>hThreshold_opt[1])
  //       hThreshold=hThreshold_opt[1];
  //     ++iteration;
  //   }
  //   outputData=currentOutput;
  // }    
  // else if(postFilter_opt[0]=="open"){
  //   if(composite_opt[0]!="min")
  //     std::cout << "Warning: composite option is not set to min!" << std::endl;
  //   filter2d::Filter2d morphFilter;
  //   // morphFilter.setNoValue(0);
  //   Vector2d<float> filterInput=outputData;
  //   try{
  //     morphFilter.morphology(outputData,filterInput,"erode",dimx_opt[0],dimy_opt[0],disc_opt[0],maxSlope_opt[0]);
  //     morphFilter.morphology(filterInput,outputData,"dilate",dimx_opt[0],dimy_opt[0],disc_opt[0],maxSlope_opt[0]);
  //   }
  //   catch(std::string errorString){
  //     cout << errorString << endl;
  //     exit(1);
  //   }
  // }
  // else if(postFilter_opt[0]=="close"){
  //   if(composite_opt[0]!="max")
  //     std::cout << "Warning: composite option is not set to max!" << std::endl;
  //   filter2d::Filter2d morphFilter;
  //   // morphFilter.setNoValue(0);
  //   Vector2d<float> filterInput=outputData;
  //   try{
  //     morphFilter.morphology(outputData,filterInput,"dilate",dimx_opt[0],dimy_opt[0],disc_opt[0],maxSlope_opt[0]);
  //     morphFilter.morphology(filterInput,outputData,"erode",dimx_opt[0],dimy_opt[0],disc_opt[0],maxSlope_opt[0]);
  //   }
  //   catch(std::string errorString){
  //     cout << errorString << endl;
  //     exit(1);
  //   }
  // }
  if(composite_opt[0]!="profile"){
    //write output file
    std::cout << "writing output raster file" << std::endl;
    progress=0;
    pfnProgress(progress,pszMessage,pProgressArg);
    for(int irow=0;irow<nrow;++irow){
      try{
        assert(outputData.size()==outputWriter.nrOfRow());
        assert(outputData[0].size()==outputWriter.nrOfCol());
        outputWriter.writeData(outputData[irow],irow,0);
      }
      catch(std::string errorString){
        cout << errorString << endl;
        exit(1);
      }
      progress=static_cast<float>(irow)/outputWriter.nrOfRow();
      pfnProgress(progress,pszMessage,pProgressArg);
    }
  }
  progress=1;
  pfnProgress(progress,pszMessage,pProgressArg);
  if(verbose_opt[0])
    std::cout << "closing lasReader" << std::endl;
  outputWriter.close();
}
