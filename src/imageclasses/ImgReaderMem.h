/**********************************************************************
ImgReaderMem.h: class to read raster files using GDAL API library
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
#ifndef _IMGREADERMEM_H_
#define _IMGREADERMEM_H_

#include "ImgReaderGdal.h"
#include <assert.h>
#include <fstream>
#include <string>
#include <sstream>
#include "gdal_priv.h"
#include "base/Vector2d.h"

//--------------------------------------------------------------------------
//todo: automatic gdal type definitions?
class ImgReaderMem : public ImgReaderGdal
{
public:
  ImgReaderMem(void) {};
  //memory (in MB) indicates maximum memory read in memory for this image
  ImgReaderMem(const std::string& filename, const GDALAccess& readMode=GA_ReadOnly, unsigned long int memory=0) : ImgReaderGdal(filename, readMode) {initMem(memory);};
  //memory (in MB) indicates maximum memory read in memory for this image
  void open(const std::string& filename, const GDALAccess& readMode=GA_ReadOnly, unsigned long int memory=0){ImgReaderGdal::open(filename, readMode);initMem(memory);};
  void setMemory(unsigned long int memory){initMem(memory);};
  ~ImgReaderMem(void){for(int iband=0;iband<m_nband;++iband) free(m_data[iband]);};
  //void open(const std::string& filename, const GDALAccess& readMode=GA_ReadOnly);
  template<typename T1> void readData(T1& value, int col, int row, int band=0);
  template<typename T1> void readData(std::vector<T1>& buffer, int minCol, int maxCol, int row, int band=0);
  template<typename T1> void readData(std::vector<T1>& buffer, int minCol, int maxCol, double row, int band=0, RESAMPLE resample=NEAR);
  template<typename T1> void readDataBlock(Vector2d<T1>& buffer2d, int minCol, int maxCol, int minRow, int maxRow, int band=0);
  template<typename T1> void readDataBlock(std::vector<T1>& buffer, int minCol, int maxCol, int minRow, int maxRow, int band=0);
  template<typename T1> void readData(std::vector<T1>& buffer, int row, int band=0);
  template<typename T1> void readData(std::vector<T1>& buffer, double row, int band=0, RESAMPLE resample=NEAR);

protected:
  unsigned int m_nrow;
  std::vector<void *> m_data;
  unsigned int m_begin;//first line that has been read
  unsigned int m_end;//beyond last line read
private:
  void initMem(unsigned long int memory);
  bool readNewBlock(unsigned int row);
};

//     adfGeoTransform[0] /* top left x */
//     adfGeoTransform[1] /* w-e pixel resolution */
//     adfGeoTransform[2] /* rotation, 0 if image is "north up" */
//     adfGeoTransform[3] /* top left y */
//     adfGeoTransform[4] /* rotation, 0 if image is "north up" */
//     adfGeoTransform[5] /* n-s pixel resolution */

// template<typename T> void ImgReaderMem<T>::open(const std::string& filename, const GDALAccess& readMode){
//   ImgReaderGdal::open(filename, readMode);
//   setCodec();
// }

//todo: support external memory setting (initMem by passing pointer to allocated memory). This will allow in place image processing in memory (streaming)
void ImgReaderMem::initMem(unsigned long int memory)
{
  m_nrow=static_cast<unsigned int>(memory*1000000/nrOfBand()/nrOfCol());
  // //test
  // m_nrow=nrOfRow();
  if(m_nrow<1)
    m_nrow=1;
  if(m_nrow>nrOfRow())
    m_nrow=nrOfRow();
  m_data.resize(m_nband);
  for(int iband=0;iband<m_nband;++iband)
    m_data[iband]=(void *) CPLMalloc((GDALGetDataTypeSize(getDataType())>>3)*nrOfCol()*m_nrow);
  m_begin=0;
  m_end=0;
  readNewBlock(0);
}

bool ImgReaderMem::readNewBlock(unsigned int row)
{
  if(row<m_begin||row>=m_end){
    if(m_end<m_nrow)//first time
      m_end=m_nrow;
    else{
      while(row>=m_end&&m_begin<nrOfRow()){
        m_begin+=m_nrow;
        m_end=m_begin+m_nrow;
      }
    }
    if(m_end>nrOfRow())
      m_end=nrOfRow();
    double theScale=1;
    double theOffset=0;
    for(int iband=0;iband<m_nband;++iband){
      //fetch raster band
      GDALRasterBand  *poBand;
      assert(iband<nrOfBand()+1);
      poBand = m_gds->GetRasterBand(iband+1);//GDAL uses 1 based index
      poBand->RasterIO(GF_Read,0,m_begin,nrOfCol(),m_end-m_begin,m_data[iband],nrOfCol(),m_end-m_begin,getDataType(),0,0);
    }
    return true;//new block was read
  }
  else{
    return false;//no need to read new block
  }
}

