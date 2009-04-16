////////////////////////////////////////////////////////////////////////////////
/// @brief palo cube storage
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

#ifndef OLAP_CUBE_STORAGE_H
#define OLAP_CUBE_STORAGE_H 1

#include "palo.h"

#include <set>
#include <map>

#include "Exceptions/ErrorException.h"

#include "Olap/CellPath.h"
#include "Olap/Cube.h"
#include "Olap/CubeIndex.h"
#include "Olap/CubePage.h"
#include "Olap/Element.h"

namespace palo {
  class AreaStorage;
  class CubeLooper;
  class ExportStorage;
  class HashAreaStorage;
  class Lock;
  class MarkerStorage;
  class User;

  ////////////////////////////////////////////////////////////////////////////////
  /// @brief palo cube storage
  ///
  /// A cube storage uses cube pages with a fixed memory size to store the values. 
  /// Each cube page is divided into rows. A row consits of a value (double value
  /// or char*) and a key. The memory size needed for the row is calculated
  /// by the constructor. 
  ////////////////////////////////////////////////////////////////////////////////

  class SERVER_CLASS CubeStorage {
    friend class CubeLooper;

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief Creates an empty cube storage
    ////////////////////////////////////////////////////////////////////////////////

    CubeStorage (Cube*,
                 const vector<size_t>* sizeDimensions, 
                 size_t valueSize,
                 bool isStringStorage);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief Remove a cube storage
    ////////////////////////////////////////////////////////////////////////////////

    ~CubeStorage ();

  public:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief removes a value
    ////////////////////////////////////////////////////////////////////////////////

