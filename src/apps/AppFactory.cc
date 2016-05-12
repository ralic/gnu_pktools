#include <algorithm>
#include <iostream>
#include <string>
#include "imageclasses/ImgReaderOgr.h"
#include "base/Vector2d.h"
#include "base/Optionpk.h"
#include "algorithms/StatFactory.h"
#include "AppFactory.h"

using namespace std;
using namespace appfactory;

void AppFactory::setOptions(int argc, char* argv[]){
  m_argc=argc;
  for(int iarg=0;iarg<argc;++iarg)
    m_argv.push_back(argv[iarg]);
}

bool AppFactory::pkcomposite(vector<ImgReaderGdal>& imgReader, ImgWriterGdal& imgWriter){
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
    doProcess=band_opt.retrieveOption(m_argc,m_argv);
    dx_opt.retrieveOption(m_argc,m_argv);
    dy_opt.retrieveOption(m_argc,m_argv);
    extent_opt.retrieveOption(m_argc,m_argv);
    cut_opt.retrieveOption(m_argc,m_argv);
    eoption_opt.retrieveOption(m_argc,m_argv);
    mask_opt.retrieveOption(m_argc,m_argv);
    msknodata_opt.retrieveOption(m_argc,m_argv);
    mskband_opt.retrieveOption(m_argc,m_argv);
    ulx_opt.retrieveOption(m_argc,m_argv);
    uly_opt.retrieveOption(m_argc,m_argv);
    lrx_opt.retrieveOption(m_argc,m_argv);
    lry_opt.retrieveOption(m_argc,m_argv);
    crule_opt.retrieveOption(m_argc,m_argv);
    ruleBand_opt.retrieveOption(m_argc,m_argv);
    srcnodata_opt.retrieveOption(m_argc,m_argv);
    bndnodata_opt.retrieveOption(m_argc,m_argv);
    minValue_opt.retrieveOption(m_argc,m_argv);
    maxValue_opt.retrieveOption(m_argc,m_argv);
    dstnodata_opt.retrieveOption(m_argc,m_argv);
    resample_opt.retrieveOption(m_argc,m_argv);
    otype_opt.retrieveOption(m_argc,m_argv);
    oformat_opt.retrieveOption(m_argc,m_argv);
    option_opt.retrieveOption(m_argc,m_argv);
    projection_opt.retrieveOption(m_argc,m_argv);
    file_opt.retrieveOption(m_argc,m_argv);
    weight_opt.retrieveOption(m_argc,m_argv);
    class_opt.retrieveOption(m_argc,m_argv);
    colorTable_opt.retrieveOption(m_argc,m_argv);
    description_opt.retrieveOption(m_argc,m_argv);
    align_opt.retrieveOption(m_argc,m_argv);
    scale_opt.retrieveOption(m_argc,m_argv);
    offset_opt.retrieveOption(m_argc,m_argv);
    memory_opt.retrieveOption(m_argc,m_argv);
    verbose_opt.retrieveOption(m_argc,m_argv);
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
  // ///Output image filename
  // vector<string>  output_opt;
  // ///band index(es) to crop (leave empty if all bands must be retained)
  // vector<int>  band_opt;
  // ///Output resolution in x (in meter) (empty: keep original resolution)
  // vector<double>  dx_opt;
  // ///Output resolution in y (in meter) (empty: keep original resolution)
  // vector<double>  dy_opt;
  // ///get boundary from extent from polygons in vector file
  // vector<string>  extent_opt;
  // ///Crop the extent of the target dataset to the extent of the cutline.
  // vector<bool> cut_opt;
  // ///special extent options controlling rasterization: ATTRIBUTE|CHUNKYSIZE|ALL_TOUCHED|BURN_VALUE_FROM|MERGE_ALG, e.g., -eo ATTRIBUTE=fieldname
  // vector<string> eoption_opt;
  // ///Use the first band of the specified file as a validity mask (0 is nodata).
  // vector<string> mask_opt;
  // ///Mask value not to consider for composite.
  // vector<float> msknodata_opt;
  // ///Mask band to read (0 indexed)
  // vector<short> mskband_opt;
  // ///Upper left x value bounding box
  // vector<double>  ulx_opt;
  // ///Upper left y value bounding box
  // vector<double>  uly_opt;
  // ///Lower right x value bounding box
  // vector<double>  lrx_opt;
  // ///Lower right y value bounding box
  // vector<double>  lry_opt;
  // ///Composite rule (overwrite, maxndvi, maxband, minband, mean, mode (only for byte images), median, sum, maxallbands, minallbands, stdev
  // vector<string> crule_opt;
  // ///band index used for the composite rule (e.g., for ndvi, use --cband=0 --cband=1 with 0 and 1 indices for red and nir band respectively
  // vector<int> ruleBand_opt;
  // ///invalid value(s) for input raster dataset
  // vector<double> srcnodata_opt;
  // ///Band(s) in input image to check if pixel is valid (used for srcnodata, min and max options)
  // vector<int> bndnodata_opt;
  // ///flag values smaller or equal to this value as invalid.
  // vector<double> minValue_opt;
  // ///flag values larger or equal to this value as invalid.
  // vector<double> maxValue_opt;
  // ///nodata value to put in output raster dataset if not valid or out of bounds.
  // vector<double>  dstnodata_opt;
  // ///Resampling method (near: nearest neighbor, bilinear: bi-linear interpolation).
  // vector<string>  resample_opt("near",1);
  // ///Data type for output image ({Byte/Int16/UInt16/UInt32/Int32/Float32/Float64/CInt16/CInt32/CFloat32/CFloat64}). Empty string: inherit type from input image
  // vector<string>  otype_opt("",1);
  // ///Output image format (see also gdal_translate).
  // vector<string>  oformat_opt("GTiff",1);
  // ///Creation option for output file. Multiple options can be specified.
  // vector<string> option_opt;
  // ///Override the spatial reference for the output file (leave blank to copy from input file, use epsg:3035 to use European projection and force to European grid
  // vector<string>  projection_opt;
  // ///write number of observations (1) or sequence nr of selected file (2) for each pixels as additional layer in composite
  // vector<short> file_opt;
  // ///Weights (type: short) for the composite, use one weight for each input file in same order as input files are provided). Use value 1 for equal weights.
  // vector<short> weight_opt;
  // ///classes for multi-band output image: each band represents the number of observations for one specific class. Use value 0 for no multi-band output image.
  // vector<short> class_opt;
  // ///color table file with 5 columns: id R G B ALFA (0: transparent, 255: solid)
  // vector<string>  colorTable_opt;
  // ///Set image description
  // vector<string>  description_opt;
  // ///Align output bounding box to input image
  // vector<bool>  align_opt;
  // ///output=scale*input+offset
  // vector<double> scale_opt;
  // ///output=scale*input+offset
  // vector<double> offset_opt;
  // ///verbose
  // vector<short> verbose_opt(0,1);

  //test
  cout << "debug0" << endl;
  std::map<std::string, crule::CRULE_TYPE> cruleMap;
  // //initialize cruleMap
  // enum CRULE_TYPE {overwrite=0, maxndvi=1, maxband=2, minband=3, validband=4, mean=5, mode=6, median=7,sum=8};
  
  cruleMap["overwrite"]=crule::overwrite;
  cruleMap["maxndvi"]=crule::maxndvi;
  cruleMap["maxband"]=crule::maxband;
  cruleMap["minband"]=crule::minband;
  cruleMap["validband"]=crule::validband;
  cruleMap["mean"]=crule::mean;
  cruleMap["mode"]=crule::mode;
  cruleMap["median"]=crule::median;
  cruleMap["sum"]=crule::sum;
  cruleMap["maxallbands"]=crule::maxallbands;
  cruleMap["minallbands"]=crule::minallbands;
  cruleMap["stdev"]=crule::stdev;

 //test
 cout << "debug0" << endl;

  if(srcnodata_opt.size()){
    while(srcnodata_opt.size()<bndnodata_opt.size())
      srcnodata_opt.push_back(srcnodata_opt[0]);
  }
  while(bndnodata_opt.size()<srcnodata_opt.size())
    bndnodata_opt.push_back(bndnodata_opt[0]);
  if(minValue_opt.size()){
    while(minValue_opt.size()<bndnodata_opt.size())
      minValue_opt.push_back(minValue_opt[0]);
    while(bndnodata_opt.size()<minValue_opt.size())
      bndnodata_opt.push_back(bndnodata_opt[0]);
  }
  if(maxValue_opt.size()){
    while(maxValue_opt.size()<bndnodata_opt.size())
      maxValue_opt.push_back(maxValue_opt[0]);
    while(bndnodata_opt.size()<maxValue_opt.size())
      bndnodata_opt.push_back(bndnodata_opt[0]);
  }
  RESAMPLE theResample;
  if(resample_opt[0]=="near"){
    theResample=NEAR;
    if(verbose_opt[0])
      cout << "resampling: nearest neighbor" << endl;
  }
  else if(resample_opt[0]=="bilinear"){
    theResample=BILINEAR;
    if(verbose_opt[0])
      cout << "resampling: bilinear interpolation" << endl;
  }
  else{
    std::cout << "Error: resampling method " << resample_opt[0] << " not supported" << std::endl;
    exit(1);
  }
  
  if(imgReader.empty()){
    std::cerr << "No input file provided (use option -i). Use --help for help information" << std::endl;
    exit(0);
  }
  int nband=0;
  int nwriteBand=0;
  int writeBand=0;
  vector<short> bands;

  //test
  cout << "debug1" << endl;
  
  //get bounding box
  double maxLRX=lrx_opt[0];
  double maxULY=uly_opt[0];
  double minULX=ulx_opt[0];
  double minLRY=lry_opt[0];
  double magic_x=1,magic_y=1;//magic pixel for GDAL map info

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
  if(verbose_opt[0]){
    cout << endl;
    if(theType==GDT_Unknown)
      cout << "Unknown output pixel type: " << otype_opt[0] << endl;
    else
      cout << "Output pixel type:  " << GDALGetDataTypeName(theType) << endl;
  }

  //test
  cout << "debug2" << endl;

  double dx=0;
  double dy=0;
  //get bounding box from extentReader if defined
  ImgReaderOgr extentReader;
  if(extent_opt.size()){
    double e_ulx;
    double e_uly;
    double e_lrx;
    double e_lry;
    for(int iextent=0;iextent<extent_opt.size();++iextent){
      extentReader.open(extent_opt[iextent]);
      if(!(extentReader.getExtent(e_ulx,e_uly,e_lrx,e_lry))){
        cerr << "Error: could not get extent from " << extent_opt[0] << endl;
        exit(1);
      }
      if(!iextent){
	ulx_opt[0]=e_ulx;
	uly_opt[0]=e_uly;
	lrx_opt[0]=e_lrx;
	lry_opt[0]=e_lry;
      }
      else{
	if(e_ulx<ulx_opt[0])
	  ulx_opt[0]=e_ulx;
	if(e_uly>uly_opt[0])
	  uly_opt[0]=e_uly;
	if(e_lrx>lrx_opt[0])
	  lrx_opt[0]=e_lrx;
	if(e_lry<lry_opt[0])
	  lry_opt[0]=e_lry;
      }
      extentReader.close();
    }
    if(maxLRX>minULX&&minULX>ulx_opt[0])
      ulx_opt[0]=minULX;
    if(maxLRX>minULX&&maxLRX<lrx_opt[0])
      lrx_opt[0]=maxLRX;
    if(maxULY>minLRY&&maxULY<uly_opt[0])
      uly_opt[0]=maxULY;
    if(minLRY<maxULY&&minLRY>lry_opt[0])
      lry_opt[0]=minLRY;
    if(cut_opt.size()||eoption_opt.size())
      extentReader.open(extent_opt[0]);
      extentReader.open(extent_opt[0]);
  }

  //test
  cout << "debug3" << endl;

  if(verbose_opt[0])
    cout << "--ulx=" << ulx_opt[0] << " --uly=" << uly_opt[0] << " --lrx=" << lrx_opt[0] << " --lry=" << lry_opt[0] << endl;


  string theProjection="";
  GDALColorTable* theColorTable=NULL;
  string imageType;
  bool init=false;

  //test
  cout << "debug4" << endl;

  for(int ifile=0;ifile<imgReader.size();++ifile){
    //todo: must be in init part only?
    if(colorTable_opt.empty())
      if(imgReader[ifile].getColorTable())
	theColorTable=(imgReader[ifile].getColorTable()->Clone());
    if(projection_opt.empty())
      theProjection=imgReader[ifile].getProjection();
    // if(option_opt.findSubstring("INTERLEAVE=")==option_opt.end()){
    //   string theInterleave="INTERLEAVE=";
    //   theInterleave+=imgReader[ifile].getInterleave();
    //   option_opt.push_back(theInterleave);
    // }

    if((ulx_opt[0]||uly_opt[0]||lrx_opt[0]||lry_opt[0])&&(!imgReader[ifile].covers(ulx_opt[0],uly_opt[0],lrx_opt[0],lry_opt[0]))){
      if(verbose_opt[0])
	cout << "Input " << ifile << " not within bounding box, skipping..." << endl;
      continue;
    }
    double theULX, theULY, theLRX, theLRY;
    imgReader[ifile].getBoundingBox(theULX,theULY,theLRX,theLRY);
    if(theLRY>theULY){
      cerr << "Error: input " << ifile << " is not georeferenced, only referenced images are supported for pkcomposite " << endl;
      exit(1);
    }
    if(verbose_opt[0])
      cout << "Bounding Box (ULX ULY LRX LRY): " << fixed << setprecision(6) << theULX << " " << theULY << " " << theLRX << " " << theLRY << endl;
    if(!init){
      if(verbose_opt[0]){
        switch(cruleMap[crule_opt[0]]){
        default:
        case(crule::overwrite):
          cout << "Composite rule: overwrite" << endl;
          break;
        case(crule::maxndvi):
          cout << "Composite rule: max ndvi" << endl;
          break;
        case(crule::maxband):
          cout << "Composite rule: max band" << endl;
          break;
        case(crule::minband):
          cout << "Composite rule: min band" << endl;
          break;
        case(crule::validband):
          cout << "Composite rule: valid band" << endl;
          break;
        case(crule::mean):
          cout << "Composite rule: mean value" << endl;
          break;
        case(crule::mode):
          cout << "Composite rule: max voting (only for byte images)" << endl;
          break;
        case(crule::median):
          cout << "Composite rule: median" << endl;
          break;
        case(crule::stdev):
          cout << "Composite rule: stdev" << endl;
          break;
        case(crule::sum):
          cout << "Composite rule: sum" << endl;
          break;
        case(crule::minallbands):
          cout << "Composite rule: minallbands" << endl;
          break;
        case(crule::maxallbands):
          cout << "Composite rule: maxallbands" << endl;
          break;
        }
      }
      if(band_opt.size()){
	nband=band_opt.size();
        bands.resize(band_opt.size());
        for(int iband=0;iband<band_opt.size();++iband){
          bands[iband]=band_opt[iband];
          assert(bands[iband]<imgReader[ifile].nrOfBand());
        }
      }
      else{
	nband=imgReader[ifile].nrOfBand();
        bands.resize(nband);
        for(int iband=0;iband<nband;++iband)
          bands[iband]=iband;
      }
      for(int iband=0;iband<bndnodata_opt.size();++iband){
        assert(bndnodata_opt[iband]>=0&&bndnodata_opt[iband]<nband);
      }
      //if output type not set, get type from input image
      if(theType==GDT_Unknown){
        theType=imgReader[ifile].getDataType();
        if(verbose_opt[0])
          cout << "Using data type from input image: " << GDALGetDataTypeName(theType) << endl;
      }

      if(oformat_opt.size())//default
        imageType=oformat_opt[0];
      else
        imageType=imgReader[ifile].getImageType();

      // dataType=imgReader.getDataType(0);
      if(verbose_opt[0]){
        cout << "type of data for input " << ifile << ": " << theType << endl;
        cout << "nband: " << nband << endl;
      }
      
      maxLRX=theLRX;
      maxULY=theULY;
      minULX=theULX;
      minLRY=theLRY;
      if(dx_opt.size())
	dx=dx_opt[0];
      else
        dx=imgReader[ifile].getDeltaX();
      if(dy_opt.size())
	dy=dy_opt[0];
      else
        dy=imgReader[ifile].getDeltaY();
      // imgReader.getMagicPixel(magic_x,magic_y);
      init=true;
    }
    else{
      //convert bounding box to magic coordinates
      //check uniformity magic pixel
      // double test_x,test_y;
      // imgReader.getMagicPixel(test_x,test_y);
      // if(verbose_opt[0]){
      //   cout << "magic_x, magic_y: " << magic_x << ", " << magic_y << endl;
      // }
      // assert(magic_x==test_x);
      // assert(magic_y==test_y);
      maxLRX=(theLRX>maxLRX)?theLRX:maxLRX;
      maxULY=(theULY>maxULY)?theULY:maxULY;
      minULX=(theULX<minULX)?theULX:minULX;
      minLRY=(theLRY<minLRY)?theLRY:minLRY;
    }
    // imgReader.close();
  }
  if(verbose_opt[0])
    cout << "bounding box input images (ULX ULY LRX LRY): " << fixed << setprecision(6) << minULX << " " << maxULY << " " << maxLRX << " " << minLRY << endl;
  if(ulx_opt[0]||uly_opt[0]||lrx_opt[0]||lry_opt[0]){
    maxLRX=lrx_opt[0];
    maxULY=uly_opt[0];
    minULX=ulx_opt[0];
    minLRY=lry_opt[0];
  }
  
  bool forceEUgrid=false;
  if(projection_opt.size())
    forceEUgrid=(!(projection_opt[0].compare("EPSG:3035"))||!(projection_opt[0].compare("EPSG:3035"))||projection_opt[0].find("ETRS-LAEA")!=string::npos);
  if(forceEUgrid){
    //force to LAEA grid
    minULX=floor(minULX);
    minULX-=static_cast<int>(minULX)%(static_cast<int>(dx));
    maxULY=ceil(maxULY);
    if(static_cast<int>(maxULY)%static_cast<int>(dy))
      maxULY+=dy;
    maxULY-=static_cast<int>(maxULY)%(static_cast<int>(dy));
    maxLRX=ceil(maxLRX);
    if(static_cast<int>(maxLRX)%static_cast<int>(dx))
      maxLRX+=dx;
    maxLRX-=static_cast<int>(maxLRX)%(static_cast<int>(dx));
    minLRY=floor(minLRY);
    minLRY-=static_cast<int>(minLRY)%(static_cast<int>(dy));
  }
  else if(align_opt[0]){
    if(minULX>imgReader[0].getUlx())
      minULX-=fmod(minULX-imgReader[0].getUlx(),dx);
    else if(minULX<imgReader[0].getUlx())
      minULX+=fmod(imgReader[0].getUlx()-minULX,dx)-dx;
    if(maxLRX<imgReader[0].getLrx())
      maxLRX+=fmod(imgReader[0].getLrx()-maxLRX,dx);
    else if(maxLRX>imgReader[0].getLrx())
      maxLRX-=fmod(maxLRX-imgReader[0].getLrx(),dx)+dx;
    if(minLRY>imgReader[0].getLry())
      minLRY-=fmod(minLRY-imgReader[0].getLry(),dy);
    else if(minLRY<imgReader[0].getLry())
      minLRY+=fmod(imgReader[0].getLry()-minLRY,dy)-dy;
    if(maxULY<imgReader[0].getUly())
      maxULY+=fmod(imgReader[0].getUly()-maxULY,dy);
    else if(maxULY>imgReader[0].getUly())
      maxULY-=fmod(maxULY-imgReader[0].getUly(),dy)+dy;
  }

  if(verbose_opt[0])
    cout << "bounding box composite image (ULX ULY LRX LRY): " << fixed << setprecision(6) << minULX << " " << maxULY << " " << maxLRX << " " << minLRY << endl;
  //initialize image
  if(verbose_opt[0])
    cout << "initializing composite image..." << endl;
//   double dcol=(maxLRX-minULX+dx-1)/dx;
//   double drow=(maxULY-minLRY+dy-1)/dy;
//   int ncol=static_cast<int>(dcol);
//   int nrow=static_cast<int>(drow);

  int ncol=ceil((maxLRX-minULX)/dx);
  int nrow=ceil((maxULY-minLRY)/dy);

  if(verbose_opt[0])
    cout << "composite image dim (nrow x ncol): " << nrow << " x " << ncol << endl;
  while(weight_opt.size()< imgReader.size())
    weight_opt.push_back(weight_opt[0]);
  // if(verbose_opt[0]){
  //   std::cout << weight_opt << std::endl;
  // }
  if(cruleMap[crule_opt[0]]==crule::mode){
    nwriteBand=(file_opt[0])? class_opt.size()+1:class_opt.size();
  }
  else
    nwriteBand=(file_opt[0])? bands.size()+1:bands.size();
  // if(output_opt.empty()){
  //   std::cerr << "No output file provided (use option -o). Use --help for help information" << std::endl;
  //   exit(0);
  // }
  // if(verbose_opt[0])
  //   cout << "open output image " << output_opt[0] << " with " << nwriteBand << " bands" << endl << flush;

  //test
  cout << "debug5" << endl;
  //todo: segmentation faults between debug5 and debug6
  //todo: if in memory, do not require filename, else, filename must be set by caller?
  try{
    //open imgWriter in memory
    imgWriter.open(ncol,nrow,nwriteBand,theType);
    // for(int iband=0;iband<nwriteBand;++iband)
    //   imgWriter.GDALSetNoDataValue(dstnodata_opt[0],iband);
  }
  catch(string error){
    cout << error << endl;
  }
  // if(description_opt.size())
  //   imgWriter.setImageDescription(description_opt[0]);
  double gt[6];
  gt[0]=minULX;
  gt[1]=dx;
  gt[2]=0;
  gt[3]=maxULY;
  gt[4]=0;
  gt[5]=-dy;

  //test
  cout << "debug3" << endl;

  //todo: check if I can set geotransform and projection when dataset is not set yet...
  imgWriter.setGeoTransform(gt);
  // imgWriter.setGeoTransform(minULX,maxULY,dx,dy,0,0);
  if(projection_opt.size()){
    if(verbose_opt[0])
      cout << "projection: " << projection_opt[0] << endl;
    imgWriter.setProjectionProj4(projection_opt[0]);
  }
  else if(theProjection!=""){
    if(verbose_opt[0])
      cout << "projection: " << theProjection << endl;
    imgWriter.setProjection(theProjection);
  }
  // if(imgWriter.getDataType()==GDT_Byte){
  //   if(colorTable_opt.size()){
  //     if(colorTable_opt[0]!="none")
  //       imgWriter.setColorTable(colorTable_opt[0]);
  //   }
  //   else if(theColorTable)
  //     imgWriter.setColorTable(theColorTable);
  // }

  //test
  cout << "debug3" << endl;

  ImgWriterGdal maskWriter;
  if(extent_opt.size()&&(cut_opt[0]||eoption_opt.size())){
    try{
      maskWriter.open("/vsimem/mask.tif",ncol,nrow,1,GDT_Float32,"GTiff",option_opt);
      double gt[6];
      gt[0]=minULX;
      gt[1]=dx;
      gt[2]=0;
      gt[3]=maxULY;
      gt[4]=0;
      gt[5]=-dy;
      maskWriter.setGeoTransform(gt);
      if(projection_opt.size())
	maskWriter.setProjectionProj4(projection_opt[0]);
      else if(theProjection!=""){
	if(verbose_opt[0])
	  cout << "projection: " << theProjection << endl;
	maskWriter.setProjection(theProjection);
      }
      vector<double> burnValues(1,1);//burn value is 1 (single band)
      maskWriter.rasterizeOgr(extentReader,burnValues,eoption_opt);
      maskWriter.close();
    }
    catch(string error){
      cerr << error << std::endl;
      exit(2);
    }
    catch(...){
      cerr << "error caught" << std::endl;
      exit(1);
    }
    //todo: support multiple masks
    mask_opt.clear();
    mask_opt.push_back("/vsimem/mask.tif");
  }
  ImgReaderGdal maskReader;
  if(mask_opt.size()){
    try{
      if(verbose_opt[0]>=1)
	std::cout << "opening mask image file " << mask_opt[0] << std::endl;
      maskReader.open(mask_opt[0],GA_ReadOnly,m_memory);
      if(mskband_opt[0]>=maskReader.nrOfBand()){
	string errorString="Error: illegal mask band";
	throw(errorString);
      }
    }
    catch(string error){
      cerr << error << std::endl;
      exit(2);
    }
    catch(...){
      cerr << "error caught" << std::endl;
      exit(1);
    }
  }

  //create composite image
  if(verbose_opt[0])
     cout << "creating composite image" << endl;
  Vector2d<double> writeBuffer(nband,imgWriter.nrOfCol());
  vector<short> fileBuffer(ncol);//holds the number of used files
  Vector2d<short> maxBuffer;//buffer used for maximum voting
  // Vector2d<double> readBuffer(nband);
  vector<Vector2d<unsigned short> > readBuffer(imgReader.size());
  for(int ifile=0;ifile<imgReader.size();++ifile)
    readBuffer[ifile].resize(imgReader[ifile].nrOfBand());
  statfactory::StatFactory stat;
  if(cruleMap[crule_opt[0]]==crule::maxndvi)//ndvi
    assert(ruleBand_opt.size()==2);
  if(cruleMap[crule_opt[0]]==crule::mode){//max voting
    maxBuffer.resize(imgWriter.nrOfCol(),256);//use only byte images for max voting
    for(int iclass=0;iclass<class_opt.size();++iclass)
      assert(class_opt[iclass]<maxBuffer.size());
  }
  int jb=0;
  double readRow=0;
  double readCol=0;
  double lowerCol=0;
  double upperCol=0;
  const char* pszMessage;
  void* pProgressArg=NULL;
  GDALProgressFunc pfnProgress=GDALTermProgress;
  double progress=0;
  pfnProgress(progress,pszMessage,pProgressArg);
  for(int irow=0;irow<imgWriter.nrOfRow();++irow){
    vector<float> lineMask;
    Vector2d< vector<double> > storeBuffer;
    vector<bool> writeValid(ncol);

    //convert irow to geo
    double x=0;
    double y=0;
    imgWriter.image2geo(0,irow,x,y);


    if(cruleMap[crule_opt[0]]==crule::mean ||
       cruleMap[crule_opt[0]]==crule::median ||
       cruleMap[crule_opt[0]]==crule::sum ||
       cruleMap[crule_opt[0]]==crule::minallbands ||
       cruleMap[crule_opt[0]]==crule::maxallbands ||
       cruleMap[crule_opt[0]]==crule::stdev)
      storeBuffer.resize(nband,ncol);
    for(int icol=0;icol<imgWriter.nrOfCol();++icol){
      writeValid[icol]=false;
      fileBuffer[icol]=0;
      if(cruleMap[crule_opt[0]]==crule::mode){//max voting
        for(int iclass=0;iclass<256;++iclass)
          maxBuffer[icol][iclass]=0;
      }
      else{
        for(int iband=0;iband<nband;++iband)
          writeBuffer[iband][icol]=dstnodata_opt[0];
      }
    }

    double oldRowMask=-1;//keep track of row mask to optimize number of line readings

    for(int ifile=0;ifile<imgReader.size();++ifile){
      //imgReader already open...
      // try{
      //   imgReader.open(imgReader[ifile]);
      // }
      // catch(string error){
      //   cout << error << endl;
      // }
      // assert(imgReader.getDataType()==theType);
      assert(imgReader[ifile].nrOfBand()>=nband);
      if(!imgReader[ifile].covers(minULX,maxULY,maxLRX,minLRY)){
        // imgReader.close();
        continue;
      }
      double uli,ulj,lri,lrj;
      imgReader[ifile].geo2image(minULX+(magic_x-1.0)*imgReader[ifile].getDeltaX(),maxULY-(magic_y-1.0)*imgReader[ifile].getDeltaY(),uli,ulj);
      imgReader[ifile].geo2image(maxLRX+(magic_x-2.0)*imgReader[ifile].getDeltaX(),minLRY-(magic_y-2.0)*imgReader[ifile].getDeltaY(),lri,lrj);
      uli=floor(uli);
      ulj=floor(ulj);
      lri=floor(lri);
      lrj=floor(lrj);
        
      double startCol=uli;
      double endCol=lri;
      if(uli<0)
        startCol=0;
      else if(uli>=imgReader[ifile].nrOfCol())
        startCol=imgReader[ifile].nrOfCol()-1;
      if(lri<0)
        endCol=0;
      else if(lri>=imgReader[ifile].nrOfCol())
        endCol=imgReader[ifile].nrOfCol()-1;
      int readncol=endCol-startCol+1;

      //lookup corresponding row for irow in this file
      imgReader[ifile].geo2image(x,y,readCol,readRow);
      if(readRow<0||readRow>=imgReader[ifile].nrOfRow()){
        // imgReader.close();
        continue;
      }
      // for(int iband=0;iband<imgReader.nrOfBand();++iband){
      for(int iband=0;iband<nband;++iband){
	int readBand=(band_opt.size()>iband)? band_opt[iband] : iband;
        // readBuffer[iband].resize(readncol);
	try{
          imgReader[ifile].readData(readBuffer[ifile][iband],startCol,endCol,readRow,readBand,theResample);
	  //test
          // imgReader[ifile].readData(readBuffer[ifile][iband],static_cast<int>(startCol),static_cast<int>(endCol),static_cast<int>(readRow),static_cast<int>(readBand));
	  // if(readRow==0&&iband==0){
	  //   for(int icol=0;icol<10;++icol)
	  //     cout << readBuffer[0][0][icol] << " ";
	  //   cout << endl;
	  // }
	}
	catch(string error){
	  cerr << "error reading image " << ifile << ": " << endl;
	  throw;
	}
      }
      for(int ib=0;ib<ncol;++ib){
        imgWriter.image2geo(ib,irow,x,y);
	//check mask first
	bool valid=true;
	if(mask_opt.size()){
	  //read mask
	  double colMask=0;
	  double rowMask=0;

	  maskReader.geo2image(x,y,colMask,rowMask);
	  colMask=static_cast<int>(colMask);
	  rowMask=static_cast<int>(rowMask);
	  if(rowMask>=0&&rowMask<maskReader.nrOfRow()&&colMask>=0&&colMask<maskReader.nrOfCol()){
	    if(static_cast<int>(rowMask)!=static_cast<int>(oldRowMask)){

	      assert(rowMask>=0&&rowMask<maskReader.nrOfRow());
	      try{
		maskReader.readData(lineMask,static_cast<int>(rowMask),mskband_opt[0]);
	      }
	      catch(string errorstring){
		cerr << errorstring << endl;
		exit(1);
	      }
	      catch(...){
		cerr << "error caught" << std::endl;
		exit(3);
	      }
	      oldRowMask=rowMask;
	    }
	    if(lineMask[colMask]==msknodata_opt[0])
	      valid=false;
	  }
	}

	if(!valid)
	  continue;

        //lookup corresponding row for irow in this file
        imgReader[ifile].geo2image(x,y,readCol,readRow);
        if(readCol<0||readCol>=imgReader[ifile].nrOfCol())
          continue;
        double val_current=0;
        double val_new=0;
        bool readValid=true;
        switch(theResample){
        case(BILINEAR):
          lowerCol=readCol-0.5;
          lowerCol=static_cast<int>(lowerCol);
          upperCol=readCol+0.5;
          upperCol=static_cast<int>(upperCol);
          if(lowerCol<0)
            lowerCol=0;
          if(upperCol>=imgReader[ifile].nrOfCol())
            upperCol=imgReader[ifile].nrOfCol()-1;
          for(int vband=0;vband<bndnodata_opt.size();++vband){
            val_new=(readCol-0.5-lowerCol)*readBuffer[ifile][bndnodata_opt[vband]][upperCol-startCol]+(1-readCol+0.5+lowerCol)*readBuffer[ifile][bndnodata_opt[vband]][lowerCol-startCol];
	    if(minValue_opt.size()>vband){
	      if(val_new<=minValue_opt[vband]){
		readValid=false;
		break;
	      }
	    }
	    if(maxValue_opt.size()>vband){
	      if(val_new>=maxValue_opt[vband]){
		readValid=false;
		break;
	      }
	    }
	    if(srcnodata_opt.size()>vband){
	      if(val_new==srcnodata_opt[vband]){
		readValid=false;
		break;
	      }
	    }
	  }
          break;
        default:
          readCol=static_cast<int>(readCol);
          for(int vband=0;vband<bndnodata_opt.size();++vband){
            val_new=readBuffer[ifile][bndnodata_opt[vband]][readCol-startCol];
	    if(minValue_opt.size()>vband){
	      if(val_new<=minValue_opt[vband]){
		readValid=false;
		break;
	      }
	    }
	    if(maxValue_opt.size()>vband){
	      if(val_new>=maxValue_opt[vband]){
		readValid=false;
		break;
	      }
	    }
	    if(srcnodata_opt.size()>vband){
	      if(val_new==srcnodata_opt[vband]){
		readValid=false;
		break;
	      }
	    }
	  }
          break;
	}
	if(readValid){
	  if(file_opt[0]==1)
	    ++fileBuffer[ib];
          if(writeValid[ib]){
            int iband=0;
	    switch(cruleMap[crule_opt[0]]){
	    case(crule::maxndvi):{//max ndvi
              double red_current=writeBuffer[ruleBand_opt[0]][ib];
              double nir_current=writeBuffer[ruleBand_opt[1]][ib];
	      double ndvi_current=0;
              if(red_current+nir_current>0&&red_current>=0&&nir_current>=0)
                ndvi_current=(nir_current-red_current)/(nir_current+red_current);
	      double ndvi_new=0;
              double red_new=0;
              double nir_new=0;
              switch(theResample){
              case(BILINEAR):
                lowerCol=readCol-0.5;
                lowerCol=static_cast<int>(lowerCol);
                upperCol=readCol+0.5;
                upperCol=static_cast<int>(upperCol);
                if(lowerCol<0)
                  lowerCol=0;
                if(upperCol>=imgReader[ifile].nrOfCol())
                  upperCol=imgReader[ifile].nrOfCol()-1;
                red_new=(readCol-0.5-lowerCol)*readBuffer[ifile][ruleBand_opt[0]][upperCol-startCol]+(1-readCol+0.5+lowerCol)*readBuffer[ifile][ruleBand_opt[0]][lowerCol-startCol];
                nir_new=(readCol-0.5-lowerCol)*readBuffer[ifile][ruleBand_opt[1]][upperCol-startCol]+(1-readCol+0.5+lowerCol)*readBuffer[ifile][ruleBand_opt[1]][lowerCol-startCol];
                if(red_new+nir_new>0&&red_new>=0&&nir_new>=0)
                  ndvi_new=(nir_new-red_new)/(nir_new+red_new);
                if(ndvi_new>=ndvi_current){
                  for(iband=0;iband<nband;++iband){
                    val_new=(readCol-0.5-lowerCol)*readBuffer[ifile][iband][upperCol-startCol]+(1-readCol+0.5+lowerCol)*readBuffer[ifile][iband][lowerCol-startCol];
                    writeBuffer[iband][ib]=val_new;
                  }
		  if(file_opt[0]>1)
		    fileBuffer[ib]=ifile;
                }
                break;
              default:
                readCol=static_cast<int>(readCol);
                red_new=readBuffer[ifile][ruleBand_opt[0]][readCol-startCol];
                nir_new=readBuffer[ifile][ruleBand_opt[1]][readCol-startCol];
                if(red_new+nir_new>0&&red_new>=0&&nir_new>=0)
                  ndvi_new=(nir_new-red_new)/(nir_new+red_new);
                if(ndvi_new>=ndvi_current){
                  for(iband=0;iband<nband;++iband){
                    val_new=readBuffer[ifile][iband][readCol-startCol];
                    writeBuffer[iband][ib]=val_new;
                  }
		  if(file_opt[0]>1)
		    fileBuffer[ib]=ifile;
                }
                break;
              }
	      break;
            }
	    case(crule::maxband):
            case(crule::minband):
            case(crule::validband)://max,min,valid band
              val_current=writeBuffer[ruleBand_opt[0]][ib];
              switch(theResample){
              case(BILINEAR):
                lowerCol=readCol-0.5;
                lowerCol=static_cast<int>(lowerCol);
                upperCol=readCol+0.5;
                upperCol=static_cast<int>(upperCol);
                if(lowerCol<0)
                  lowerCol=0;
                if(upperCol>=imgReader[ifile].nrOfCol())
                  upperCol=imgReader[ifile].nrOfCol()-1;
                val_new=(readCol-0.5-lowerCol)*readBuffer[ifile][ruleBand_opt[0]][upperCol-startCol]+(1-readCol+0.5+lowerCol)*readBuffer[ifile][ruleBand_opt[0]][lowerCol-startCol];
                val_new*=weight_opt[ifile];
                if((cruleMap[crule_opt[0]]==crule::maxband&&val_new>val_current)||(cruleMap[crule_opt[0]]==crule::minband&&val_new<val_current)||(cruleMap[crule_opt[0]]==crule::validband)){//&&val_new>minValue_opt[0]&&val_new<maxValue_opt[0])){
                  for(iband=0;iband<nband;++iband){
                    val_new=(readCol-0.5-lowerCol)*readBuffer[ifile][iband][upperCol-startCol]+(1-readCol+0.5+lowerCol)*readBuffer[ifile][iband][lowerCol-startCol];
                    val_new*=weight_opt[ifile];
                    writeBuffer[iband][ib]=val_new;
                  }
		  if(file_opt[0]>1)
		    fileBuffer[ib]=ifile;
                }
                break;
              default:
                readCol=static_cast<int>(readCol);
                val_new=readBuffer[ifile][ruleBand_opt[0]][readCol-startCol];
                val_new*=weight_opt[ifile];
                if((cruleMap[crule_opt[0]]==crule::maxband&&val_new>val_current)||(cruleMap[crule_opt[0]]==crule::minband&&val_new<val_current)||(cruleMap[crule_opt[0]]==crule::validband)){//&&val_new>minValue_opt[0]&&val_new<maxValue_opt[0])){
                  for(iband=0;iband<nband;++iband){
                    val_new=readBuffer[ifile][iband][readCol-startCol];
                    val_new*=weight_opt[ifile];
                    writeBuffer[iband][ib]=val_new;
                  }
		  if(file_opt[0]>1)
		    fileBuffer[ib]=ifile;
                }
                break;
              }
	      break;
            case(crule::mode)://max voting (only for Byte images)
              switch(theResample){
              case(BILINEAR):
                lowerCol=readCol-0.5;
                lowerCol=static_cast<int>(lowerCol);
                upperCol=readCol+0.5;
                upperCol=static_cast<int>(upperCol);
                if(lowerCol<0)
                  lowerCol=0;
                if(upperCol>=imgReader[ifile].nrOfCol())
                  upperCol=imgReader[ifile].nrOfCol()-1;
                for(iband=0;iband<nband;++iband){
                  val_new=(readCol-0.5-lowerCol)*readBuffer[ifile][iband][upperCol-startCol]+(1-readCol+0.5+lowerCol)*readBuffer[ifile][iband][lowerCol-startCol];
		  maxBuffer[ib][val_new]=maxBuffer[ib][val_new]+weight_opt[ifile];
                  // ++(maxBuffer[ib][val_new]);
                }
                break;
              default:
                readCol=static_cast<int>(readCol);
                for(iband=0;iband<nband;++iband){
                  val_new=readBuffer[ifile][iband][readCol-startCol];
		  maxBuffer[ib][val_new]=maxBuffer[ib][val_new]+weight_opt[ifile];
		}
                break;
	      }
              break;
            case(crule::mean)://mean value
	    case(crule::median)://median value
	    case(crule::sum)://sum value
	    case(crule::minallbands)://minimum for each and every band
	    case(crule::maxallbands)://maximum for each and every band
	    case(crule::stdev)://maximum for each and every band
              switch(theResample){
              case(BILINEAR):
                lowerCol=readCol-0.5;
                lowerCol=static_cast<int>(lowerCol);
                upperCol=readCol+0.5;
                upperCol=static_cast<int>(upperCol);
                if(lowerCol<0)
                  lowerCol=0;
                if(upperCol>=imgReader[ifile].nrOfCol())
                  upperCol=imgReader[ifile].nrOfCol()-1;
                for(iband=0;iband<nband;++iband){
                  val_new=(readCol-0.5-lowerCol)*readBuffer[ifile][iband][upperCol-startCol]+(1-readCol+0.5+lowerCol)*readBuffer[ifile][iband][lowerCol-startCol];
                  val_new*=weight_opt[ifile];
                  storeBuffer[iband][ib].push_back(val_new);
                }
                break;
              default:
                readCol=static_cast<int>(readCol);
                for(iband=0;iband<nband;++iband){
                  val_new=readBuffer[ifile][iband][readCol-startCol];
                  val_new*=weight_opt[ifile];
                  storeBuffer[iband][ib].push_back(val_new);
                  assert(ifile>0);
                  // assert(weight_opt[ifile]>=0);
                  // assert(storeBuffer[iband][ib].back()>=0);
                }
                break;
              }
	    if(file_opt[0]>1)
	      fileBuffer[ib]=ifile;
	      break;
	    case(crule::overwrite):
	    default:
              switch(theResample){
              case(BILINEAR):
                lowerCol=readCol-0.5;
                lowerCol=static_cast<int>(lowerCol);
                upperCol=readCol+0.5;
                upperCol=static_cast<int>(upperCol);
                if(lowerCol<0)
                  lowerCol=0;
                if(upperCol>=imgReader[ifile].nrOfCol())
                  upperCol=imgReader[ifile].nrOfCol()-1;
                for(iband=0;iband<nband;++iband){
                  val_new=(readCol-0.5-lowerCol)*readBuffer[ifile][iband][upperCol-startCol]+(1-readCol+0.5+lowerCol)*readBuffer[ifile][iband][lowerCol-startCol];
                  val_new*=weight_opt[ifile];
                  writeBuffer[iband][ib]=val_new;
                }
                break;
              default:
                readCol=static_cast<int>(readCol);
                for(iband=0;iband<nband;++iband){
                  val_new=readBuffer[ifile][iband][readCol-startCol];
                  val_new*=weight_opt[ifile];
                  writeBuffer[iband][ib]=val_new;
                }
                break;
              }
	    if(file_opt[0]>1)
	      fileBuffer[ib]=ifile;
            break;
	    }
	  }
	  else{
            writeValid[ib]=true;//readValid was true
            int iband=0;
	    switch(cruleMap[crule_opt[0]]){
            case(crule::mean):
            case(crule::median):
            case(crule::sum):
            case(crule::minallbands):
            case(crule::maxallbands):
            case(crule::stdev):
              switch(theResample){
              case(BILINEAR):
                lowerCol=readCol-0.5;
                lowerCol=static_cast<int>(lowerCol);
                upperCol=readCol+0.5;
                upperCol=static_cast<int>(upperCol);
                if(lowerCol<0)
                  lowerCol=0;
                if(upperCol>=imgReader[ifile].nrOfCol())
                  upperCol=imgReader[ifile].nrOfCol()-1;
                for(iband=0;iband<nband;++iband){
                  val_new=(readCol-0.5-lowerCol)*readBuffer[ifile][iband][upperCol-startCol]+(1-readCol+0.5+lowerCol)*readBuffer[ifile][iband][lowerCol-startCol];
                  val_new*=weight_opt[ifile];
                  storeBuffer[iband][ib].push_back(val_new);
                }
                break;
              default:
                readCol=static_cast<int>(readCol);
                for(iband=0;iband<nband;++iband){
                  val_new=readBuffer[ifile][iband][readCol-startCol];
                  val_new*=weight_opt[ifile];
                  storeBuffer[iband][ib].push_back(val_new);
                }
                break;
              }
	    if(file_opt[0]>1)
	      fileBuffer[ib]=ifile;
	    break;
            case(crule::mode):
              switch(theResample){
              case(BILINEAR):
                lowerCol=readCol-0.5;
                lowerCol=static_cast<int>(lowerCol);
                upperCol=readCol+0.5;
                upperCol=static_cast<int>(upperCol);
                if(lowerCol<0)
                  lowerCol=0;
                if(upperCol>=imgReader[ifile].nrOfCol())
                  upperCol=imgReader[ifile].nrOfCol()-1;
                for(iband=0;iband<nband;++iband){
                  val_new=(readCol-0.5-lowerCol)*readBuffer[ifile][iband][upperCol-startCol]+(1-readCol+0.5+lowerCol)*readBuffer[ifile][iband][lowerCol-startCol];
		  maxBuffer[ib][val_new]=maxBuffer[ib][val_new]+weight_opt[ifile];
                  // ++(maxBuffer[ib][val_new]);
		}
                break;
              default:
                readCol=static_cast<int>(readCol);
                for(iband=0;iband<nband;++iband){
		  val_new=readBuffer[ifile][iband][readCol-startCol];
		  maxBuffer[ib][val_new]=maxBuffer[ib][val_new]+weight_opt[ifile];
		}
                  // ++(maxBuffer[ib][val_new]);
                break;
              }
              break;
            default:
              switch(theResample){
              case(BILINEAR):
                lowerCol=readCol-0.5;
                lowerCol=static_cast<int>(lowerCol);
                upperCol=readCol+0.5;
                upperCol=static_cast<int>(upperCol);
                if(lowerCol<0)
                  lowerCol=0;
                if(upperCol>=imgReader[ifile].nrOfCol())
                  upperCol=imgReader[ifile].nrOfCol()-1;
                for(iband=0;iband<nband;++iband){
                  val_new=(readCol-0.5-lowerCol)*readBuffer[ifile][iband][upperCol-startCol]+(1-readCol+0.5+lowerCol)*readBuffer[ifile][iband][lowerCol-startCol];
                  val_new*=weight_opt[ifile];
                  writeBuffer[iband][ib]=val_new;
                }
                break;
              default:
                readCol=static_cast<int>(readCol);
                for(iband=0;iband<nband;++iband){
                  val_new=readBuffer[ifile][iband][readCol-startCol];
                  val_new*=weight_opt[ifile];
                  writeBuffer[iband][ib]=val_new;
                }
                break;
              }
	      if(file_opt[0]>1)
		fileBuffer[ib]=ifile;
              break;
            }
          }
        }
      }
      // imgReader.close();
    }
    if(cruleMap[crule_opt[0]]==crule::mode){
      vector<short> classBuffer(imgWriter.nrOfCol());
      if(class_opt.size()>1){
        for(int iclass=0;iclass<class_opt.size();++iclass){
          for(int icol=0;icol<imgWriter.nrOfCol();++icol)
            classBuffer[icol]=maxBuffer[icol][class_opt[iclass]];
          try{
            imgWriter.writeData(classBuffer,irow,iclass);
          }
          catch(string error){
            cerr << "error writing image file: " << error << endl;
            throw;
          }
        }
      }
      else{
        for(int icol=0;icol<imgWriter.nrOfCol();++icol){
          vector<short>::iterator maxit=maxBuffer[icol].begin();
          maxit=stat.mymax(maxBuffer[icol],maxBuffer[icol].begin(),maxBuffer[icol].end());
          writeBuffer[0][icol]=distance(maxBuffer[icol].begin(),maxit);
	  if(file_opt[0]>1)
	    fileBuffer[icol]=*(maxit);
        }
        try{
          imgWriter.writeData(writeBuffer[0],irow,0);
          if(file_opt[0])
            imgWriter.writeData(fileBuffer,irow,1);
        }
        catch(string error){
          cerr << "error writing image file: " << error << endl;
          throw;
        }
      }
    }
    else{
      for(int iband=0;iband<bands.size();++iband){
        // assert(writeBuffer[bands[iband]].size()==imgWriter.nrOfCol());
        assert(writeBuffer[iband].size()==imgWriter.nrOfCol());
        for(int icol=0;icol<imgWriter.nrOfCol();++icol){
	  try{
	    switch(cruleMap[crule_opt[0]]){
	    case(crule::mean):
	      // writeBuffer[iband][icol]=stat.mean(storeBuffer[bands[iband]][icol]);
	      writeBuffer[iband][icol]=stat.mean(storeBuffer[iband][icol]);
	      break;
	    case(crule::median):
	      // writeBuffer[iband][icol]=stat.median(storeBuffer[bands[iband]][icol]);
	      writeBuffer[iband][icol]=stat.median(storeBuffer[iband][icol]);
	      break;
	    case(crule::sum):
	      // writeBuffer[iband][icol]=stat.sum(storeBuffer[bands[iband]][icol]);
	      writeBuffer[iband][icol]=stat.sum(storeBuffer[iband][icol]);
	      break;
	    case(crule::minallbands):
	      // writeBuffer[iband][icol]=stat.mymin(storeBuffer[bands[iband]][icol]);
	      writeBuffer[iband][icol]=stat.mymin(storeBuffer[iband][icol]);
	      break;
	    case(crule::maxallbands):
	      // writeBuffer[iband][icol]=stat.mymax(storeBuffer[bands[iband]][icol]);
	      writeBuffer[iband][icol]=stat.mymax(storeBuffer[iband][icol]);
	      break;
	    case(crule::stdev):
	      // writeBuffer[iband][icol]=sqrt(stat.var(storeBuffer[bands[iband]][icol]));
	      writeBuffer[iband][icol]=sqrt(stat.var(storeBuffer[iband][icol]));
	      break;
	    default:
	      break;
	    }
	  }
	  catch(string error){
	    if(verbose_opt[0])
	      cerr << error << endl;
	    writeBuffer[iband][icol]=dstnodata_opt[0];
	    continue;
	  }
        }
        try{
          imgWriter.writeData(writeBuffer[iband],irow,iband);
        }
        catch(string error){
          cerr << error << endl;
          throw;
        }
      }
      if(file_opt[0]){
        try{
          imgWriter.writeData(fileBuffer,irow,bands.size());
        }
        catch(string error){
          cerr << error << endl;
          throw;
        }
      }
    }
    progress=static_cast<float>(irow+1.0)/imgWriter.nrOfRow();
    pfnProgress(progress,pszMessage,pProgressArg);
  }
  if(extent_opt.size()&&(cut_opt[0]||eoption_opt.size())){
    extentReader.close();
  }
  if(mask_opt.size())
    maskReader.close();
}