template<typename T1> void ImgReaderMem::readData(T1& value, int col, int row, int band)
{
  readNewBlock(row);
  //fetch raster band
  GDALRasterBand  *poBand;
  assert(band<nrOfBand()+1);
  poBand = m_gds->GetRasterBand(band+1);//GDAL uses 1 based index
  assert(col<nrOfCol());
  assert(col>=0);
  assert(row<nrOfRow());
  assert(row>=0);
  int index=(row-m_begin)*nrOfCol()+col;
  //index*=(GDALGetDataTypeSize(getDataType())>>3);//multiply index with size of GDALDataType in Bytes
  if(m_scale.size()>band||m_offset.size()>band){
    double theScale=1;
    double theOffset=0;
    if(m_scale.size()>band)
      theScale=m_scale[band];
    if(m_offset.size()>band)
      theOffset=m_offset[band];
    double dvalue=theScale*(static_cast<double*>(m_data[band]))[index]+theOffset;
    value=static_cast<T1>(dvalue);
  }
  else
    value=(static_cast<T1*>(m_data[band]))[index];
}

template<typename T1> void ImgReaderMem::readData(std::vector<T1>& buffer, int minCol, int maxCol, int row, int band)
{
  readNewBlock(row);
  //fetch raster band
  GDALRasterBand  *poBand;
  assert(band<nrOfBand()+1);
  poBand = m_gds->GetRasterBand(band+1);//GDAL uses 1 based index
  assert(minCol<nrOfCol());
  assert(minCol>=0);
  assert(maxCol<nrOfCol());
  assert(minCol<=maxCol);
  assert(row<nrOfRow());
  assert(row>=0);
  if(buffer.size()!=maxCol-minCol+1)
    buffer.resize(maxCol-minCol+1);
  int index=(row-m_begin)*nrOfCol();
  int minindex=(index+minCol);//*(GDALGetDataTypeSize(getDataType())>>3);
  int maxindex=(index+maxCol);//*(GDALGetDataTypeSize(getDataType())>>3);
  if(getGDALDataType<T1>()==getDataType()){//no conversion needed
    buffer.assign(static_cast<T1*>(m_data[band])+minindex,static_cast<T1*>(m_data[band])+maxindex);
    if(m_scale.size()>band||m_offset.size()>band){
      double theScale=1;
      double theOffset=0;
      if(m_scale.size()>band)
        theScale=m_scale[band];
      if(m_offset.size()>band)
        theOffset=m_offset[band];
      typename std::vector<T1>::iterator bufit=buffer.begin();
      while(bufit!=buffer.end()){
        double dvalue=theScale*(*bufit)+theOffset;
        (*bufit++)=static_cast<T1>(dvalue);
      }
    }
  }
  else{
    for(index=minindex;index<maxindex;++index){
      typename std::vector<T1>::iterator bufit=buffer.begin();
      if(m_scale.size()>band||m_offset.size()>band){
        double theScale=1;
        double theOffset=0;
        if(m_scale.size()>band)
          theScale=m_scale[band];
        if(m_offset.size()>band)
          theOffset=m_offset[band];
        double dvalue=theScale*(*(static_cast<double*>(m_data[band])+index))+theOffset;
        *(bufit++)=static_cast<T1>(dvalue);
      }
      else
        *(bufit++)=*(static_cast<T1*>(m_data[band])+index);
    }
  }
}

