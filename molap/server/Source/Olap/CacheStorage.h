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

#ifndef OLAP_CACHE_STORAGE_H
#define OLAP_CACHE_STORAGE_H 1

#include "palo.h"

#include <set>

#include "Exceptions/ErrorException.h"

#include "InputOutput/Logger.h"

#include "Olap/CellPath.h"
#include "Olap/CubeIndex.h"
#include "Olap/CachePage.h"
#include "Olap/Element.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief palo cache storage
  ///
  /// A cache storage uses a cache page with a fixed memory size to store the values. 
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS CacheStorage {

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief Creates an empty cache storage
    ////////////////////////////////////////////////////////////////////////////////

    CacheStorage (const vector<size_t>* sizeDimensions, size_t valueSize);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief Remove a cache storage
    ////////////////////////////////////////////////////////////////////////////////

    virtual ~CacheStorage ();

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief removes a value
    ////////////////////////////////////////////////////////////////////////////////

    template <typename PATH>
    void deleteCell (const PATH path) {
      fillKeyBuffer(tmpKeyBuffer, path);

      CachePage::element_t element = index->lookupKey(tmpKeyBuffer);

      if (element == 0) {
        return;
      }

      if (numberElements == 0) {
        throw ErrorException(ErrorException::ERROR_INTERNAL,
                             "in CacheStorage::deleteCell numberElements is 0");
      }

      page->removeElement(element);
      numberElements--;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief copies the value ("valueSize" bytes) to the cube storage
    ////////////////////////////////////////////////////////////////////////////////

    template <typename PATH>
    void setCellValue (const PATH path, CachePage::value_t value) {
      CachePage::key_t keyBuffer = tmpElementBuffer + valueSize;
      
      CachePage::counter_t counterBuffer = tmpElementBuffer + valueSize + keySize;
      * (uint32_t*) counterBuffer = 0;
       
      memcpy(tmpElementBuffer, value, valueSize);

      fillKeyBuffer(keyBuffer, path);

      CachePage::element_t element = index->lookupKey(keyBuffer);

      if (element == 0) {
        
        element = page->addElement(tmpElementBuffer);        
        numberElements++;

        // check size of used memory for cache pages
        if (page->getTotalCacheSize() > page->getMaximumCacheSize()) {
          shrinkCacheStorages();
        }

      }
      else {
        memcpy(element, tmpElementBuffer, elementSize);
      }
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns a pointer to a cell value
    ///
    /// The method returns "0" for an undefined value   
    ////////////////////////////////////////////////////////////////////////////////

    template <typename PATH>
    CachePage::element_t getCellValue (const PATH path) {
      fillKeyBuffer(tmpKeyBuffer, path);

      CachePage::element_t result = index->lookupKey(tmpKeyBuffer);
      
      if (result) {
        CachePage::counter_t counterBuffer = result + valueSize + keySize;
        uint32_t* counter = (uint32_t*) counterBuffer;      
        (*counter) += 1;      
      }
      
      return result;
    }


    ////////////////////////////////////////////////////////////////////////////////
    /// @brief fills path
    ////////////////////////////////////////////////////////////////////////////////
    
    void fillPath (const CachePage::element_t row, IdentifiersType* path) {
      const IdentifierType * buffer = (const IdentifierType*) (row + valueSize);
      IdentifiersType::iterator pathIter = path->begin();

      for (;  pathIter != path->end();  pathIter++, buffer++) {
        *pathIter = *buffer;
      }
    }

  public:
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the maximum number of each dimension  
    ////////////////////////////////////////////////////////////////////////////////
    
    const vector<size_t> * getSizeDimensions () const {
      return &maxima;
    };
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief number of used rows  
    ////////////////////////////////////////////////////////////////////////////////
    
    size_t size () const {
      return numberElements;
    };
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief memory size of a value in byte  
    ////////////////////////////////////////////////////////////////////////////////
    
    size_t getValueSize () const {
      return valueSize;
    };

    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief clear storage  
    ////////////////////////////////////////////////////////////////////////////////
    
    virtual void clear ();
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief delete elements  
    ////////////////////////////////////////////////////////////////////////////////
    
    void deleteCells (vector< set<IdentifierType> >* area);
    void deleteCells (size_t numDimension, set<IdentifierType>* elements);
    
  protected:
    void fillKeyBuffer (CachePage::key_t keyBuffer, const IdentifiersType * path) {
      IdentifierType * buffer = (IdentifierType*) keyBuffer;
      IdentifiersType::const_iterator pathIter = path->begin();

      for (;  pathIter != path->end();  pathIter++) {
        *buffer++ = (IdentifierType) (*pathIter);
      }
    }



    void fillKeyBuffer (CachePage::key_t keyBuffer, const PathType * path) {
      IdentifierType * buffer = (IdentifierType*) keyBuffer;
      PathType::const_iterator pathIter = path->begin();

      for (;  pathIter != path->end();  pathIter++) {
        *buffer++ = (IdentifierType) (*pathIter)->getIdentifier();
      }
    }



    void fillKeyBuffer (CachePage::key_t keyBuffer, const PathWeightType * path) {
      IdentifierType * buffer = (IdentifierType*) keyBuffer;
      PathType::const_iterator pathIter = path->first.begin();

      for (;  pathIter != path->first.end();  pathIter++) {
        *buffer++ = (IdentifierType) (*pathIter)->getIdentifier();
      }
    }



    void fillKeyBuffer (CachePage::key_t keyBuffer, CachePage::key_t path) {
      memcpy(keyBuffer, path, keySize);
    }



    void fillKeyBuffer (CachePage::key_t keyBuffer, const CellPath * path) {
      fillKeyBuffer(keyBuffer, path->getPathIdentifier());
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief shrink size of cache storage
    ////////////////////////////////////////////////////////////////////////////////

    void shrinkCacheStorage ();

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief shrink size of all cache storages
    ////////////////////////////////////////////////////////////////////////////////

    void shrinkCacheStorages ();

  protected:    
    void deleteCellsRecursive (
          vector<IdentifierType>* path,
          size_t position,
          vector< set<IdentifierType> >* area);
          
    virtual set<CacheStorage*>* getStorages () = 0;          
          
  protected:
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief total number of elements
    ////////////////////////////////////////////////////////////////////////////////
    
    size_t numberElements;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief maximum possible value for dimension identifier
    ////////////////////////////////////////////////////////////////////////////////
    
    vector<size_t> maxima;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief the used page
    ////////////////////////////////////////////////////////////////////////////////
    
    CachePage * page;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief memory size needed to store a path 
    ////////////////////////////////////////////////////////////////////////////////
    
    size_t keySize;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief memory size needed to store a value
    ////////////////////////////////////////////////////////////////////////////////
    
    size_t valueSize;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief size of the counter value
    ////////////////////////////////////////////////////////////////////////////////

    size_t counterSize;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief memory size needed to store the path and the value
    ////////////////////////////////////////////////////////////////////////////////
    
    size_t elementSize;
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief Pointer to a cube index object
    ////////////////////////////////////////////////////////////////////////////////
    
    CubeIndex * index;
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief temporary buffer
    ////////////////////////////////////////////////////////////////////////////////
    
    uint8_t * tmpKeyBuffer;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief temporary buffer
    ////////////////////////////////////////////////////////////////////////////////
    
    uint8_t * tmpElementBuffer;

  };

}

#endif 
