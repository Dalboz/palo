////////////////////////////////////////////////////////////////////////////////
/// @brief generate sample cube using libpalo 1.0
///
/// @file
///
/// Copyright (C) 2006-2008 Jedox AG
///
/// This program is free software; you can redistribute it and/or modify it
/// under the terms of the GNU General Public License (Version 2) as published
/// by the Free Software Foundation at http://www.gnu.org/copyleft/gpl.html.
///
/// This program is distributed in the hope that it will be useful, but WITHOUT
/// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
/// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
/// more details.
///
/// You should have received a copy of the GNU General Public License along with
/// this program; if not, write to the Free Software Foundation, Inc., 59 Temple
/// Place, Suite 330, Boston, MA 02111-1307 USA
/// 
/// You may obtain a copy of the License at
///
/// <a href="http://www.jedox.com/license_palo.txt">
///   http://www.jedox.com/license_palo.txt
/// </a>
///
/// If you are developing and distributing open source applications under the
/// GPL License, then you are free to use Palo under the GPL License.  For OEMs,
/// ISVs, and VARs who distribute Palo with their products, and do not license
/// and distribute their source code under the GPL, Jedox provides a flexible
/// OEM Commercial License.
///
/// Developed by triagens GmbH, Koeln on behalf of Jedox AG. Intellectual
/// property rights has triagens GmbH, Koeln. Exclusive worldwide
/// exploitation right (commercial copyright) has Jedox AG, Freiburg.
///
/// @author Frank Celler, triagens GmbH, Cologne, Germany
/// @author Achim Brandt, triagens GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#include "CubeSample.h"

#include "assert.h"

#include <time.h>
#include <stdlib.h>
#include <stddef.h>

#include <iostream>
#include <sstream>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif


namespace palo {
#ifdef _MSC_VER
  typedef long suseconds_t;
  typedef long long   int64_t;

  inline int gettimeofday (struct timeval* tv, void* tz) {
    union {
      int64_t ns100; // since 1.1.1601 in 100ns units
      FILETIME ft;
    } n;

    GetSystemTimeAsFileTime(&n.ft);

    tv->tv_usec = (long) ((n.ns100 / 10LL) % 1000000LL);
    tv->tv_sec  = (long) ((n.ns100 - 116444736000000000LL) / 10000000LL);

    return 0;
  }
#endif

  static double subtract (const timeval& l, const timeval& r) {
    time_t      sec = l.tv_sec  - r.tv_sec;
    suseconds_t msc = l.tv_usec - r.tv_usec;

    while (msc < 0) {
      msc += 1000000;
      sec -= 1;
    }

    return ((sec * 1000000LL) + msc) / 1000000.0;
  }


  CubeSample::CubeSample (PaloConnector &p)
    : addedDB(false), infoLoaded(false), palo(&p) {
  }



  CubeSample::~CubeSample () {
  }



  void CubeSample::createDatabase (const string& databaseName, 
                                   const string& cubeName,
                                   int numDimension, 
                                   int numConsolidateElements, 
                                   int numElements, 
                                   int extraElements, 
                                   int posExtraElements) {
    cout << "---------------------------------------------------" << endl;
    cout << "creating database '" << databaseName << "' with\n"
         << "  number dimensions: "                             << numDimension << "\n"
         << "  number of consolidated elements per dimension: " << numConsolidateElements << "\n"
         << "  size of normal dimensions: "                     << numElements << "*" << numConsolidateElements << "\n"
         << "  size of large dimensions: "                      << extraElements << "*" << numConsolidateElements << "\n"
         << "  position of large dimensions: "                  << posExtraElements << endl;

    this->databaseName = databaseName;
  
    palo->createDatabase(databaseName);

    this->addedDB = true;
  
    baseElements.resize(numDimension);
    consolidatedElements.resize(numDimension);
  
    timeval t1;
    gettimeofday(&t1, 0);
  
    for (char i = 0;  i < numDimension;  i++) {
      string sd(1, 'A' + i); 
      dimensions.push_back(sd);

      palo->createDimension(databaseName, sd);

      for (char c = 0; c < numConsolidateElements;  c++) { 
        string sce = sd;
        sce.append(1, 'A'+c); 
        consolidatedElements[i].push_back(sce);        

        vector<string> subElements;
      
        int max;

        if (posExtraElements == i) {
          max = extraElements;
        }
        else {
          max = numElements;
        }
      
        for (int e = 0;  e < max;  e++) {
          stringstream se;
          se << sce << e;

          palo->addNumericElement(databaseName, sd, se.str());
 
          baseElements[i].push_back(se.str());       
          subElements.push_back(se.str());
        }

        palo->addConsolidateElement(databaseName, sd, sce, subElements, de_n);
      }   
    
      palo->addConsolidateElement(databaseName, sd, sd, consolidatedElements[i], de_c);
    }
  
    timeval t2;
    gettimeofday(&t2, 0);
  
    cout << "Walltime (create dimension): " << subtract(t2, t1) << endl;

    palo->createCube(databaseName, cubeName, dimensions);

    gettimeofday(&t1, 0);
  
    cout << "Walltime (create cube): " << subtract(t1, t2) << endl;

    this->cubeName = cubeName;

    infoLoaded = true;
  }