//deprecated: there is a new interpolation argument from GDAL 2.0 (see http://www.gdal.org/structGDALRasterIOExtraArg.html)
template<typename T1> void ImgReaderMem::readData(std::vector<T1>& buffer, int minCol, int maxCol, double row, int band, RESAMPLE resample)
{
  //todo: make upper and lower row depend on isGeo...
  std::vector<T1> readBuffer_upper;
  std::vector<T1> readBuffer_lower;
  if(buffer.size()!=maxCol-minCol+1)
    buffer.resize(maxCol-minCol+1);
  double upperRow=row-0.5;
  upperRow=static_cast<int>(upperRow);
  double lowerRow=row+0.5;
  lowerRow=static_cast<int>(lowerRow);
  switch(resample){
  case(BILINEAR):
    if(lowerRow>=nrOfRow())
      lowerRow=nrOfRow()-1;
    if(upperRow<0)
      upperRow=0;
    readData(readBuffer_upper,minCol,maxCol,static_cast<int>(upperRow),band);
    readData(readBuffer_lower,minCol,maxCol,static_cast<int>(lowerRow),band);
    //do interpolation in y
    for(int icol=0;icol<maxCol-minCol+1;++icol){
      buffer[icol]=(lowerRow-row+0.5)*readBuffer_upper[icol]+(1-lowerRow+row-0.5)*readBuffer_lower[icol];
    }
    break;
  default:
    readData(buffer,minCol,maxCol,static_cast<int>(row),band);
    break;
  }
}

template<typename T1> void ImgReaderMem::readDataBlock(Vector2d<T1>& buffer2d, int minCol, int maxCol, int minRow, int maxRow, int band)
{
  buffer2d.resize(maxRow-minRow+1);
  typename std::vector<T1> buffer;
  readDataBlock(buffer,minCol,maxCol,minRow,maxRow,band);
  typename std::vector<T1>::const_iterator startit=buffer.begin();
  typename std::vector<T1>::const_iterator endit=startit;
  for(int irow=minRow;irow<=maxRow;++irow){
    buffer2d[irow-minRow].resize(maxCol-minCol+1);
    endit+=maxCol-minCol+1;
    buffer2d[irow-minRow].assign(startit,endit);
    startit+=maxCol-minCol+1;
  }
}

template<typename T1> void ImgReaderMem::readDataBlock(std::vector<T1>& buffer, int minCol, int maxCol, int minRow, int maxRow, int band)
{
  double theScale=1;
  double theOffset=0;
  if(m_scale.size()>band)
    theScale=m_scale[band];
  if(m_offset.size()>band)
    theOffset=m_offset[band];
  //fetch raster band
  GDALRasterBand  *poBand;
  assert(band<nrOfBand()+1);
  poBand = m_gds->GetRasterBand(band+1);//GDAL uses 1 based index
  if(minCol>=nrOfCol() ||
     (minCol<0) ||
     (maxCol>=nrOfCol()) ||
     (minCol>maxCol) ||
     (minRow>=nrOfRow()) ||
     (minRow<0) ||
     (maxRow>=nrOfRow()) ||
     (minRow>maxRow)){
    std::string errorString="block not within image boundaries";
    throw(errorString);
  }
  if(buffer.size()!=(maxRow-minRow+1)*(maxCol-minCol+1))
    buffer.resize((maxRow-minRow+1)*(maxCol-minCol+1));

  for(int irow=minRow;irow<=maxRow;++irow){
    readNewBlock(irow);
    int index=(irow-m_begin)*nrOfCol();
    int minindex=(index+minCol);//*(GDALGetDataTypeSize(getDataType())>>3);
    int maxindex=(index+maxCol);//*(GDALGetDataTypeSize(getDataType())>>3);
    if(getGDALDataType<T1>()==getDataType()){//no conversion needed
      buffer.assign(static_cast<T1*>(m_data[band])+minindex,static_cast<T1*>(m_data[band])+maxindex);
      if(m_scale.size()>band||m_offset.size()>band){
        typename std::vector<T1>::iterator bufit=buffer.begin();
        while(bufit!=buffer.end()){
          double dvalue=theScale*(*bufit)+theOffset;
          (*bufit++)=static_cast<T1>(dvalue);
        }
      }
    }
    else{
      typename std::vector<T1>::iterator bufit=buffer.begin();
      for(index=minindex;index<maxindex;++index){
        if(m_scale.size()>band||m_offset.size()>band){
          double dvalue=theScale*(*(static_cast<double*>(m_data[band])+index))+theOffset;
          *(bufit++)=static_cast<T1>(dvalue);
        }
        else
          *(bufit++)=*(static_cast<T1*>(m_data[band])+index);
      }
    }
  }
}
  
template<typename T1> void ImgReaderMem::readData(std::vector<T1>& buffer, int row, int band)
{
  readData(buffer,0,nrOfCol()-1,row,band);
}

template<typename T1> void ImgReaderMem::readData(std::vector<T1>& buffer, double row, int band, RESAMPLE resample)
{
  readData(buffer,0,nrOfCol()-1,row,band,resample);
}


#endif // _IMGREADERMEM_H_
