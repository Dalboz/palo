////////////////////////////////////////////////////////////////////////////////
/// @brief palo cache storage
///
/// @file
///
/// The contents of this file are subject to the Jedox AG Palo license. You
/// may not use this file except in compliance with the license. You may obtain
/// a copy of the License at
///
/// <a href="http://www.palo.com/license.txt">
///   http://www.palo.com/license.txt
/// </a>
///
/// Software distributed under the license is distributed on an "AS IS" basis,
/// WITHOUT WARRANTY OF ANY KIND, either express or implied.  See the license
/// for the specific language governing rights and limitations under the
/// license.
///
/// Developed by triagens GmbH, Koeln on behalf of Jedox AG. Intellectual
/// property rights has triagens GmbH, Koeln. Exclusive worldwide
/// exploitation right (commercial copyright) has Jedox AG, Freiburg.
///
/// @author Frank Celler, triagens GmbH, Cologne, Germany
/// @author Achim Brandt, triagens GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#include "Olap/CacheStorage.h"

#include "Exceptions/KeyBufferException.h"

#include "InputOutput/Logger.h"
#include "InputOutput/Statistics.h"

#include "Olap/CellPath.h"
#include "Olap/CubeIndex.h"
#include "Olap/Dimension.h"

#include <iostream>

namespace palo {
  
  CacheStorage::CacheStorage (const vector<size_t>* sizeDimensions,
                            size_t valueSize)
    : numberElements(0),
      valueSize(valueSize) {
    maxima.clear();
    
    size_t totalNumberBits = 0;
    
    counterSize = sizeof(uint32_t);
      
    for (vector<size_t>::const_iterator i = sizeDimensions->begin();
         i != sizeDimensions->end();
         i++) {
      uint32_t bitsDimension = 32;
      uint32_t maximum = ~0;

      maxima.push_back(maximum);
      totalNumberBits += bitsDimension;
    }
    
    keySize = ((totalNumberBits + 31) / 32) * 4; // normalise to 32 bit, convert to byte
    elementSize = keySize + valueSize + counterSize;

    Logger::trace << "creating new CacheStorage: key size = " << keySize 
                  << ", value size = " <<  valueSize << endl;
         
    tmpKeyBuffer = new uint8_t [keySize];
    tmpElementBuffer = new uint8_t [elementSize];

    // generate index
    index = new CubeIndex(keySize, valueSize);

    page = 0;
  }



  CacheStorage::~CacheStorage () {
    delete[] tmpKeyBuffer;
    delete[] tmpElementBuffer;
    
    if (page != 0) {
      delete page;
    }

    delete index;
  }


    void CacheStorage::shrinkCacheStorages () {
      set<CacheStorage*>* caches = getStorages(); 
      
      // minimum cache size      
      size_t min = CachePage::PAGE_SIZE * caches->size();
      
      if (min > page->getTotalCacheSize()) {
        // it is not possible to shrink the cache
        Logger::info << "maximum cache size is to small" << endl;
        return;
      }
      
      Logger::info << "cache size = " << page->getTotalCacheSize() << endl;
      for (set<CacheStorage*>::iterator i = caches->begin(); i != caches->end(); i++) {
        (*i)->shrinkCacheStorage();
      }
      Logger::info << "cache size after shrinking = " << page->getTotalCacheSize() << endl;
      
    }


    void CacheStorage::shrinkCacheStorage () {
      page->shrink();
      numberElements = page->getNumberElements();
    }

    
    void CacheStorage::clear () {
      Logger::debug << "clearing cache" << endl;
      
      if (page != 0) {
        delete page;
        page=0;
      }
      delete index;

      // generate index
      index = new CubeIndex(keySize, valueSize);
  
      numberElements = 0;
    }

    
    void CacheStorage::deleteCellsRecursive (
          vector<IdentifierType>* path,
          size_t position,
          vector< set<IdentifierType> >* area) {
      if (position == area->size()) {
        deleteCell(path);
      }
      else {
        size_t p = position+1;
        for (set<IdentifierType>::iterator i = area->at(position).begin() ; i != area->at(position).end(); i++) {
          path->at(position) = *i;
          deleteCellsRecursive(path, p, area); 
        }
      } 
    }      
    
    
    void CacheStorage::deleteCells (vector< set<IdentifierType> >* area) {      
      vector<IdentifierType> path(area->size());
      deleteCellsRecursive(&path, 0, area);
    }

    
    void CacheStorage::deleteCells (size_t numDimension, set<IdentifierType>* elements) {
      page->deleteRows(numDimension, elements);
    }
        
}