  void CubeSample::loadDatabase (const string& databaseName,
         const string& cubeName) {
    if (infoLoaded) {
      return;
    }

    cout << "---------------------------------------------------" << endl;
    this->databaseName = databaseName;
    this->cubeName     = cubeName;

    dimensions = palo->getCubeDimensions(databaseName, cubeName);

    cout << "information for database '" << databaseName << "' loaded\n"
         << "  number dimensions: " << (int) dimensions.size() << "\n";

    for (vector<string>::const_iterator i = dimensions.begin();  i != dimensions.end();  i++) {
      vector<string> be = palo->getNumericElements(databaseName, *i);
      vector<string> ce = palo->getConsolidatedElements(databaseName, *i);

      baseElements.push_back(be);
      consolidatedElements.push_back(ce);

      cout << "  dimension '" << *i << "' # base/consolidated: " << (int) be.size() << " / " << (int) ce.size() << "\n";
    }

    cout << flush;
  }


  void CubeSample::deleteDatabase () {
    if (this->addedDB) {
      palo->deleteDatabase(databaseName);
      this->addedDB = false;
    }
  }


           
  void CubeSample::setData (double rate, double * runtime) {
    cout << "---------------------------------------------------" << endl;
    cout << "setting data " << rate << "%" << endl;
  
    vector<int> numberBaseElements;
  
    long max = 1;

    for (size_t i = 0;  i < dimensions.size();  i++) {
      size_t nb = baseElements[i].size();

      numberBaseElements.push_back((int) nb);
      max *= (long) nb;
    }
    
    cout << "total number of elements " << max << endl;
    cout << "setting " << (int) (max * rate / 100) << " elements" << endl;
  
    double go = 100 / rate;

    vector<string> names(numberBaseElements.size());

    timeval t1;
    gettimeofday(&t1, 0);
  
    for (double i = 0;  i < max;  i += go) {
      long int x = (long int)i;
      
      for (size_t j = 0;  j < numberBaseElements.size();  j++) {
        long int pos = x % numberBaseElements.at(j);
        names.at(j) = baseElements[j][pos];
        x = x / numberBaseElements.at(j);
      }

      palo->setData(databaseName, cubeName, names, 1.0);
    } 

    timeval t2;
    gettimeofday(&t2, 0);
  
    cout << "walltime: " << subtract(t2, t1) << endl;

    if (runtime != 0) {
      *runtime = subtract(t2, t1);
    }
  }



  double CubeSample::getData1 (bool useMulti, double * runtime) {
    cout << "---------------------------------------------------" << endl;

    if (useMulti) {
      cout << "getdata 1 using getdata_multi" << endl;
    }
    else {
      cout << "getdata 1 using getdata" << endl;
    }

    vector<int> numberConsElements;
    vector< vector<string> > elements;
  
    long max = 1;

    for (size_t i = 0;  i < dimensions.size();  i++) {
      size_t nb = consolidatedElements[i].size();

      if (nb == 0) {
        if (baseElements[i].empty()) {
          cerr << "dimension '" << dimensions[i] << "' is empty, giving up" << endl;
          return 0;
        }
        
        nb = 1;
        cout << "using '" << baseElements[i][0] << "' instead of consolidation" << endl;
        
        vector<string> fake;
        fake.push_back(baseElements[i][0]);
        
        elements.push_back(fake);
      }
      else {
        elements.push_back(consolidatedElements[i]);
      }
      
      numberConsElements.push_back((int) nb);
      max *= (long) nb;
    }
    
    vector< vector<string> > requests;
    vector<string> path(numberConsElements.size());
    
    // compute all possible cells (without recursion)  
    for (long int i = 0;  i < max;  i++) {
      long int x = i;
      
      for (size_t j = 0;   j < numberConsElements.size();   j++) {
        long int pos = x % numberConsElements.at(j);
        path.at(j) = elements[j][pos];
        x = x / numberConsElements.at(j);
      }
    
      requests.push_back(path);
    }

    // now get the data
    timeval t1;
    gettimeofday(&t1, 0);

    double result;
  
    if (useMulti) {
      result = palo->getDataMulti(databaseName, cubeName, requests);
    }
    else {
      result = 0.0;

      for (vector< vector<string> >::iterator i = requests.begin();  i != requests.end();  i++) {
        double value = palo->getData(databaseName, cubeName, *i);

        // vector<string>& names = *i;
        //
        // for (vector<string>::iterator j = names.begin();  j != names.end();  j++) {
        //   cout << *j << " ";
        // }
        //
        // cout << value << endl;

        result += value;
      }
    }

    timeval t2;
    gettimeofday(&t2, 0);
  
    cout << "walltime: " << subtract(t2, t1) << endl;

    if (runtime != 0) {
      *runtime = subtract(t2, t1);
    }

    return result;
  }



