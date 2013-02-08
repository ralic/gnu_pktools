/**********************************************************************
pkeditogr.cc: program to edit (rename fields) ogr fil
Copyright (C) 2008-2013 Pieter Kempeneers

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
#include <assert.h>
#include <map>
#include "base/Optionpk.h"
#include "imageclasses/ImgReaderOgr.h"
#include "imageclasses/ImgWriterOgr.h"
#include "imageclasses/ImgReaderGdal.h"
#include "imageclasses/ImgWriterGdal.h"

int main(int argc, char *argv[])
{
  Optionpk<string> input_opt("i", "input", "Input image");
  Optionpk<string> output_opt("o", "output", "Output mask file");
  Optionpk<string> field_opt("f", "field", "output field names (number must exactly match input fields)");
  Optionpk<string> addname_opt("an", "an", "name(s) of field(s) to add (number must exactly match field types)");
  Optionpk<string> addtype_opt("at", "at", "type(s) of field(s) to add (number must exactly match fieldnames to add", "Real");
  Optionpk<string> default_opt("d", "default", "default value(s) for new field(s)");
  Optionpk<short> verbose_opt("v", "verbose", "verbose", 0);

  bool doProcess;//stop process when program was invoked with help option (-h --help)
  try{
    doProcess=input_opt.retrieveOption(argc,argv);
    output_opt.retrieveOption(argc,argv);
    field_opt.retrieveOption(argc,argv);
    addname_opt.retrieveOption(argc,argv);
    addtype_opt.retrieveOption(argc,argv);
    default_opt.retrieveOption(argc,argv);
    verbose_opt.retrieveOption(argc,argv);
  }
  catch(string predefinedString){
    std::cout << predefinedString << std::endl;
    exit(0);
  }
  if(!doProcess){
    std::cout << "short option -h shows basic options only, use long option --help to show all options" << std::endl;
    exit(0);//help was invoked, stop processing
  }
  if(verbose_opt[0])
    cout << "opening " << input_opt[0] << " for reading " << endl;
  ImgReaderOgr ogrReader(input_opt[0]);
  if(verbose_opt[0])
    cout << "opening " << output_opt[0] << " for writing " << endl;

  OGRFieldType fieldType[addtype_opt.size()];
  int ogr_typecount=11;//hard coded for now!
  for(int it=0;it<addtype_opt.size();++it){
    for(int iType = 0; iType < ogr_typecount; ++iType){
      if( OGRFieldDefn::GetFieldTypeName((OGRFieldType)iType) != NULL
          && EQUAL(OGRFieldDefn::GetFieldTypeName((OGRFieldType)iType),
                   addtype_opt[it].c_str()))
        fieldType[it]=(OGRFieldType) iType;
    }
    if(verbose_opt[0]>1)
      std::cout << std::endl << "field type is: " << OGRFieldDefn::GetFieldTypeName(fieldType[it]) << std::endl;
  }

  //start reading features from the layer
  if(verbose_opt[0])
    cout << "reset reading" << endl;
  ogrReader.getLayer()->ResetReading();
  if(field_opt.size())
    assert(field_opt.size()==ogrReader.getFieldCount());
  unsigned long int ifeature=0;
  if(verbose_opt[0])
    cout << "going through features" << endl << flush;

  ImgWriterOgr ogrWriter(output_opt[0]);
  OGRLayer* writeLayer=ogrWriter.createLayer(output_opt[0],ogrReader.getProjection(),ogrReader.getGeometryType(),NULL);
  std::vector<OGRFieldDefn*> readFields;
  std::vector<OGRFieldDefn*> writeFields;
  ogrReader.getFields(readFields);
  writeFields=readFields;
  try{
    for(int ifield=0;ifield<readFields.size();++ifield){
      if(field_opt.size()>ifield)
        writeFields[ifield]->SetName(field_opt[ifield].c_str());
      if(verbose_opt[0])
        std::cout << readFields[ifield]->GetNameRef() << " -> " << writeFields[ifield]->GetNameRef() << std::endl;
      if(writeLayer->CreateField(writeFields[ifield]) != OGRERR_NONE ){
        ostringstream es;
        if(field_opt.size()>ifield)
          es << "Creating field " << field_opt[ifield] << " failed";
        else
          es << "Creating field " << readFields[ifield] << " failed";
        string errorString=es.str();
        throw(errorString);
      }
    }
  }
  catch(string errorString){
    std::cerr << errorString << std::endl;
    exit(1);
  }
  if(verbose_opt[0])
    std::cout << "add " << addname_opt.size() << " fields" << std::endl;
  if(addname_opt.size()){
    assert(addname_opt.size()==addtype_opt.size());
    while(default_opt.size()<addname_opt.size())
      default_opt.push_back(default_opt.back());
  }
  for(int iname=0;iname<addname_opt.size();++iname){
    if(verbose_opt[0])
      std::cout << addname_opt[iname] << " " << std::endl;
    ogrWriter.createField(addname_opt[iname],fieldType[iname]);
  }
  if(verbose_opt[0]){
    std::cout << std::endl;
    std::cout << addname_opt.size() << " fields created" << std::endl;
  }
  OGRFeature *poFeature;
  // while(true){// (poFeature = imgReaderOgr.getLayer()->GetNextFeature()) != NULL ){
  while((poFeature = ogrReader.getLayer()->GetNextFeature()) != NULL ){
    ++ifeature;
    // poFeature = ogrReader.getLayer()->GetNextFeature();
    // if( poFeature == NULL )
    //   break;
    OGRFeature *poDstFeature = NULL;
    poDstFeature=ogrWriter.createFeature();
    if( poDstFeature->SetFrom( poFeature, TRUE ) != OGRERR_NONE ){
      const char* fmt;
      string errorString="Unable to translate feature %d from layer %s.\n";
      fmt=errorString.c_str();
      CPLError( CE_Failure, CPLE_AppDefined,
                fmt,
                poFeature->GetFID(), ogrWriter.getLayerName().c_str() );
      OGRFeature::DestroyFeature( poFeature );
      OGRFeature::DestroyFeature( poDstFeature );
    }
    poDstFeature->SetFID( poFeature->GetFID() );
    //set default values for new fields
    if(verbose_opt[0])
      std::cout << "set default values for new fields in feature " << ifeature << std::endl;
    for(int iname=0;iname<addname_opt.size();++iname){
      switch(fieldType[iname]){
      case(OFTReal):
        if(verbose_opt[0])
          std::cout << "set field " << addname_opt[iname] << " to default " << string2type<float>(default_opt[iname]) << std::endl;
        poDstFeature->SetField(addname_opt[iname].c_str(),string2type<float>(default_opt[iname]));
        break;
      case(OFTInteger):
        if(verbose_opt[0])
          std::cout << "set field " << addname_opt[iname] << " to default " << string2type<int>(default_opt[iname]) << std::endl;
        poDstFeature->SetField(addname_opt[iname].c_str(),string2type<int>(default_opt[iname]));
        break;
      case(OFTString):
        if(verbose_opt[0])
          std::cout << "set field " << addname_opt[iname] << " to default " << default_opt[iname] << std::endl;
        poDstFeature->SetField(addname_opt[iname].c_str(),default_opt[iname].c_str());
        break;
      default:
        break;
      }
      OGRFeature::DestroyFeature( poFeature );
    }
    CPLErrorReset();
    if(verbose_opt[0])
      std::cout << "create feature" << std::endl;
    if(ogrWriter.createFeature( poDstFeature ) != OGRERR_NONE){
      const char* fmt;
      string errorString="Unable to translate feature %d from layer %s.\n";
      fmt=errorString.c_str();
      CPLError( CE_Failure, CPLE_AppDefined,
                fmt,
                poFeature->GetFID(), ogrWriter.getLayerName().c_str() );
      OGRFeature::DestroyFeature( poDstFeature );
    }
    OGRFeature::DestroyFeature( poDstFeature );
  }
  if(verbose_opt[0])
    std::cout << "replaced " << ifeature << " features" << std::endl;
  ogrReader.close();
  ogrWriter.close();
}