    template <typename PATH>
    bool deleteCell (const PATH path) {
      fillKeyBuffer(tmpKeyBuffer, path);

      CubePage::element_t element = index->lookupKey(tmpKeyBuffer);

      if (element == 0) {
        return false;
      }

      if (numberElements == 0) {
        throw ErrorException(ErrorException::ERROR_INTERNAL,
                             "in CubePage::deleteCell numberElements is 0");
      }

      CubePage * page = lookupCubePage(tmpKeyBuffer);

      if (page == 0) {
        throw ErrorException(ErrorException::ERROR_INTERNAL,
                             "in CubePage::deleteCell using lookupCubePage");
      }

      if (isString) {
        delete [] * (char**) element;
        page->removeElement(element);
        numberElements--;
      }
      else {
        if (isMarked(element)) {
          * (double*) element = 0;
        }
        else {
          page->removeElement(element);
          numberElements--;
        }
      }

      return true;
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief copies the value ("valueSize" bytes) to the cube storage
    ////////////////////////////////////////////////////////////////////////////////

    template <typename PATH>
    void setCellValue (const PATH path, CubePage::value_t value, bool isMarked) {
      CubePage::key_t keyBuffer = tmpElementBuffer + valueSize;

      fillKeyBuffer(keyBuffer, path);

      CubePage::element_t element = index->lookupKey(keyBuffer);

      if (element == 0) {
        CubePage * page = findCubePage(keyBuffer);
        memcpy(tmpElementBuffer, value, valueSize);
        element = page->addElement(tmpElementBuffer, isMarked);
        
        numberElements++;

        if (! isString) {
          CubePage::element_t t = new uint8_t[keySize];
          memcpy(t, element + valueSize, keySize);
          cube->checkFromMarkers(t);
          delete[] t;
        }
      }
      else {
        if (isMarked) {
          setMarker(element);
        }
        else {
          clearMarkerAndDeleted(element);

          if (isString) {
            delete [] * (char**) element;
          }

          memcpy(tmpElementBuffer, value, valueSize);
          memcpy(element, tmpElementBuffer, elementSize);
        }
      }
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns a pointer to a cell value
    ///
    /// The method returns "0" for an undefined value   
    ////////////////////////////////////////////////////////////////////////////////

    template <typename PATH>
    CubePage::element_t getCellValue (const PATH path) {
      fillKeyBuffer(tmpKeyBuffer, path);

      return index->lookupKey(tmpKeyBuffer);
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns consolidated double value
    ///
    /// @warning: the element vector in baseElements must be sorted!
    ////////////////////////////////////////////////////////////////////////////////

    double getConsolidatedCellValue (const vector<IdentifiersWeightType> * baseElements, 
                                     bool * found,
                                     size_t * numFoundElements,
                                     User*);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief splash consolidated double value by factor
    ///
    /// @warning: the element vector in baseElements must be sorted!
    ////////////////////////////////////////////////////////////////////////////////

    void setConsolidatedCellValue (const vector<IdentifiersWeightType> * baseElements, 
                                   double factor,
                                   Lock* lock);

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief fills path
    ////////////////////////////////////////////////////////////////////////////////
    
    void fillPath (const CubePage::element_t row, IdentifiersType* path) {
      const IdentifierType * buffer = (const IdentifierType*) (row + valueSize);
      IdentifiersType::iterator pathIter = path->begin();

      for (;  pathIter != path->end();  pathIter++, buffer++) {
        *pathIter = *buffer;
      }
    }

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns a pointer to the associative array
    ///
    /// @warning if you add elements to the index the pointer is invalidated.
    ////////////////////////////////////////////////////////////////////////////////    

    CubePage::element_t const * getArray (size_t& size) {
      return index->getArray(size);
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief deletes rows by mask
    ////////////////////////////////////////////////////////////////////////////////
    
    void deleteByMask (const IdentifiersType* path, const IdentifiersType* mask);
    
  public:
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief returns the maximum number of each dimension  
    ////////////////////////////////////////////////////////////////////////////////
    
    const vector<size_t> * getSizeDimensions () const {
      return &maxima;
    };
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief true if the storage stores pointer to strings
    ////////////////////////////////////////////////////////////////////////////////

    bool isStringStorage() const {
      return isString;
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


    void computeAreaCellValue (AreaStorage* storage,
                               vector< map<IdentifierType, map<IdentifierType, double> > > *baseElements,
                               User*);

    void computeHashAreaCellValue (HashAreaStorage*,
                                   vector< vector< vector< pair<uint32_t, double> > > >& numericMapping,
                                   User*);
                               
    void computeExportCellValue (ExportStorage* storage,
                                 vector< map<IdentifierType, map<IdentifierType, double> > > *baseElements,
                                 bool baseElementsOnly,								 
                                 User*);
                               
    void computeExportStringCellValue (ExportStorage* storage, vector<IdentifiersType>* area);    
    
    void sort ();


    void removeCellValue (vector< set<IdentifierType> > *baseElements, Lock* lock);
                               
    IdentifierType getFirstDimension () const {
      return (IdentifierType) firstDimension;
    }

    IdentifierType getSecondDimension () const {
      return (IdentifierType) secondDimension;
    }

    CubePage * lookupCubePage (IdentifierType id1, IdentifierType id2) {
      if (id1 >= endIdentifier1 || id2 >= endIdentifier2) {
        return 0;
      }

      PageKey key;
      key.first  = id1;
      key.second = id2;

      return cubePages.findKey(&key);
    }

    void setMarkers (const vector< set<IdentifierType> > * baseElements,
                     MarkerStorage* storageMarkers);

    void setMarkersRecursively (CubePage * page,
                                CubePage::buffer_t start,
                                CubePage::buffer_t stop,
                                vector< set<IdentifierType> >::const_iterator base,
                                size_t minimalDimension,
                                size_t dimension,
                                MarkerStorage* storageMarkers);

    void clearAllMarkers ();

  private:
    void computeAreaCellValue (AreaStorage* storage,
                               CubePage * page,
                               vector< map<IdentifierType, map<IdentifierType, double> > >* baseElements,
                               size_t minimalDimension,
                               size_t dimension,
                               vector< map<IdentifierType, double>* >& mappings,
                               User* user);    

    void setAreaValue (AreaStorage* storage, double value, vector< map<IdentifierType, double>* >& mappings);

    void computeHashAreaCellValue (double* storage,
                                   CubePage * page,
                                   size_t minimalDimension,
                                   size_t maximalDimension,
                                   vector< vector< vector< pair<uint32_t, double> > > >& numericMapping,
                                   vector< vector< pair<uint32_t, double> >* >& mappings,
                                   User*);

    void setHashAreaValue (double* storage,
                           double value,
                           vector< vector< pair<uint32_t, double> >* >& mappings);

    void getCellsNotRecursive (vector<CubePage::element_t> * elements,
                               CubePage * page,
                               CubePage::buffer_t start,
                               CubePage::buffer_t stop,
                               vector< set<IdentifierType> >* baseElements,
                               size_t minimalDimension,
                               size_t dimension);
                               
    void removeCubeCells (CubePage *page, vector<CubePage::buffer_t>  * elements, Lock* lock);                               

    void computeExportCellValueNotRecursive (ExportStorage* storage,
                                             CubePage * page,
                                             CubePage::buffer_t start,
                                             CubePage::buffer_t stop,
                                             vector< map<IdentifierType, map<IdentifierType, double> > >* baseElements,
                                             size_t minimalDimension,
                                             size_t dimension,
                                             vector< map<IdentifierType, double>* >& mappings,											
                                             User*);    

    void setExportValue (ExportStorage* storage, double value, vector< map<IdentifierType, double>* >& mappings);
    
    struct PageKey {
      IdentifierType first;
      IdentifierType second;
    };

    struct PageDesc {
      PageDesc () {
      }

      uint32_t hash (PageKey const * key) {
        return StringUtils::hashValue32((uint32_t*) key, sizeof(PageKey) / 4);
      }

      bool isEmptyElement (CubePage * & element) {
        return element == 0;
      }

      uint32_t hashElement (CubePage * const & element) {
        PageKey key;
        key.first  = element->getFirst();
        key.second = element->getSecond();

        return hash(&key);
      }

      uint32_t hashKey (PageKey const * key) {
        return hash(key);
      }

      bool isEqualElementElement (CubePage * const & left, CubePage * const & right) {
        return left->getFirst() == right->getFirst() && left->getSecond() == right->getSecond();
      }

      bool isEqualKeyElement (PageKey const * key, CubePage * const & element) {
        return key->first == element->getFirst() && key->second == element->getSecond();
      }

      void clearElement (CubePage * & element) {
        element = 0;
      }
    };

  private:
    void fillKeyBuffer (CubePage::key_t keyBuffer, const IdentifiersType * path) {
      IdentifierType * buffer = (IdentifierType*) keyBuffer;
      IdentifiersType::const_iterator pathIter = path->begin();

      for (;  pathIter != path->end();  pathIter++) {
        *buffer++ = (IdentifierType) (*pathIter);
      }
    }



    void fillKeyBuffer (CubePage::key_t keyBuffer, const PathType * path) {
      IdentifierType * buffer = (IdentifierType*) keyBuffer;
      PathType::const_iterator pathIter = path->begin();

      for (;  pathIter != path->end();  pathIter++) {
        *buffer++ = (IdentifierType) (*pathIter)->getIdentifier();
      }
    }



    void fillKeyBuffer (CubePage::key_t keyBuffer, const PathWeightType * path) {
      IdentifierType * buffer = (IdentifierType*) keyBuffer;
      PathType::const_iterator pathIter = path->first.begin();

      for (;  pathIter != path->first.end();  pathIter++) {
        *buffer++ = (IdentifierType) (*pathIter)->getIdentifier();
      }
    }



    void fillKeyBuffer (CubePage::key_t keyBuffer, CubePage::key_t path) {
      memcpy(keyBuffer, path, keySize);
    }



    void fillKeyBuffer (CubePage::key_t keyBuffer, const CellPath * path) {
      fillKeyBuffer(keyBuffer, path->getPathIdentifier());
    }



    CubePage * lookupCubePage (CubePage::key_t key) {
      size_t length = maxima.size();

      if (length <= 1) {
        return lookupCubePage(0, 0);
      }

      IdentifierType id1 = ((IdentifierType*) key)[firstDimension];

      if (length == 2) {
        return lookupCubePage(id1, 0);
      }

      IdentifierType id2 = ((IdentifierType*) key)[secondDimension];

      return lookupCubePage(id1, id2);
    }



    CubePage * findCubePage (IdentifierType id1, IdentifierType id2) {
      CubePage * page = lookupCubePage(id1, id2);

      if (page == 0) {
        size_t length = maxima.size();

        if (length < 2) {
          throw new ErrorException(ErrorException::ERROR_INTERNAL,
                                   "in findCubePage for empty cube");
        }

        if (length == 2) {

          // ignore the first key entry
          page = new CubePage(index,
                              keySize,
                              valueSize,
                              id1,
                              0);

          if (id1 >= endIdentifier1) {
            endIdentifier1 = id1 + 1;
          }
        }
        else {

          // ignore the first two key entries
          page = new CubePage(index,
                              keySize,
                              valueSize,
                              id1,
                              id2);

          if (id1 >= endIdentifier1) {
            endIdentifier1 = id1 + 1;
          }

          if (id2 >= endIdentifier2) {
            endIdentifier2 = id2 + 1;
          }
        }

        cubePages.addElement(page);
      }

      return page;
    }



    CubePage * findCubePage (CubePage::key_t key) {
      size_t length = maxima.size();

      if (length <= 1) {
        return findCubePage(0, 0);
      }

      IdentifierType id1 = ((IdentifierType*) key)[firstDimension];

      if (length == 2) {
        return findCubePage(id1, 0);
      }

      IdentifierType id2 = ((IdentifierType*) key)[secondDimension];

      return findCubePage(id1, id2);
    }

  public:
    bool isEqualBuffer (uint8_t * buffer, uint8_t * keyBuffer, uint8_t * maskBuffer);

    double computeConsolidatedValue (CubePage * page,
                                     CubePage::buffer_t start,
                                     CubePage::buffer_t stop,
                                     vector<IdentifiersWeightType>::const_iterator base,
                                     size_t minimalDimension,
                                     size_t dimension,
                                     double weight,
                                     bool * found,
                                     size_t *numFoundElements,
                                     User*);
    
    void setConsolidatedValue (CubePage * page,
                               CubePage::buffer_t start,
                               CubePage::buffer_t stop,
                               vector<IdentifiersWeightType>::const_iterator base,
                               size_t minimalDimension,
                               size_t dimension,
                               double factor,
                               Lock* lock);

  private:
    bool isMarked (CubePage::element_t element) {
      return ((* (uint32_t*) (element + valueSize + keySize)) & 0x80000000) != 0;
    }
    
    void setMarker (CubePage::element_t element) {
      * (uint32_t*)(element + valueSize + keySize) |= 0x80000000;
      * (uint32_t*)(element + valueSize + keySize) &= ~0x40000000;
    }
    
    void setDeleted (CubePage::element_t element) {
      * (uint32_t*)(element + valueSize + keySize) |= 0x40000000;
      * (uint32_t*)(element + valueSize + keySize) &= ~0x80000000;
    }
    
    void clearMarkerAndDeleted (CubePage::element_t element) {
      * (uint32_t*)(element + valueSize + keySize) &= ~0xc0000000;
    }

  private:

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief underlying cube
    ////////////////////////////////////////////////////////////////////////////////
    
    Cube* cube;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief first dimension
    ////////////////////////////////////////////////////////////////////////////////
    
    size_t firstDimension;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief second dimension
    ////////////////////////////////////////////////////////////////////////////////
    
    size_t secondDimension;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief total number of elements
    ////////////////////////////////////////////////////////////////////////////////
    
    size_t numberElements;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief maximum possible value for first dimension identifier + 1
    ////////////////////////////////////////////////////////////////////////////////
    
    IdentifierType endIdentifier1;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief maximum possible value for second dimension identifier + 1
    ////////////////////////////////////////////////////////////////////////////////
    
    IdentifierType endIdentifier2;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief maximum possible value for dimension identifier
    ////////////////////////////////////////////////////////////////////////////////
    
    vector<size_t> maxima;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief list of used pages
    ////////////////////////////////////////////////////////////////////////////////
    
    AssociativeArray<PageKey const *, CubePage *, PageDesc> cubePages;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief memory size needed to store a path 
    ////////////////////////////////////////////////////////////////////////////////
    
    size_t keySize;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief memory size needed to store a value
    ////////////////////////////////////////////////////////////////////////////////
    
    size_t valueSize;

    ////////////////////////////////////////////////////////////////////////////////
    /// @brief memory size needed to store the path and the value
    ////////////////////////////////////////////////////////////////////////////////
    
    size_t elementSize;
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief Pointer to a cube index object
    ////////////////////////////////////////////////////////////////////////////////
    
    CubeIndex * index;
    
    ////////////////////////////////////////////////////////////////////////////////
    /// @brief true if the cube stores character pointers
    ////////////////////////////////////////////////////////////////////////////////
    
    bool isString;

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