  double CubeSample::getData2 (int dim, bool useMulti, double * runtime) {
    cout << "---------------------------------------------------" << endl;

    if (useMulti) {
      cout << "getdata 2 using getdata_multi" << endl;
    }
    else {
      cout << "getdata 2 using getdata" << endl;
    }

    vector< vector<string> > requests;
    vector<string> path;
    path.resize(baseElements.size());

    for (size_t i = 0; i < baseElements.size(); i++) {
      path.at(i) = baseElements[i][0];
    }

    for (vector<string>::iterator i = (baseElements.at(dim)).begin();  i != (baseElements.at(dim)).end(); i++) {
      path.at(dim) = *i;
      requests.push_back(path);
    } 

    // now get the data
    timeval t1;
    gettimeofday(&t1, 0);

    double result;
  
    if (useMulti) {
      result = palo->getDataMulti(databaseName, cubeName, requests);
    }
    else {
      result = 0;

      for (vector< vector<string> >::iterator i = requests.begin(); i != requests.end(); i++) {
        result += palo->getData(databaseName, cubeName, (*i));
      }
    }

    timeval t2;
    gettimeofday(&t2, 0);

    cout << "got " << (int) requests.size() << " values, sum " << result << "\n"
         << "walltime: " << subtract(t2, t1) << endl;

    if (runtime != 0) {
      *runtime = subtract(t2, t1);
    }

    return result;
  }



  double CubeSample::getData3 (int dim1, int dim2, bool useMulti, double * runtime) {
    cout << "---------------------------------------------------" << endl;

    if (useMulti) {
      cout << "getdata 2 using getdata_multi" << endl;
    }
    else {
      cout << "getdata 2 using getdata" << endl;
    }

    vector< vector<string> > requests;
    vector<string> path;
    path.resize(baseElements.size());

    for (size_t i = 0; i < baseElements.size(); i++) {
      path.at(i) = baseElements[i][0];
    }

    for (vector<string>::iterator i = (baseElements.at(dim1)).begin();  i != (baseElements.at(dim1)).end(); i++) {
      path.at(dim1) = *i;

      for (vector<string>::iterator j = (baseElements.at(dim2)).begin();  j != (baseElements.at(dim2)).end(); j++) {
        path.at(dim2) = *j;
        requests.push_back(path);
      } 
    }

    // now get the data
    timeval t1;
    gettimeofday(&t1, 0);

    double result;
  
    if (useMulti) {
      result = palo->getDataMulti(databaseName, cubeName, requests);
    }
    else {
      result = 0;

      for (vector< vector<string> >::iterator i = requests.begin(); i != requests.end(); i++) {
        result += palo->getData(databaseName, cubeName, (*i));
      }
    }

    timeval t2;
    gettimeofday(&t2, 0);

    cout << "got " << (int) requests.size() << " values, sum " << result << "\n"
         << "walltime: " << subtract(t2, t1) << endl;

    if (runtime != 0) {
      *runtime = subtract(t2, t1);
    }

    return result;
  }



  double CubeSample::getArea1 (int dim, double * runtime) {
    cout << "---------------------------------------------------\n"
         << "getarea 1 using getdata_area for fixed dimension " << dim << endl;

    vector< vector<string> > path(baseElements.size());

    for (size_t i = 0; i < dimensions.size(); i++) {
      if (i != dim) {
        vector<string>& be = baseElements[i];
        
        for (vector<string>::iterator j = be.begin();  j != be.end();  j++) {
          path[i].push_back(*j);
        }
      }
      else {
        path[i].push_back(baseElements[i][0]);
        cout << "fixing '" << baseElements[i][0] << "'" << endl;
      }
    }
    
    // now get the data
    timeval t1;
    gettimeofday(&t1, 0);
    
    double result = palo->getArea(databaseName, cubeName, path);
    
    timeval t2;
    gettimeofday(&t2, 0);
    
    cout << "walltime: " << subtract(t2, t1) << endl;

    if (runtime != 0) {
      *runtime = subtract(t2, t1);
    }

    return result;
  }
  
  
  
  double CubeSample::getArea2 (int dim, double * runtime) {
    cout << "---------------------------------------------------\n"
         << "getarea 1 using getdata_area for fixed dimension " << dim << endl;

    vector< vector<string> > path;
    path.resize(baseElements.size());

    for (size_t i = 0; i < baseElements.size(); i++) {
      if (i != dim) {
        vector<string>& be = baseElements[i];

        for (vector<string>::iterator j = be.begin();  j != be.end();  j++) {
          path[i].push_back(*j);
        }
      }
      else {
        if (consolidatedElements[i].empty()) {
          cerr << "no consolidating elements in dimension '" << dimensions[i] << "'" << endl;
          return 0;
        }
        
        path[i].push_back(consolidatedElements[i][0]);
        cout << "fixing '" << consolidatedElements[i][0] << "'" << endl;
      }
    }
    
    // now get the data
    timeval t1;
    gettimeofday(&t1, 0);
  
    double result = palo->getArea(databaseName, cubeName, path);

    timeval t2;
    gettimeofday(&t2, 0);
  
    cout << "walltime: " << subtract(t2, t1) << endl;

    if (runtime != 0) {
      *runtime = subtract(t2, t1);
    }

    return result;
  }
}
