/**********************************************************************
pkstat.cc: program to calculate basic statistics from raster image
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
#include <iostream>
#include <fstream>
#include <vector>
#include <math.h>
#include "base/Optionpk.h"
#include "fileclasses/FileReaderAscii.h"
#include "algorithms/StatFactory.h"
using namespace std;

int main(int argc, char *argv[])
{
  Optionpk<string> input_opt("i","input","name of the input text file","");
  Optionpk<char> fs_opt("fs","fs","field separator.",' ');
  Optionpk<char> comment_opt("comment","comment","comment character",'#');
  Optionpk<bool> output_opt("o","output","output the selected columns",false);
  Optionpk<int> col_opt("c", "column", "column nr, starting from 0", 0);
  Optionpk<int> range_opt("r", "range", "rows to start/end reading. Use -r 1 -r 10 to read first 10 rows where first row is header. Use 0 to read all rows with no header.", 0);
  Optionpk<bool> size_opt("size","size","sample size",false);
  Optionpk<unsigned int> rand_opt("rnd", "rnd", "generate random numbers", 0);
  Optionpk<std::string> randdist_opt("dist", "dist", "distribution for generating random numbers, see http://www.gn/software/gsl/manual/gsl-ref_toc.html#TOC320 (only uniform and Gaussian supported yet)", "gaussian");
  Optionpk<double> randa_opt("rnda", "rnda", "first parameter for random distribution (standard deviation in case of Gaussian)", 1);
  Optionpk<double> randb_opt("rndb", "rndb", "second parameter for random distribution (standard deviation in case of Gaussian)", 0);
  Optionpk<bool> mean_opt("m","mean","calculate median",false);
  Optionpk<bool> median_opt("med","median","calculate median",false);
  Optionpk<bool> var_opt("var","var","calculate variance",false);
  Optionpk<bool> skewness_opt("skew","skewness","calculate skewness",false);
  Optionpk<bool> kurtosis_opt("kurt","kurtosis","calculate kurtosis",false);
  Optionpk<bool> stdev_opt("stdev","stdev","calculate standard deviation",false);
  Optionpk<bool> sum_opt("s","sum","calculate sum of column",false);
  Optionpk<bool> minmax_opt("mm","minmax","calculate minimum and maximum value",false);
  Optionpk<double> min_opt("min","min","set minimum value",0);
  Optionpk<double> max_opt("max","max","set maximum value",0);
  Optionpk<bool> histogram_opt("hist","hist","calculate histogram",false);
  Optionpk<bool> histogram2d_opt("hist2d","hist2d","calculate 2-dimensional histogram based on two columns",false);
  Optionpk<short> nbin_opt("bin","bin","number of bins to calculate histogram",0);
  Optionpk<bool> relative_opt("rel","relative","use percentiles for histogram to calculate histogram",false);
  Optionpk<double> kde_opt("kde","kde","bandwith of kernel density when producing histogram, use 0 for practical estimation based on Silverman's rule of thumb");
  Optionpk<bool> correlation_opt("cor","correlation","calculate Pearson produc-moment correlation coefficient between two columns (defined by -c <col1> -c <col2>",false);
  Optionpk<bool> rmse_opt("e","rmse","calculate root mean square error between two columns (defined by -c <col1> -c <col2>",false);
  Optionpk<bool> reg_opt("reg","regression","calculate linear regression error between two columns (defined by -c <col1> -c <col2>",false);
  Optionpk<short> verbose_opt("v", "verbose", "verbose mode when > 0", 0);

  bool doProcess;//stop process when program was invoked with help option (-h --help)
  try{
    doProcess=input_opt.retrieveOption(argc,argv);
    fs_opt.retrieveOption(argc,argv);
    comment_opt.retrieveOption(argc,argv);
    output_opt.retrieveOption(argc,argv);
    col_opt.retrieveOption(argc,argv);
    range_opt.retrieveOption(argc,argv);
    size_opt.retrieveOption(argc,argv);
    rand_opt.retrieveOption(argc,argv);
    randdist_opt.retrieveOption(argc,argv);
    randa_opt.retrieveOption(argc,argv);
    randb_opt.retrieveOption(argc,argv);
    mean_opt.retrieveOption(argc,argv);
    median_opt.retrieveOption(argc,argv);
    var_opt.retrieveOption(argc,argv);
    stdev_opt.retrieveOption(argc,argv);
    skewness_opt.retrieveOption(argc,argv);
    kurtosis_opt.retrieveOption(argc,argv);
    sum_opt.retrieveOption(argc,argv);
    minmax_opt.retrieveOption(argc,argv);
    min_opt.retrieveOption(argc,argv);
    max_opt.retrieveOption(argc,argv);
    histogram_opt.retrieveOption(argc,argv);
    histogram2d_opt.retrieveOption(argc,argv);
    nbin_opt.retrieveOption(argc,argv);
    relative_opt.retrieveOption(argc,argv);
    kde_opt.retrieveOption(argc,argv);
    correlation_opt.retrieveOption(argc,argv);
    rmse_opt.retrieveOption(argc,argv);
    reg_opt.retrieveOption(argc,argv);
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

  statfactory::StatFactory stat;
  if(rand_opt[0]>0){
    gsl_rng* r=stat.getRandomGenerator(time(NULL));
    //todo: init random number generator using time...
    if(verbose_opt[0])
      std::cout << "generating " << rand_opt[0] << " random numbers: " << std::endl;
    for(unsigned int i=0;i<rand_opt[0];++i)
      std::cout << i << " " << stat.getRandomValue(r,randdist_opt[0],randa_opt[0],randb_opt[0]) << std::endl;
  }
  vector< vector<double> > dataVector(col_opt.size());
  vector< vector<double> > statVector(col_opt.size());

  if(!input_opt.size())
    exit(0);
  FileReaderAscii asciiReader(input_opt[0]);
  asciiReader.setFieldSeparator(fs_opt[0]);
  asciiReader.setComment(comment_opt[0]);
  asciiReader.setMinRow(range_opt[0]);
  if(range_opt.size()>1)
    asciiReader.setMaxRow(range_opt[1]);
  asciiReader.readData(dataVector,col_opt);
  assert(dataVector.size());
  double minValue=min_opt[0];
  double maxValue=max_opt[0];
  if(histogram_opt[0]){
    if(nbin_opt[0]<1){
      std::cerr << "Warning: number of bins not defined, calculating bins from min and max value" << std::endl;
      if(maxValue<=minValue)
        stat.minmax(dataVector[0],dataVector[0].begin(),dataVector[0].end(),minValue,maxValue);
      nbin_opt[0]=maxValue-minValue+1;
    }
  }
  double minX=min_opt[0];
  double minY=(min_opt.size()==2)? min_opt[1] : min_opt[0];
  double maxX=max_opt[0];
  double maxY=(max_opt.size()==2)? max_opt[1] : max_opt[0];
  if(histogram2d_opt[0]){
    assert(col_opt.size()==2);
    if(nbin_opt[0]<1){
      std::cerr << "Warning: number of bins not defined, calculating bins from min and max value" << std::endl;
      if(maxValue<=minValue){
        stat.minmax(dataVector[0],dataVector[0].begin(),dataVector[0].end(),minX,maxX);
        stat.minmax(dataVector[1],dataVector[1].begin(),dataVector[1].end(),minY,maxY);
      }
      minValue=(minX<minY)? minX:minY;
      maxValue=(maxX>maxY)? maxX:maxY;
      if(verbose_opt[0])
        std::cout << "min and max values: " << minValue << ", " << maxValue << std::endl;
      nbin_opt[0]=maxValue-minValue+1;
    }
  }
  for(int icol=0;icol<col_opt.size();++icol){
    if(!dataVector[icol].size()){
      std::cerr << "Warning: dataVector[" << icol << "] is empty" << std::endl;
      continue;
    }
    if(size_opt[0])
      cout << "sample size column " << col_opt[icol] << ": " << dataVector[icol].size() << endl;
    if(mean_opt[0])
      cout << "mean value column " << col_opt[icol] << ": " << stat.mean(dataVector[icol]) << endl;
    if(var_opt[0])
      cout << "variance value column " << col_opt[icol] << ": " << stat.var(dataVector[icol]) << endl;
    if(stdev_opt[0])
      cout << "standard deviation column " << col_opt[icol] << ": " << sqrt(stat.var(dataVector[icol])) << endl;
    if(skewness_opt[0])
      cout << "skewness value column " << col_opt[icol] << ": " << stat.skewness(dataVector[icol]) << endl;
    if(kurtosis_opt[0])
      cout << "kurtosis value column " << col_opt[icol] << ": " << stat.kurtosis(dataVector[icol]) << endl;
    if(sum_opt[0]){
      cout << setprecision(2);
      cout << fixed << "sum column " << col_opt[icol] << ": " << (stat.sum(dataVector[icol])) << endl;
    }
    if(median_opt[0])
      cout << "median value column " << col_opt[icol] << ": " << stat.median(dataVector[icol]) << endl;
    if(minmax_opt[0]){
      cout << "min value column " << col_opt[icol] << ": " << stat.min(dataVector[icol]) << endl;
      cout << "max value column " << col_opt[icol] << ": " << stat.max(dataVector[icol]) << endl;
    }
    if(histogram_opt[0]){
      //todo: support kernel density function and estimate sigma as in practical estimate of the bandwith in http://en.wikipedia.org/wiki/Kernel_density_estimation
      double sigma=0;
      if(kde_opt.size()){
        if(kde_opt[0]>0)
          sigma=kde_opt[0];
        else
          sigma=1.06*sqrt(stat.var(dataVector[icol]))*pow(dataVector[icol].size(),-0.2);
      }
      assert(nbin_opt[0]);
      if(verbose_opt[0]){
        if(sigma>0)
          std::cout << "calculating kernel density estimate with sigma " << sigma << " for col " << icol << std::endl;
        else
          std::cout << "calculating histogram for col " << icol << std::endl;
      }
      stat.distribution(dataVector[icol],dataVector[icol].begin(),dataVector[icol].end(),statVector[icol],nbin_opt[0],minValue,maxValue,sigma);
      if(verbose_opt[0])
        std::cout << "min and max values: " << minValue << ", " << maxValue << std::endl;
    }
  }
  if(correlation_opt[0]){
    assert(dataVector.size()==2);
    cout << "correlation between columns " << col_opt[0] << " and " << col_opt[1] << ": " << stat.correlation(dataVector[0],dataVector[1]) << endl;
  }
  if(rmse_opt[0]){
    assert(dataVector.size()==2);
    cout << "root mean square error between columns " << col_opt[0] << " and " << col_opt[1] << ": " << stat.rmse(dataVector[0],dataVector[1]) << endl;
  }
  if(reg_opt[0]){
    assert(dataVector.size()==2);
    double c0=0;
    double c1=0;
    double r2=stat.linear_regression(dataVector[0],dataVector[1],c0,c1);
    cout << "linear regression between columns: " << col_opt[0] << " and " << col_opt[1] << ": " << c0 << "+" << c1 << " * x " << " with R^2 (square correlation coefficient): " << r2 << endl;
  }
  if(histogram_opt[0]){
    for(int irow=0;irow<statVector.begin()->size();++irow){
      std::cout << (maxValue-minValue)*irow/(nbin_opt[0]-1)+minValue << " ";
      for(int icol=0;icol<col_opt.size();++icol){
        if(relative_opt[0])
          std::cout << 100.0*static_cast<double>(statVector[icol][irow])/static_cast<double>(dataVector[icol].size());
        else
          std::cout << statVector[icol][irow];
        if(icol<col_opt.size()-1)
          cout << " ";
      }
      cout << endl;
    }
  }
  if(histogram2d_opt[0]){
    assert(nbin_opt[0]);
    assert(dataVector.size()==2);
    assert(dataVector[0].size()==dataVector[1].size());
    double sigma=0;
    //kernel density estimation as in http://en.wikipedia.org/wiki/Kernel_density_estimation
    if(kde_opt.size()){
      if(kde_opt[0]>0)
        sigma=kde_opt[0];
      else
        sigma=1.06*sqrt(sqrt(stat.var(dataVector[0]))*sqrt(stat.var(dataVector[0])))*pow(dataVector[0].size(),-0.2);
    }
    assert(nbin_opt[0]);
    if(verbose_opt[0]){
      if(sigma>0)
        std::cout << "calculating 2d kernel density estimate with sigma " << sigma << " for cols " << col_opt[0] << " and " << col_opt[1] << std::endl;
      else
        std::cout << "calculating 2d histogram for cols " << col_opt[0] << " and " << col_opt[1] << std::endl;
      std::cout << "nbin: " << nbin_opt[0] << std::endl;
    }
    std::vector< std::vector<double> > histVector;
    stat.distribution2d(dataVector[0],dataVector[1],histVector,nbin_opt[0],minX,maxX,minY,maxY,sigma);
    for(int binX=0;binX<nbin_opt[0];++binX){
      std::cout << std::endl;
      for(int binY=0;binY<nbin_opt[0];++binY){
        double value=0;
        value=static_cast<double>(histVector[binX][binY])/dataVector[0].size();
        std::cout << (maxX-minX)*binX/(nbin_opt[0]-1)+minX << " " << (maxY-minY)*binY/(nbin_opt[0]-1)+minY << " " << value << std::endl;
      }
    }
  }
  
  if(output_opt[0]){
    for(int irow=0;irow<dataVector.begin()->size();++irow){
      for(int icol=0;icol<col_opt.size();++icol){
        cout << dataVector[icol][irow];
        if(icol<col_opt.size()-1)
          cout << " ";
      }
      cout << endl;
    }
  }
}