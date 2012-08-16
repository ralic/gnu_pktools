/**********************************************************************
Filter2d.cc: class for filtering images
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
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cmath>
#include "Filter2d.h"
#include "Histogram.h"
// #include "imageclasses/ImgUtils.h"

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif

#include <math.h>

#ifndef DEG2RAD
#define DEG2RAD(DEG) ((DEG)*((PI)/(180.0)))
#endif

Filter2d::Filter2d::Filter2d(void)
  : m_noValue(0)
{
}

Filter2d::Filter2d::Filter2d(const Vector2d<double> &taps)
  : m_taps(taps), m_noValue(0)
{
}

void Filter2d::Filter2d::setTaps(const Vector2d<double> &taps)
{
  m_taps=taps;
}

void Filter2d::Filter2d::smoothNoData(const ImgReaderGdal& input, ImgWriterGdal& output, int dim)
{
  smoothNoData(input, output,dim,dim);
}

void Filter2d::Filter2d::smooth(const ImgReaderGdal& input, ImgWriterGdal& output, int dim)
{
  smooth(input, output,dim,dim);
}

void Filter2d::Filter2d::smoothNoData(const ImgReaderGdal& input, ImgWriterGdal& output, int dimX, int dimY)
{
  m_taps.resize(dimY);
  for(int j=0;j<dimY;++j){
    m_taps[j].resize(dimX);
    for(int i=0;i<dimX;++i)
      m_taps[j][i]=1.0;
  }
  filter(input,output,false,true,true);
}

void Filter2d::Filter2d::smooth(const ImgReaderGdal& input, ImgWriterGdal& output, int dimX, int dimY)
{
  m_taps.resize(dimY);
  for(int j=0;j<dimY;++j){
    m_taps[j].resize(dimX);
    for(int i=0;i<dimX;++i)
      m_taps[j][i]=1.0;
  }
  filter(input,output,false,true,false);
}

    
void Filter2d::Filter2d::filter(const ImgReaderGdal& input, ImgWriterGdal& output, bool absolute, bool normalize, bool noData)
{
  int dimX=m_taps[0].size();//horizontal!!!
  int dimY=m_taps.size();//vertical!!!
  //  byte* tmpbuf=new byte[input.rowSize()];
  const char* pszMessage;
  void* pProgressArg=NULL;
  GDALProgressFunc pfnProgress=GDALTermProgress;
  double progress=0;
  pfnProgress(progress,pszMessage,pProgressArg);
  for(int iband=0;iband<input.nrOfBand();++iband){
    Vector2d<double> inBuffer(dimY);
    vector<double> outBuffer(input.nrOfCol());
    int indexI=0;
    int indexJ=0;
    //initialize last half of inBuffer
    for(int y=0;y<dimY;++y){
      inBuffer[y].resize(input.nrOfCol());
      if(y<dimY/2)
	continue;//skip first half
      try{
	input.readData(inBuffer[y],GDT_Float64,indexJ,iband);
	++indexJ;
      }
      catch(string errorstring){
	cerr << errorstring << "in band " << iband << ", line " << indexJ << endl;
      }
    }
    for(int y=0;y<input.nrOfRow();++y){
      if(y){//inBuffer already initialized for y=0
	//erase first line from inBuffer
	inBuffer.erase(inBuffer.begin());
	//read extra line and push back to inBuffer if not out of bounds
	if(y+dimY/2<input.nrOfRow()){
	  //allocate buffer
	  inBuffer.push_back(inBuffer.back());
	  try{
            input.readData(inBuffer[inBuffer.size()-1],GDT_Float64,y+dimY/2,iband);
	  }
	  catch(string errorstring){
	    cerr << errorstring << "in band " << iband << ", line " << y << endl;
	  }
	}
      }
      for(int x=0;x<input.nrOfCol();++x){
	outBuffer[x]=0;
        double norm=0;
        bool masked=false;
        if(noData){//only filter noData values
          for(int imask=0;imask<m_mask.size();++imask){
            if(inBuffer[dimY/2][x]==m_mask[imask]){
              masked=true;
              break;
            }
          }
          if(!masked){
            outBuffer[x]=inBuffer[dimY/2][x];
            continue;
          }
        }
        assert(!noData||masked);
	for(int j=-dimY/2;j<(dimY+1)/2;++j){
	  for(int i=-dimX/2;i<(dimX+1)/2;++i){
	    indexI=x+i;
	    indexJ=dimY/2+j;
	    //check if out of bounds
	    if(x<dimX/2)
	      indexI=x+abs(i);
	    else if(x>=input.nrOfCol()-dimX/2)
	      indexI=x-abs(i);
	    if(y<dimY/2)
	      indexJ=dimY/2+abs(j);
	    else if(y>=input.nrOfRow()-dimY/2)
	      indexJ=dimY/2-abs(j);
            //do not take masked values into account
            masked=false;
	    for(int imask=0;imask<m_mask.size();++imask){
	      if(inBuffer[indexJ][indexI]==m_mask[imask]){
		masked=true;
		break;
	      }
	    }
	    if(!masked){
              outBuffer[x]+=(m_taps[dimY/2+j][dimX/2+i]*inBuffer[indexJ][indexI]);
              norm+=m_taps[dimY/2+j][dimX/2+i];
            }
	  }
        }
        if(absolute)
          outBuffer[x]=(normalize)? abs(outBuffer[x])/norm : abs(outBuffer[x]);
        else if(normalize&&norm!=0)
          outBuffer[x]=outBuffer[x]/norm;
      }
      //write outBuffer to file
      try{
        output.writeData(outBuffer,GDT_Float64,y,iband);
      }
      catch(string errorstring){
	    cerr << errorstring << "in band " << iband << ", line " << y << endl;
      }
      progress=(1.0+y);
      progress+=(output.nrOfRow()*iband);
      progress/=output.nrOfBand()*output.nrOfRow();
      pfnProgress(progress,pszMessage,pProgressArg);
    }
  }
}


void Filter2d::Filter2d::majorVoting(const string& inputFilename, const string& outputFilename,int dim,const vector<int> &prior)
{
  bool usePriors=true;  
  if(prior.empty()){
    cout << "no prior information" << endl;
    usePriors=false;
  }
  else{
    cout << "using priors ";    
    for(int iclass=0;iclass<prior.size();++iclass)
      cout << " " << static_cast<short>(prior[iclass]);
    cout << endl;    
  }  
  ImgReaderGdal input;
  ImgWriterGdal output;
  input.open(inputFilename);
  output.open(outputFilename,input);
  int dimX=0;//horizontal!!!
  int dimY=0;//vertical!!!
  if(dim){
    dimX=dim;
    dimY=dim;
  }
  else{
    dimX=m_taps[0].size();
    dimY=m_taps.size();
  }
  Vector2d<double> inBuffer(dimY);
  vector<double> outBuffer(input.nrOfCol());
  //initialize last half of inBuffer
  int indexI=0;
  int indexJ=0;
//   byte* tmpbuf=new byte[input.rowSize()];
  for(int y=0;y<dimY;++y){
    inBuffer[y].resize(input.nrOfCol());
    if(y<dimY/2)
      continue;//skip first half
    try{
      input.readData(inBuffer[y],GDT_Float64,indexJ++);
    }
    catch(string errorstring){
      cerr << errorstring << "in line " << indexJ << endl;
    }
  }
  const char* pszMessage;
  void* pProgressArg=NULL;
  GDALProgressFunc pfnProgress=GDALTermProgress;
  double progress=0;
  pfnProgress(progress,pszMessage,pProgressArg);
  for(int y=0;y<input.nrOfRow();++y){
    if(y){//inBuffer already initialized for y=0
      //erase first line from inBuffer
      inBuffer.erase(inBuffer.begin());
      //read extra line and push back to inBuffer if not out of bounds
      if(y+dimY/2<input.nrOfRow()){
	//allocate buffer
	inBuffer.push_back(inBuffer.back());
	try{
          input.readData(inBuffer[inBuffer.size()-1],GDT_Float64,y+dimY/2);
	}
	catch(string errorstring){
	  cerr << errorstring << "in line" << y << endl;
	}
      }
    }
    for(int x=0;x<input.nrOfCol();++x){
      outBuffer[x]=0;
      map<int,int> occurrence;
      for(int j=-dimY/2;j<(dimY+1)/2;++j){
	for(int i=-dimX/2;i<(dimX+1)/2;++i){
	  indexI=x+i;
	  indexJ=dimY/2+j;
	  //check if out of bounds
	  if(x<dimX/2)
	    indexI=x+abs(i);
	  else if(x>=input.nrOfCol()-dimX/2)
	    indexI=x-abs(i);
	  if(y<dimY/2)
	    indexJ=dimY/2+abs(j);
	  else if(y>=input.nrOfRow()-dimY/2)
	    indexJ=dimY/2-abs(j);
	  if(usePriors){
	    occurrence[inBuffer[indexJ][indexI]]+=prior[inBuffer[indexJ][indexI]-1];
	  }	  
	  else
	    ++occurrence[inBuffer[indexJ][indexI]];
	}
      }
      map<int,int>::const_iterator maxit=occurrence.begin();
      for(map<int,int>::const_iterator mit=occurrence.begin();mit!=occurrence.end();++mit){
	if(mit->second>maxit->second)
	  maxit=mit;
      }
      if(occurrence[inBuffer[dimY/2][x]]<maxit->second)//
	outBuffer[x]=maxit->first;
      else//favorize original value in case of ties
	outBuffer[x]=inBuffer[dimY/2][x];
    }
    //write outBuffer to file
    try{
      output.writeData(outBuffer,GDT_Float64,y);
    }
    catch(string errorstring){
      cerr << errorstring << "in line" << y << endl;
    }
    progress=(1.0+y)/output.nrOfRow();
    pfnProgress(progress,pszMessage,pProgressArg);
  }
  input.close();
  output.close();
}

// void Filter2d::homogeneousSpatial(const string& inputFilename, const string& outputFilename, int dim, bool disc, int noValue)
// {
//   ImgReaderGdal input;
//   ImgWriterGdal output;
//   input.open(inputFilename);
//   output.open(outputFilename,input);
//   int dimX=0;//horizontal!!!
//   int dimY=0;//vertical!!!
//   assert(dim);
//   dimX=dim;
//   dimY=dim;
//   Vector2d<double> inBuffer(dimY);
//   vector<double> outBuffer(input.nrOfCol());
//   //initialize last half of inBuffer
//   int indexI=0;
//   int indexJ=0;
//   for(int y=0;y<dimY;++y){
//     inBuffer[y].resize(input.nrOfCol());
//     if(y<dimY/2)
//       continue;//skip first half
//     try{
//       input.readData(inBuffer[y],GDT_Float64,indexJ++);
//     }
//     catch(string errorstring){
//       cerr << errorstring << "in line " << indexJ << endl;
//     }
//   }
//   const char* pszMessage;
//   void* pProgressArg=NULL;
//   GDALProgressFunc pfnProgress=GDALTermProgress;
//   double progress=0;
//   pfnProgress(progress,pszMessage,pProgressArg);
//   for(int y=0;y<input.nrOfRow();++y){
//     if(y){//inBuffer already initialized for y=0
//       //erase first line from inBuffer
//       inBuffer.erase(inBuffer.begin());
//       //read extra line and push back to inBuffer if not out of bounds
//       if(y+dimY/2<input.nrOfRow()){
//         //allocate buffer
//         inBuffer.push_back(inBuffer.back());
//         try{
//           input.readData(inBuffer[inBuffer.size()-1],GDT_Float64,y+dimY/2);
//         }
//         catch(string errorstring){
//           cerr << errorstring << "in line " << y << endl;
//         }
//       }
//     }
//     for(int x=0;x<input.nrOfCol();++x){
//       outBuffer[x]=0;
//       map<int,int> occurrence;
//       for(int j=-dimY/2;j<(dimY+1)/2;++j){
// 	for(int i=-dimX/2;i<(dimX+1)/2;++i){
//           if(disc&&(i*i+j*j>(dim/2)*(dim/2)))
//             continue;
//           indexI=x+i;
//           //check if out of bounds
//           if(indexI<0)
//             indexI=-indexI;
//           else if(indexI>=input.nrOfCol())
//             indexI=input.nrOfCol()-indexI;
//           if(y+j<0)
//             indexJ=-j;
//           else if(y+j>=input.nrOfRow())
//             indexJ=dimY/2-j;
//           else
//             indexJ=dimY/2+j;
//           ++occurrence[inBuffer[indexJ][indexI]];
// 	}
//       }
//       if(occurrence.size()==1)//all values in window must be the same
// 	outBuffer[x]=inBuffer[dimY/2][x];
//       else//favorize original value in case of ties
// 	outBuffer[x]=noValue;
//     }
//     //write outBuffer to file
//     try{
//       output.writeData(outBuffer,GDT_Float64,y);
//     }
//     catch(string errorstring){
//       cerr << errorstring << "in line" << y << endl;
//     }
//     progress=(1.0+y)/input.nrOfRow();
//     pfnProgress(progress,pszMessage,pProgressArg);
//   }
//   input.close();
//   output.close();
// }

void Filter2d::Filter2d::median(const string& inputFilename, const string& outputFilename,int dim, bool disc)
{
  ImgReaderGdal input;
  ImgWriterGdal output;
  input.open(inputFilename);
  output.open(outputFilename,input);
  doit(input,output,MEDIAN,dim,disc);
}

void Filter2d::Filter2d::var(const string& inputFilename, const string& outputFilename,int dim, bool disc)
{
  ImgReaderGdal input;
  ImgWriterGdal output;
  input.open(inputFilename);
  output.open(outputFilename,input);
  doit(input,output,VAR,dim,disc);
}

void Filter2d::Filter2d::doit(const ImgReaderGdal& input, ImgWriterGdal& output, int method, int dim, short down, bool disc)
{
  doit(input,output,method,dim,dim,down,disc);
}

void Filter2d::Filter2d::doit(const ImgReaderGdal& input, ImgWriterGdal& output, int method, int dimX, int dimY, short down, bool disc)
{
  // ImgReaderGdal input;
  // ImgWriterGdal output;
  // input.open(inputFilename);
  // output.open(outputFilename,input);
  assert(dimX);
  assert(dimY);
  Histogram hist;
  for(int iband=0;iband<input.nrOfBand();++iband){
    Vector2d<double> inBuffer(dimY,input.nrOfCol());
    // vector<double> outBuffer(input.nrOfCol());
    vector<double> outBuffer((input.nrOfCol()+down-1)/down);
    //initialize last half of inBuffer
    int indexI=0;
    int indexJ=0;
    // for(int y=0;y<dimY;++y){
    for(int j=-dimY/2;j<(dimY+1)/2;++j){
      // if(y<dimY/2)
      //   continue;//skip first half
      try{
        input.readData(inBuffer[indexJ],GDT_Float64,abs(j),iband);
      }
      catch(string errorstring){
	cerr << errorstring << "in line " << indexJ << endl;
      }
      ++indexJ;
    }
    const char* pszMessage;
    void* pProgressArg=NULL;
    GDALProgressFunc pfnProgress=GDALTermProgress;
    double progress=0;
    pfnProgress(progress,pszMessage,pProgressArg);
    // for(int y=0;y<input.nrOfRow();++y){
    for(int y=0;y<input.nrOfRow();++y){
      if(y){//inBuffer already initialized for y=0
	//erase first line from inBuffer
	inBuffer.erase(inBuffer.begin());
	//read extra line and push back to inBuffer if not out of bounds
	if(y+dimY/2<input.nrOfRow()){
          //allocate buffer
          inBuffer.push_back(inBuffer.back());
	  try{
            input.readData(inBuffer[inBuffer.size()-1],GDT_Float64,y+dimY/2,iband);
	  }
	  catch(string errorstring){
	    cerr << errorstring << "in band " << iband << ", line " << y << endl;
	  }
	}
        else{
          int over=y+dimY/2-input.nrOfRow();
          int index=(inBuffer.size()-1)-over;
          assert(index>=0);
          assert(index<inBuffer.size());
          inBuffer.push_back(inBuffer[index]);
        }
      }
      if((y+1+down/2)%down)
        continue;
      for(int x=0;x<input.nrOfCol();++x){
        if((x+1+down/2)%down)
          continue;
	// outBuffer[x]=0;
	outBuffer[x/down]=0;
	vector<double> windowBuffer;
	map<int,int> occurrence;
	for(int j=-dimY/2;j<(dimY+1)/2;++j){
	  for(int i=-dimX/2;i<(dimX+1)/2;++i){
	    double d2=i*i+j*j;//square distance
            if(disc&&(d2>(dimX/2)*(dimY/2)))
              continue;
	    indexI=x+i;
	    //check if out of bounds
	    if(indexI<0)
	      indexI=-indexI;
	    else if(indexI>=input.nrOfCol())
	      indexI=input.nrOfCol()-i;
	      // indexI=input.nrOfCol()-indexI;
	    if(y+j<0)
	      indexJ=-j;
	    else if(y+j>=input.nrOfRow())
	      indexJ=dimY/2-j;
	    else
	      indexJ=dimY/2+j;
	    // if(method==HOMOG||method==DENSITY||method==MAJORITY||method==MIXED){
	    //   bool masked=false;
	    //   for(int imask=0;imask<m_mask.size();++imask){
	    //     if(inBuffer[indexJ][indexI]==m_mask[imask]){
	    //       masked=true;
	    //       break;
	    //     }
	    //   }
	    //   if(!masked)
	    //     ++occurrence[inBuffer[indexJ][indexI]];
	    // }
	    bool masked=false;
	    for(int imask=0;imask<m_mask.size();++imask){
	      if(inBuffer[indexJ][indexI]==m_mask[imask]){
		masked=true;
		break;
	      }
	    }
	    if(!masked){
              vector<short>::const_iterator vit=m_class.begin();
              //todo: test if this works (only add occurrence if within defined classes)!
              if(!m_class.size())
                ++occurrence[inBuffer[indexJ][indexI]];
              else{
                while(vit!=m_class.end()){
                  if(inBuffer[indexJ][indexI]==*(vit++))
                    ++occurrence[inBuffer[indexJ][indexI]];
                }
              }
              windowBuffer.push_back(inBuffer[indexJ][indexI]);
            }
	  }
        }
        switch(method){
        case(MEDIAN):
          // outBuffer[x]=hist.median(windowBuffer);
          outBuffer[x/down]=hist.median(windowBuffer);
          break;
        case(VAR):{
          // outBuffer[x]=hist.var(windowBuffer);
          outBuffer[x/down]=hist.var(windowBuffer);
          break;
        }
        case(MEAN):{
          // outBuffer[x]=hist.mean(windowBuffer);
          outBuffer[x/down]=hist.mean(windowBuffer);
          break;
        }
        case(MIN):{
          // outBuffer[x]=hist.min(windowBuffer);
          outBuffer[x/down]=hist.min(windowBuffer);
          break;
        }
        case(MINMAX):{
          double min=0;
          double max=0;
          hist.minmax(windowBuffer,windowBuffer.begin(),windowBuffer.end(),min,max);
          if(min!=max)
            outBuffer[x/down]=0;
            // outBuffer[x]=0;
          else
            outBuffer[x/down]=windowBuffer[dimX*dimY/2];//centre pixels
            // outBuffer[x]=windowBuffer[dimX*dimY/2];//centre pixels
          break;
        }
        case(MAX):{
          // outBuffer[x]=hist.max(windowBuffer);
          outBuffer[x/down]=hist.max(windowBuffer);
          break;
        }
        case(SUM):{
          // outBuffer[x]=hist.sum(windowBuffer);
          outBuffer[x/down]=hist.sum(windowBuffer);
          break;
        }
        case(HOMOG):
	  if(occurrence.size()==1)//all values in window must be the same
	    outBuffer[x/down]=inBuffer[dimY/2][x];
	    // outBuffer[x]=inBuffer[dimY/2][x];
	  else//favorize original value in case of ties
	    outBuffer[x/down]=m_noValue;
	    // outBuffer[x]=m_noValue;
          break;
        case(DENSITY):{
	  if(windowBuffer.size()){
	    vector<short>::const_iterator vit=m_class.begin();
	    while(vit!=m_class.end())
	      outBuffer[x/down]+=100.0*occurrence[*(vit++)]/windowBuffer.size();
	      // outBuffer[x]+=100.0*occurrence[*(vit++)]/windowBuffer.size();
	  }
	  else
	    outBuffer[x/down]=0;
	    // outBuffer[x]=0;
          break;
	}
        case(MAJORITY):{
	  if(occurrence.size()){
            map<int,int>::const_iterator maxit=occurrence.begin();
            for(map<int,int>::const_iterator mit=occurrence.begin();mit!=occurrence.end();++mit){
              if(mit->second>maxit->second)
                maxit=mit;
            }
            if(occurrence[inBuffer[dimY/2][x]]<maxit->second)//
              outBuffer[x/down]=maxit->first;
            else//favorize original value in case of ties
              outBuffer[x/down]=inBuffer[dimY/2][x];
	  }
	  else
	    outBuffer[x/down]=0;
          // outBuffer[x]=0;
          break;
        }
        case(THRESHOLD):{
          assert(m_class.size()==m_threshold.size());
	  if(windowBuffer.size()){
            outBuffer[x/down]=inBuffer[dimY/2][x];//initialize with original value (in case thresholds not met)
            for(int iclass=0;iclass<m_class.size();++iclass){
              if(100.0*(occurrence[m_class[iclass]])/windowBuffer.size()>m_threshold[iclass])
                outBuffer[x/down]=m_class[iclass];
            }
          }
          else
	    outBuffer[x/down]=0;
          break;
        }
        case(MIXED):{
          enum Type { BF=11, CF=12, MF=13, NF=20, W=30 };
          double nBF=occurrence[BF];
          double nCF=occurrence[CF];
          double nMF=occurrence[MF];
          double nNF=occurrence[NF];
          double nW=occurrence[W];
	  if(windowBuffer.size()){
            if((nBF+nCF+nMF)&&(nBF+nCF+nMF>=nNF+nW)){//forest
              if(nBF/(nBF+nCF)>=0.75)
                outBuffer[x/down]=BF;
              else if(nCF/(nBF+nCF)>=0.75)
                outBuffer[x/down]=CF;
              else
                outBuffer[x/down]=MF;
            }
            else{//non-forest
              if(nW&&(nW>=nNF))
                outBuffer[x/down]=W;
              else
                outBuffer[x/down]=NF;
            }
          }
	  else
	    outBuffer[x/down]=inBuffer[indexJ][indexI];
            // outBuffer[x]=0;
          break;
        }
        default:
          break;
        }
      }
      // progress=(1.0+y);
      progress=(1.0+y/down);
      progress+=(output.nrOfRow()*iband);
      progress/=output.nrOfBand()*output.nrOfRow();
      // assert(progress>=0);
      // assert(progress<=1);
      pfnProgress(progress,pszMessage,pProgressArg);
      //write outBuffer to file
      try{
        output.writeData(outBuffer,GDT_Float64,y/down,iband);
        // output.writeData(outBuffer,GDT_Float64,y,iband);
      }
      catch(string errorstring){
	cerr << errorstring << "in band " << iband << ", line " << y << endl;
      }
    }
  }
  // input.close();
  // output.close();
}

//todo: re-implement without dependency of CImg and reg libraries
// void Filter2d::Filter2d::dwt_texture(const string& inputFilename, const string& outputFilename,int dim, int scale, int down, int iband, bool verbose)
// {
//   ImgReaderGdal input;
//   ImgWriterGdal output;
//   if(verbose)
//     cout << "opening file " << inputFilename << endl;
//   input.open(inputFilename);
//   double magicX=1,magicY=1;
//   output.open(outputFilename,(input.nrOfCol()+down-1)/down,(input.nrOfRow()+down-1)/down,scale*3,GDT_Float32,input.getImageType());
//   if(input.isGeoRef()){
//     output.setProjection(input.getProjection());
//     output.copyGeoTransform(input);
//   }
//   if(verbose)
//     cout << "Dimension texture (row x col x band) = " << (input.nrOfCol()+down-1)/down << " x " << (input.nrOfRow()+down-1)/down << " x " << scale*3 << endl;
//   assert(dim%2);
//   int dimX=dim;
//   int dimY=dim;
//   Vector2d<float> inBuffer(dimY,input.nrOfCol());
//   Vector2d<float> outBuffer(scale*3,(input.nrOfCol()+down-1)/down);
//   //initialize last half of inBuffer
//   int indexI=0;
//   int indexJ=0;
//   for(int j=-dimY/2;j<(dimY+1)/2;++j){
//     try{
//       if(verbose)
// 	cout << "reading input line " << abs(j) << endl;
//       input.readData(inBuffer[indexJ],GDT_Float32,abs(j),iband);
//       ++indexJ;
//     }
//     catch(string errorstring){
//       cerr << errorstring << "in band " << iband << ", line " << indexJ << endl;
//     }
//   }
//   const char* pszMessage;
//   void* pProgressArg=NULL;
//   GDALProgressFunc pfnProgress=GDALTermProgress;
//   double progress=0;
//   pfnProgress(progress,pszMessage,pProgressArg);
//   for(int y=0;y<input.nrOfRow();y+=down){
//     if(verbose)
//       cout << "calculating line " << y/down << endl;
//     if(y){//inBuffer already initialized for y=0
//       //erase first line from inBuffer
//       inBuffer.erase(inBuffer.begin());
//       //read extra line and push back to inBuffer if not out of bounds
//       if(y+dimY/2<input.nrOfRow()){
// 	//allocate buffer
// 	inBuffer.push_back(inBuffer.back());
// 	try{
// 	  if(verbose)
// 	    cout << "reading input line " << y+dimY/2 << endl;
//           input.readData(inBuffer[inBuffer.size()-1],GDT_Float32,y+dimY/2,iband);
// 	}
// 	catch(string errorstring){
// 	  cerr << errorstring << "in band " << iband << ", line " << y << endl;
// 	}
//       }
//     }
//     for(int x=0;x<input.nrOfCol();x+=down){
//       Vector2d<double> texture_feature(scale,3);
//       CImg<> texture_in(dimX,dimY);
//       int r=0;//index for row of texture_in
//       for(int j=-dimY/2;j<(dimY+1)/2;++j){
// 	int c=0;
// 	for(int i=-dimX/2;i<(dimX+1)/2;++i){
// 	  indexI=x+i;
// 	  //check if out of bounds
// 	  if(indexI<0)
// 	    indexI=-indexI;
// 	  else if(indexI>=input.nrOfCol())
// 	    indexI=input.nrOfCol()-i;
// 	  if(y+j<0)
// 	    indexJ=-j;
// 	  else if(y+j>=input.nrOfRow())
// 	    indexJ=dimY/2-j;//indexJ=inBuffer.size()-1-j;
// 	  else
// 	    indexJ=dimY/2+j;
// 	  assert(indexJ<inBuffer.size());
// 	  assert(indexI<inBuffer[indexJ].size());
// 	  texture_in(r,c)=inBuffer[indexJ][indexI];
// 	  c++;
// 	}
// 	++r;
//       }
//       texture_in.dwt_texture(texture_feature,scale);
//       for(int v=0;v<scale*3;++v)
// 	outBuffer[v][x/down]=texture_feature[v/3][v%3];
//     }
//     //write outBuffer to file
//     try{
//       if(verbose)
//         cout << "writing line " << y/down << endl;
//       for(int v=0;v<scale*3;++v)
//         output.writeData(outBuffer[v],GDT_Float32,y/down,v);
//     }
//     catch(string errorstring){
//       cerr << errorstring << "in band " << iband << ", line " << y << endl;
//     }
//     progress=(1.0+y)/output.nrOfRow();
//     pfnProgress(progress,pszMessage,pProgressArg);
//   }
//   input.close();
//   output.close();
// }

void Filter2d::Filter2d::morphology(const ImgReaderGdal& input, ImgWriterGdal& output, int method, int dimX, int dimY, bool disc, double angle)
{
  assert(dimX);
  assert(dimY);
  Histogram hist;
  for(int iband=0;iband<input.nrOfBand();++iband){
    Vector2d<double> inBuffer(dimY,input.nrOfCol());
    vector<double> outBuffer(input.nrOfCol());
    //initialize last half of inBuffer
    int indexI=0;
    int indexJ=0;
    for(int j=-dimY/2;j<(dimY+1)/2;++j){
      try{
	input.readData(inBuffer[indexJ],GDT_Float64,abs(j),iband);
	++indexJ;
      }
      catch(string errorstring){
	cerr << errorstring << "in line " << indexJ << endl;
      }
    }
    const char* pszMessage;
    void* pProgressArg=NULL;
    GDALProgressFunc pfnProgress=GDALTermProgress;
    double progress=0;
    pfnProgress(progress,pszMessage,pProgressArg);
    for(int y=0;y<input.nrOfRow();++y){
      if(y){//inBuffer already initialized for y=0
	//erase first line from inBuffer
	inBuffer.erase(inBuffer.begin());
	//read extra line and push back to inBuffer if not out of bounds
	if(y+dimY/2<input.nrOfRow()){
	  //allocate buffer
	  inBuffer.push_back(inBuffer.back());
	  try{
            input.readData(inBuffer[inBuffer.size()-1],GDT_Float64,y+dimY/2,iband);
	  }
	  catch(string errorstring){
	    cerr << errorstring << "in band " << iband << ", line " << y << endl;
	  }
	}
      }
      for(int x=0;x<input.nrOfCol();++x){
	outBuffer[x]=0;
        double currentValue=inBuffer[dimY/2][x];
	vector<double> histBuffer;
	bool currentMasked=false;
	for(int imask=0;imask<m_mask.size();++imask){
	  if(currentValue==m_mask[imask]){
	    currentMasked=true;
	    break;
	  }
	}
	if(currentMasked){
	  outBuffer[x]=currentValue;
	}
	else{
	  for(int j=-dimY/2;j<(dimY+1)/2;++j){
	    for(int i=-dimX/2;i<(dimX+1)/2;++i){
	      if(disc&&(i*i+j*j>(dimX/2)*(dimY/2)))
		continue;
	      // if(angle>=-180){
	      // 	double theta;
	      // 	if(i)
	      // 	  theta=atan(static_cast<double>(j)/(static_cast<double>(i)));
	      // 	else if(j>0)
	      // 	  theta=PI/2.0;
	      // 	else if(j<0)
	      // 	  theta=3.0*PI/2.0;
	      // 	if(j&&(theta<DEG2RAD(angle)||theta>DEG2RAD(angle+PI)))
	      // 	  continue;
	      // }
	      indexI=x+i;
	      //check if out of bounds
	      if(indexI<0)
		indexI=-indexI;
	      else if(indexI>=input.nrOfCol())
		indexI=input.nrOfCol()-i;
	      if(y+j<0)
		indexJ=-j;
	      else if(y+j>=input.nrOfRow())
		indexJ=dimY/2-j;//indexJ=inBuffer.size()-1-j;
	      else
		indexJ=dimY/2+j;
	      bool masked=false;
	      for(int imask=0;imask<m_mask.size();++imask){
		if(inBuffer[indexJ][indexI]==m_mask[imask]){
		  masked=true;
		  break;
		}
	      }
	      if(!masked){
		short binValue=0;
		for(int iclass=0;iclass<m_class.size();++iclass){
		  if(inBuffer[indexJ][indexI]==m_class[iclass]){
		    binValue=1;
		    break;
		  }
		}
		if(m_class.size())
		  histBuffer.push_back(binValue);
		else
		  histBuffer.push_back(inBuffer[indexJ][indexI]);
	      }
	    }
	  }
	  assert(histBuffer.size());//should never occur if not masked (?)
	  switch(method){
	  case(DILATE):
	    outBuffer[x]=hist.max(histBuffer);
	    break;
	  case(ERODE):
	    outBuffer[x]=hist.min(histBuffer);
	    break;
	  default:
	    ostringstream ess;
	    ess << "Error:  morphology method " << method << " not supported, choose " << DILATE << " (dilate) or " << ERODE << " (erode)" << endl;
	    throw(ess.str());
	    break;
	  }
	  if(outBuffer[x]&&m_class.size())
	    outBuffer[x]=m_class[0];
	  // else{
	  //   assert(m_mask.size());
	  //   outBuffer[x]=m_mask[0];
	  // }
	}
      }
      //write outBuffer to file
      try{
        output.writeData(outBuffer,GDT_Float64,y,iband);
      }
      catch(string errorstring){
	cerr << errorstring << "in band " << iband << ", line " << y << endl;
      }
      progress=(1.0+y);
      progress+=(output.nrOfRow()*iband);
      progress/=output.nrOfBand()*output.nrOfRow();
      pfnProgress(progress,pszMessage,pProgressArg);
    }
  }
}
