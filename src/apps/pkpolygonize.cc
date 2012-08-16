/**********************************************************************
pkpolygonize.cc: program to make vector file from raster image
Copyright (C) 2008-2012 Pieter Kempeneers

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
#include "imageclasses/ImgReaderGdal.h"
#include "imageclasses/ImgWriterGdal.h"
#include "imageclasses/ImgWriterOgr.h"
#include "base/Optionpk.h"
#include "ogrsf_frmts.h"
extern "C" {
#include "gdal_alg.h"
#include "ogr_api.h"
}

using namespace std;

int main(int argc,char **argv) {
  Optionpk<bool> version_opt("\0","version","version 20120625, Copyright (C) 2008-2012 Pieter Kempeneers.\n\
   This program comes with ABSOLUTELY NO WARRANTY; for details type use option -h.\n\
   This is free software, and you are welcome to redistribute it\n\
   under certain conditions; use option --license for details.",false);
  Optionpk<bool> license_opt("lic","license","show license information",false);
  Optionpk<bool> help_opt("h","help","shows this help info",false);
  Optionpk<string> input_opt("i", "input", "Input image file (WARNING: will be overwritten with output!", "");
  Optionpk<string> mask_opt("m", "mask", "All pixels in the mask band with a value other than zero will be considered suitable for collection as polygons. Use input file as mask to remove background polygon! ", "");
  Optionpk<string> output_opt("o", "output", "Output vector file", "");
  Optionpk<int> band_opt("b", "band", "the band to be used from input file (default is 0)", 0);
  Optionpk<string> fname_opt("n", "name", "the field name of the output layer (default is DN)", "DN");
  Optionpk<short> verbose_opt("v", "verbose", "verbose", 0);

  version_opt.retrieveOption(argc,argv);
  license_opt.retrieveOption(argc,argv);
  help_opt.retrieveOption(argc,argv);

  if(version_opt[0]){
    cout << version_opt.getHelp() << endl;
    exit(0);
  }
  if(license_opt[0]){
    cout << Optionpk<bool>::getGPLv3License() << endl;
    exit(0);
  }

  input_opt.retrieveOption(argc,argv);
  mask_opt.retrieveOption(argc,argv);
  output_opt.retrieveOption(argc,argv);
  band_opt.retrieveOption(argc,argv);
  fname_opt.retrieveOption(argc,argv);
  verbose_opt.retrieveOption(argc,argv);

  GDALAllRegister();

  double dfComplete=0.0;
  const char* pszMessage;
  void* pProgressArg=NULL;
  GDALProgressFunc pfnProgress=GDALTermProgress;
  pfnProgress(dfComplete,pszMessage,pProgressArg);
  

  ImgReaderGdal maskReader;
  GDALRasterBand *maskBand=NULL;
  if(mask_opt[0]!=""){
    if(verbose_opt[0])
      cout << "opening mask file " << mask_opt[0] << endl;
    maskReader.open(mask_opt[0]);
    maskBand = maskReader.getRasterBand(0);
  }

  ImgReaderGdal inputReader(input_opt[0]);
  GDALRasterBand  *inputBand;
  inputBand=inputReader.getRasterBand(0);

  ImgWriterOgr ogrWriter(output_opt[0]);
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
  if(mask_opt[0]!="")
    maskReader.close();
  ogrWriter.close();
}

