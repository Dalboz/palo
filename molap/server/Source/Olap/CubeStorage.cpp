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

#include "Olap/CubeStorage.h"

#include "Exceptions/KeyBufferException.h"

#include "InputOutput/Logger.h"
#include "InputOutput/Statistics.h"

#include "Olap/AreaStorage.h"
#include "Olap/CellPath.h"
#include "Olap/CubeIndex.h"
#include "Olap/Dimension.h"
#include "Olap/ExportStorage.h"
#include "Olap/HashAreaStorage.h"
#include "Olap/Lock.h"
#include "Olap/MarkerStorage.h"
#include "Olap/RollbackPage.h"

#include "Parser/AreaNode.h"

#include <limits>

#include <iostream>

#include <math.h>

#if defined(_MSC_VER)
#include <float.h>
#endif

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  // constructors and destructors
  ////////////////////////////////////////////////////////////////////////////////

  CubeStorage::CubeStorage (Cube* cube,
                            const vector<size_t>* sizeDimensions,
                            size_t valueSize, 
                            bool isStringStorage)
    : cube(cube),
      firstDimension(0),
      secondDimension(1),
      numberElements(0),
      endIdentifier1(0),
      endIdentifier2(0),
      cubePages(1000, PageDesc()),
      valueSize(valueSize),
      isString(isStringStorage) {
    maxima.clear();
    
    size_t totalNumberBits = 0;
      
    for (vector<size_t>::const_iterator i = sizeDimensions->begin();
         i != sizeDimensions->end();
         i++) {
      uint32_t bitsDimension = 32;
      uint32_t maximum = ~0;

      maxima.push_back(maximum);
      totalNumberBits += bitsDimension;
    }
    
    keySize = ((totalNumberBits + 31) / 32) * 4; // normalise to 32 bit, convert to number of bytes
    elementSize = keySize + valueSize;

    Logger::trace << "creating new CubeStorage: key size = " << keySize 
                  << ", value size = " <<  valueSize << endl;
         
    tmpKeyBuffer = new uint8_t [keySize];
    tmpElementBuffer = new uint8_t [elementSize];

    // generate index for base elements
    index = new CubeIndex(keySize, valueSize);

    // special case: one or two dimensions
    if (sizeDimensions->size() == 1) {
      endIdentifier1 = 1;
      endIdentifier2 = 1;

      CubePage * page = new CubePage(index, keySize, valueSize, 0, 0);
      cubePages.addElement(page);
    }
    else if (sizeDimensions->size() == 2) {
      endIdentifier2 = 1;
    }
  }



  CubeStorage::~CubeStorage () {
    delete[] tmpKeyBuffer;
    delete[] tmpElementBuffer;

    // delete strings
    if (isString) {
      size_t size;
      CubePage::element_t const * array = getArray(size);

      for (size_t i = 0;  i < size;  array++) {
        char ** ptr = (char**) (*array);

        if (ptr != 0) {
          delete[] *ptr;
          i++;
        }
      }
    }
    
    // delete pages
    CubePage * const * cubePageArray = cubePages.getTable();
    size_t size = cubePages.size();

    for (size_t i = 0;  i < size;  cubePageArray++) {
      CubePage * page = *cubePageArray;

      if (page != 0) {
        delete page;
        i++;
      }
    }

    delete index;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // get cell values
  ////////////////////////////////////////////////////////////////////////////////

  double CubeStorage::getConsolidatedCellValue (const vector<IdentifiersWeightType> * baseElements,
                                                bool* found,
                                                size_t * numFoundElements,
                                                User* user) {

    // convert the base elements to the corresponding keys and get the masks
    uint32_t length = (uint32_t) baseElements->size();

    // return whether we found an non-empty cell
    *found = false;

    // sum up result
    double result = 0;

    // special case of degenerated cube, no dimension at all
    if (length == 0) {
      return result;
    }

    // special case of degenerated cube, just one dimension
    else if (length == 1) {
      CubePage * page = lookupCubePage(0, 0);

      if (page == 0) {
        return 0;
      }

      page->sort();

      for (IdentifiersWeightType::const_iterator i = (*baseElements)[0].begin();
           i != (*baseElements)[0].end();
           i++) {
        const pair<IdentifierType, double>& ew = *i;
        
        CubePage::buffer_t first = page->begin();
        CubePage::buffer_t last  = page->end();

        page->equal_range(&first, &last, ew.first, valueSize);

        if (first != last) {
          if (isMarked(first)) {
            double value = cube->computeRule(first + valueSize, *(double*) first, user);
            result += ew.second * value;
          }
          else {
            result += ew.second * (* (double*) first);
          }

          (*numFoundElements)++;
          *found = true; 
        }
      }
    }

    // special case of array, just two dimensions
    else if (length == 2) {
      for (IdentifiersWeightType::const_iterator i = (*baseElements)[0].begin();
           i != (*baseElements)[0].end();
           i++) {
        const pair<IdentifierType, double>& ew1 = *i;
        IdentifierType id1 = ew1.first;

        CubePage * page = lookupCubePage(id1, 0);

        if (page == 0) {
          continue;
        }

        page->sort();

        for (IdentifiersWeightType::const_iterator j = (*baseElements)[1].begin();
             j != (*baseElements)[1].end();
             j++) {
          const pair<IdentifierType, double>& ew2 = *j;
          IdentifierType id2 = ew2.first;

          CubePage::buffer_t first = page->begin();
          CubePage::buffer_t last  = page->end();

          page->equal_range(&first, &last, id2, valueSize + 4);

          if (first != last) {
            if (isMarked(first)) {
              double value = cube->computeRule(first + valueSize, *(double*) first, user);
              result += ew1.second * ew2.second * value;
            }
            else {
              result += ew1.second * ew2.second * (* (double*) first);
            }

            *found = true;
            (*numFoundElements)++;
          }
        }
      }
    }


    // more than two dimensions, use recursion
    else {
      size_t minimalDimension = 0;

      while (firstDimension == minimalDimension || secondDimension == minimalDimension) {
        minimalDimension++;
      }

      for (IdentifiersWeightType::const_iterator i = (*baseElements)[firstDimension].begin();
           i != (*baseElements)[firstDimension].end();
           i++) {
        const pair<IdentifierType, double>& ew1 = *i;
        IdentifierType id1 = ew1.first;

        if (id1 >= endIdentifier1) {
          return result;
        }
        
        for (IdentifiersWeightType::const_iterator j = (*baseElements)[secondDimension].begin();
             j != (*baseElements)[secondDimension].end();
             j++) {
          const pair<IdentifierType, double>& ew2 = *j;
          IdentifierType id2 = ew2.first;

          if (id2 >= endIdentifier2) {
            break;
          }

          CubePage * page = lookupCubePage(id1, id2);

          if (page != 0) {
            page->sort();

            result += computeConsolidatedValue(page,
                                               page->begin(),
                                               page->end(),
                                               baseElements->end() - 1,
                                               minimalDimension,
                                               length - 1, 
                                               ew1.second * ew2.second,
                                               found,
                                               numFoundElements,
                                               user);
          }
        }
      }
    }

    return result;
  }



  double CubeStorage::computeConsolidatedValue (CubePage * page,
                                                CubePage::buffer_t start,
                                                CubePage::buffer_t stop,
                                                vector<IdentifiersWeightType>::const_iterator base,
                                                size_t minimalDimension,
                                                size_t dimension,
                                                double weight,
                                                bool * found,
                                                size_t *numFoundElements,
                                                User* user) {
    if (dimension == firstDimension || dimension == secondDimension) {
      return computeConsolidatedValue(page,
                                      start,
                                      stop,
                                      base - 1,
                                      minimalDimension,
                                      dimension - 1,
                                      weight,
                                      found,
                                      numFoundElements,
                                      user);
    }

    // offset to current dimension
    int offset = (int)(valueSize + dimension * 4);

    // add result
    double result = 0;

    // loop over all keys
    const IdentifiersWeightType&          keys     = *base;
    IdentifiersWeightType::const_iterator keysIter = keys.begin();

    // we have not reached the value, descend further
    if (minimalDimension < dimension) {
      for (;  keysIter != keys.end();  keysIter++) {
        if (start == stop) {
          return result;
        }

        const pair<IdentifierType, double>& val = *keysIter;
        IdentifierType id = val.first;
        IdentifierType current = * (uint32_t*) (start + offset);
        
        if (id == current) {
          CubePage::buffer_t right = page->upper_bound(start, stop, id, offset);

          result += computeConsolidatedValue(page,
                                             start,
                                             right,
                                             base - 1,
                                             minimalDimension,
                                             dimension - 1,
                                             weight * val.second,
                                             found,
                                             numFoundElements,
                                             user);
          
          start = right;
        }
        else if (id > current) {
          CubePage::buffer_t left = start;
          CubePage::buffer_t right = stop;

          page->equal_range(&left, &right, id, offset);

          if (left != right) {
            result += computeConsolidatedValue(page,
                                               left,
                                               right,
                                               base - 1,
                                               minimalDimension,
                                               dimension - 1,
                                               weight * val.second,
                                               found,
                                               numFoundElements,
                                               user);
          }
          
          start = right;
        }
        else { /* (id < current) */
          for (; keysIter != keys.end(); keysIter++) {
            id = (*keysIter).first;

            if (id >= current) {
              break;
            }
          }

          keysIter--;
        }
      }
    }

    // we have reached the value, sum up
    else {
      for (; keysIter != keys.end();) {
        const pair<IdentifierType, double> * val = &(*keysIter);
        IdentifierType id = val->first;
        
        CubePage::buffer_t f = page->lower_bound(start, stop, id, offset);

        if (f == stop) {
          return result;
        }
        else {
          uint32_t nid = * (uint32_t*) (f + offset);
          
          if (nid == id) {
            size_t rowSize = page->getRowSize();
            do {
              if (isMarked(f)) {
                double value = cube->computeRule(f + valueSize, *(double*) f, user);
                result += weight * val->second * value;
              }
              else {
                result += weight * val->second * (* (double*) f);
              }

              *found  = true;
              (*numFoundElements)++;
              f = f + rowSize;
              
              if (f == stop) {
                return result;
              }
              
              keysIter++;
              
              if (keysIter == keys.end()) {
                return result;
              }
              
              nid = * (uint32_t*) (f + offset);
              val = &(*keysIter);
              id  = val->first;
            }
            while (nid == id);
          }
          else {
            keysIter++;
          }

          start = f;
        }
      }
    }
    
    return result;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // set markers
  ////////////////////////////////////////////////////////////////////////////////

  void CubeStorage::setMarkers (const AreaNode::Area * baseElements,
                                MarkerStorage* markers) {

    // convert the base elements to the corresponding keys and get the masks
    uint32_t length = (uint32_t) baseElements->size();

    // special case of degenerated cube, no dimension at all
    if (length == 0) {
      return;
    }

    // special case of degenerated cube, just one dimension
    else if (length == 1) {
      CubePage * page = lookupCubePage(0, 0);

      if (page == 0) {
        return;
      }

      page->sort();

      const set<IdentifierType>& ids = (*baseElements)[0];

      if (ids.empty()) {
        CubePage::buffer_t first = page->begin();
        CubePage::buffer_t last  = page->end();

        size_t rowSize = page->getRowSize();

        for (;  first < last;  first += rowSize) {
          markers->setCellValue(first + valueSize);
        }
      }
      else {
        for (set<IdentifierType>::const_iterator i = ids.begin();  i != ids.end();  i++) {
          IdentifierType id = *i;
          
          CubePage::buffer_t first = page->begin();
          CubePage::buffer_t last  = page->end();
          
          page->equal_range(&first, &last, id, valueSize);
          
          if (first != last) {
            markers->setCellValue(first + valueSize);
          }
        }
      }
    }

    // special case of array, just two dimensions
    else if (length == 2) {
      const set<IdentifierType>& ids = (*baseElements)[0];

      if (ids.empty()) {
        for (IdentifierType id1 = 0;  id1 < endIdentifier1;  id1++) {
          CubePage * page = lookupCubePage(id1, 0);

          if (page == 0) {
            continue;
          }

          page->sort();

          const set<IdentifierType>& ids1 = (*baseElements)[1];

          if (ids1.empty()) {
            CubePage::buffer_t first = page->begin();
            CubePage::buffer_t last  = page->end();

            for (;  first < last;  first += page->getRowSize()) {
              markers->setCellValue(first + valueSize);
            }
          }
          else {
            for (set<IdentifierType>::const_iterator j = ids.begin();  j != ids.end();  j++) {
              IdentifierType id2 = *j;

              CubePage::buffer_t first = page->begin();
              CubePage::buffer_t last  = page->end();

              page->equal_range(&first, &last, id2, valueSize + 4);
              
              if (first != last) {
                markers->setCellValue(first + valueSize);
              }
            }
          }
        }
      }
      else {
        for (set<IdentifierType>::const_iterator i = ids.begin();  i != ids.end();  i++) {
          IdentifierType id1 = *i;

          CubePage * page = lookupCubePage(id1, 0);

          if (page == 0) {
            continue;
          }

          page->sort();

          const set<IdentifierType>& ids1 = (*baseElements)[1];

          if (ids1.empty()) {
            CubePage::buffer_t first = page->begin();
            CubePage::buffer_t last  = page->end();

            for (;  first < last;  first += page->getRowSize()) {
              markers->setCellValue(first + valueSize);
            }
          }
          else {
            for (set<IdentifierType>::const_iterator j = ids.begin();  j != ids.end();  j++) {
              IdentifierType id2 = *j;

              CubePage::buffer_t first = page->begin();
              CubePage::buffer_t last  = page->end();

              page->equal_range(&first, &last, id2, valueSize + 4);
              
              if (first != last) {
                markers->setCellValue(first + valueSize);
              }
            }
          }
        }
      }
    }


    // more than two dimensions, use recursion
    else {
      size_t minimalDimension = 0;

      while (firstDimension == minimalDimension || secondDimension == minimalDimension) {
        minimalDimension++;
      }

      const set<IdentifierType>& ids = (*baseElements)[firstDimension];

      if (ids.empty()) {
        for (IdentifierType id1 = 0;  id1 < endIdentifier1;  id1++) {
          const set<IdentifierType>& ids1 = (*baseElements)[secondDimension];

          if (ids1.empty()) {
            for (IdentifierType id2 = 0;  id2 < endIdentifier2;  id2++) {
              CubePage * page = lookupCubePage(id1, id2);

              if (page != 0) {
                page->sort();

                setMarkersRecursively(page,
                                      page->begin(),
                                      page->end(),
                                      baseElements->end() - 1,
                                      minimalDimension,
                                      length - 1,
                                      markers);
              }
            }
          }
          else {
            for (set<IdentifierType>::const_iterator j = ids1.begin();  j != ids1.end();  j++) {
              IdentifierType id2 = *j;

              if (id2 >= endIdentifier2) {
                break;
              }

              CubePage * page = lookupCubePage(id1, id2);

              if (page != 0) {
                page->sort();

                setMarkersRecursively(page,
                                      page->begin(),
                                      page->end(),
                                      baseElements->end() - 1,
                                      minimalDimension,
                                      length - 1,
                                      markers);
              }
            }
          }
        }
      }
      else {
        for (set<IdentifierType>::const_iterator i = ids.begin();  i != (*baseElements)[firstDimension].end(); i++) {
          IdentifierType id1 = *i;

          if (id1 >= endIdentifier1) {
            return;
          }

          const set<IdentifierType>& ids1 = (*baseElements)[secondDimension];

          if (ids1.empty()) {
            for (IdentifierType id2 = 0;  id2 < endIdentifier2;  id2++) {
              CubePage * page = lookupCubePage(id1, id2);

              if (page != 0) {
                page->sort();

                setMarkersRecursively(page,
                                      page->begin(),
                                      page->end(),
                                      baseElements->end() - 1,
                                      minimalDimension,
                                      length - 1,
                                      markers);
              }
            }
          }
          else {
            for (set<IdentifierType>::const_iterator j = ids1.begin();  j != ids1.end();  j++) {
              IdentifierType id2 = *j;

              if (id2 >= endIdentifier2) {
                break;
              }

              CubePage * page = lookupCubePage(id1, id2);

              if (page != 0) {
                page->sort();

                setMarkersRecursively(page,
                                      page->begin(),
                                      page->end(),
                                      baseElements->end() - 1,
                                      minimalDimension,
                                      length - 1,
                                      markers);
              }
            }
          }
        }
      }
    }
  }



  void CubeStorage::setMarkersRecursively (CubePage * page,
                                           CubePage::buffer_t start,
                                           CubePage::buffer_t stop,
                                           AreaNode::Area::const_iterator base,
                                           size_t minimalDimension,
                                           size_t dimension,
                                           MarkerStorage* markers) {
    if (dimension == firstDimension || dimension == secondDimension) {
      return setMarkersRecursively(page,
                                   start,
                                   stop,
                                   base - 1,
                                   minimalDimension,
                                   dimension - 1,
                                   markers);
    }

    // offset to current dimension
    int offset = (int)(valueSize + dimension * 4);

    // add result
    double result = 0;

    // loop over all keys
    const set<IdentifierType>& keys = *base;

    // we have not reached the value, descend further
    if (minimalDimension < dimension) {
      if (keys.empty()) {
        while (start != stop) {
          IdentifierType current = * (uint32_t*) (start + offset);
          CubePage::buffer_t right = page->upper_bound(start, stop, current, offset);

          setMarkersRecursively(page,
                                start,
                                right,
                                base - 1,
                                minimalDimension,
                                dimension - 1,
                                markers);

          start = right;
        }
      }
      else {
        for (set<IdentifierType>::const_iterator keysIter = keys.begin();
             keysIter != keys.end();
             keysIter++) {
          if (start == stop) {
            return;
          }

          IdentifierType id = *keysIter;
          IdentifierType current = * (uint32_t*) (start + offset);
          
          if (id == current) {
            CubePage::buffer_t right = page->upper_bound(start, stop, id, offset);
            
            setMarkersRecursively(page,
                                  start,
                                  right,
                                  base - 1,
                                  minimalDimension,
                                  dimension - 1,
                                  markers);
            
            start = right;
          }
          else if (id > current) {
            CubePage::buffer_t left = start;
            CubePage::buffer_t right = stop;
            
            page->equal_range(&left, &right, id, offset);
            
            if (left != right) {
              setMarkersRecursively(page,
                                    left,
                                    right,
                                    base - 1,
                                    minimalDimension,
                                    dimension - 1,
                                    markers);
            }
            
            start = right;
          }
          else { /* (id < current) */
            for (; keysIter != keys.end(); keysIter++) {
              id = *keysIter;
              
              if (id >= current) {
                break;
              }
            }
            
            keysIter--;
          }
        }
      }
    }

    // we have reached the value, sum up
    else {
      if (keys.empty()) {
        size_t rowSize = page->getRowSize();

        for (CubePage::buffer_t f = start;  f < stop;  f += rowSize) {
          markers->setCellValue(f + valueSize);
        }
      }
      else {
        set<IdentifierType>::const_iterator keysIter = keys.begin();

        for (; keysIter != keys.end();) {
          IdentifierType id = *keysIter;
          
          CubePage::buffer_t f = page->lower_bound(start, stop, id, offset);
          
          if (f == stop) {
            return;
          }
          else {
            uint32_t nid = * (uint32_t*) (f + offset);
            
            if (nid == id) {
              size_t rowSize = page->getRowSize();

              do {
                markers->setCellValue(f + valueSize);
                
                f = f + rowSize;
                
                if (f == stop) {
                  return;
                }
                
                keysIter++;
                
                if (keysIter == keys.end()) {
                  return;
                }
                
                nid = * (uint32_t*) (f + offset);
                id  = *keysIter;
              }
              while (nid == id);
            }
            else {
              keysIter++;
            }

            start = f;
          }
        }
      }
    }
  }



  void CubeStorage::clearAllMarkers () {
    CubePage * const * cubePageArray = cubePages.getTable(); 
    size_t size = cubePages.size();

    for (size_t i = 0;  i < size;  cubePageArray++) {
      CubePage * page = *cubePageArray;

      if (page != 0) {
        CubePage::buffer_t first = page->begin();
        CubePage::buffer_t last  = page->end();

        while (first < last) {
          if (isMarked(first)) {
            if (*(double*)first == 0.0) {
              setDeleted(first);
              page->clearSorted();
            }
            else {
              clearMarkerAndDeleted(first);
            }
          }

          first += page->getRowSize();
        }

        i++;
      }
    }
    
  }

  ////////////////////////////////////////////////////////////////////////////////
  // set cell values
  ////////////////////////////////////////////////////////////////////////////////

  void CubeStorage::setConsolidatedCellValue (const vector<IdentifiersWeightType> * baseElements,
                                              double factor,
                                              Lock* lock) {

    // convert the base elements to the corresponding keys and get the masks
    uint32_t length = (uint32_t) baseElements->size();

    // special case of degenerated cube, no dimension at all
    if (length == 0) {
      return;
    }

    // special case of degenerated cube, just one dimension
    else if (length == 1) {
      CubePage * page = lookupCubePage(0, 0);

      if (page == 0) {
        return;
      }

      page->sort();

      for (IdentifiersWeightType::const_iterator i = (*baseElements)[0].begin();
           i != (*baseElements)[0].end();
           i++) {
        const pair<IdentifierType, double>& ew = *i;
        
        CubePage::buffer_t first = page->begin();
        CubePage::buffer_t last  = page->end();

        page->equal_range(&first, &last, ew.first, valueSize);

        if (first != last) {
          if (lock) {
            IdentifiersType path(1);
            fillPath (first, &path);        
            lock->getStorage()->addCellValue(&path, (double*) first); 
          }

          * (double*) first *= factor;
          clearMarkerAndDeleted(first);
        }
      }
    }

    // special case of array, just two dimensions
    else if (length == 2) {
      for (IdentifiersWeightType::const_iterator i = (*baseElements)[0].begin();
           i != (*baseElements)[0].end();
           i++) {
        const pair<IdentifierType, double>& ew1 = *i;
        IdentifierType id1 = ew1.first;

        CubePage * page = lookupCubePage(id1, 0);

        if (page == 0) {
          continue;
        }

        page->sort();

        for (IdentifiersWeightType::const_iterator j = (*baseElements)[1].begin();
             j != (*baseElements)[1].end();
             j++) {
          const pair<IdentifierType, double>& ew2 = *j;
          IdentifierType id2 = ew2.first;

          CubePage::buffer_t first = page->begin();
          CubePage::buffer_t last  = page->end();

          page->equal_range(&first, &last, id2, valueSize + 4);

          if (first != last) {
            if (lock) {
              IdentifiersType path(2);
              fillPath (first, &path);        
              lock->getStorage()->addCellValue(&path, (double*) first); 
            }

            * (double*) first *= factor;
            clearMarkerAndDeleted(first);
          }
        }
      }
    }


    // more than two dimensions, use recursion
    else {
      size_t minimalDimension = 0;

      while (firstDimension == minimalDimension || secondDimension == minimalDimension) {
        minimalDimension++;
      }

      for (IdentifiersWeightType::const_iterator i = (*baseElements)[firstDimension].begin();
           i != (*baseElements)[firstDimension].end();
           i++) {
        const pair<IdentifierType, double>& ew1 = *i;
        IdentifierType id1 = ew1.first;

        if (id1 >= endIdentifier1) {
          return;
        }
        
        for (IdentifiersWeightType::const_iterator j = (*baseElements)[secondDimension].begin();
             j != (*baseElements)[secondDimension].end();
             j++) {
          const pair<IdentifierType, double>& ew2 = *j;
          IdentifierType id2 = ew2.first;

          if (id2 >= endIdentifier2) {
            break;
          }

          CubePage * page = lookupCubePage(id1, id2);

          if (page != 0) {
            page->sort();

            setConsolidatedValue(page,
                                 page->begin(),
                                 page->end(),
                                 baseElements->end() - 1,
                                 minimalDimension,
                                 length - 1, 
                                 factor,
                                 lock);
          }
        }
      }
    }
  }



  void CubeStorage::setConsolidatedValue (CubePage * page,
                                                CubePage::buffer_t start,
                                                CubePage::buffer_t stop,
                                                vector<IdentifiersWeightType>::const_iterator base,
                                                size_t minimalDimension,
                                                size_t dimension,
                                                double factor,
                                                Lock* lock) {
    if (dimension == firstDimension || dimension == secondDimension) {
      setConsolidatedValue(page,
                                      start,
                                      stop,
                                      base - 1,
                                      minimalDimension,
                                      dimension - 1,
                                      factor,
                                      lock);
      return;
    }

    // offset to current dimension
    int offset = (int)(valueSize + dimension * 4);

    // loop over all keys
    const IdentifiersWeightType&          keys     = *base;
    IdentifiersWeightType::const_iterator keysIter = keys.begin();

    // we have not reached the value, descend further
    if (minimalDimension < dimension) {
      for (;  keysIter != keys.end();  keysIter++) {
        if (start == stop) {
          return;
        }

        const pair<IdentifierType, double>& val = *keysIter;
        IdentifierType id = val.first;
        IdentifierType current = * (uint32_t*) (start + offset);
        
        if (id == current) {
          CubePage::buffer_t right = page->upper_bound(start, stop, id, offset);

          setConsolidatedValue(page,
                               start,
                               right,
                               base - 1,
                               minimalDimension,
                               dimension - 1,
                               factor,
                               lock);
          
          start = right;
        }
        else if (id > current) {
          CubePage::buffer_t left = start;
          CubePage::buffer_t right = stop;

          page->equal_range(&left, &right, id, offset);

          if (left != right) {
            setConsolidatedValue(page,
                                 left,
                                 right,
                                 base - 1,
                                 minimalDimension,
                                 dimension - 1,
                                 factor,
                                 lock);
          }
          
          start = right;
        }
        else { /* (id < current) */
          for (; keysIter != keys.end(); keysIter++) {
            id = (*keysIter).first;

            if (id >= current) {
              break;
            }
          }

          keysIter--;
        }
      }
    }

    // we have reached the value, sum up
    else {
      for (; keysIter != keys.end();) {
        const pair<IdentifierType, double> * val = &(*keysIter);
        IdentifierType id = val->first;
        
        CubePage::buffer_t f = page->lower_bound(start, stop, id, offset);

        if (f == stop) {
          return;
        }
        else {
          uint32_t nid = * (uint32_t*) (f + offset);
          
          if (nid == id) {
            size_t rowSize = page->getRowSize();
            do {
              if (lock) {
                lock->getStorage()->addCellValue((RollbackPage::buffer_t) f); 
              }

              * (double*) f *= factor;
              clearMarkerAndDeleted(f);

              f = f + rowSize;
              
              if (f == stop) {
                return;
              }
              
              keysIter++;
              
              if (keysIter == keys.end()) {
                return;
              }
              
              nid = * (uint32_t*) (f + offset);
              val = &(*keysIter);
              id  = val->first;
            }
            while (nid == id);
          }
          else {
            keysIter++;
          }

          start = f;
        }
      }
    }    
  }

  ////////////////////////////////////////////////////////////////////////////////
  // area get cell values
  ////////////////////////////////////////////////////////////////////////////////


  void CubeStorage::computeAreaCellValue (AreaStorage* storage,
                                          vector< map<IdentifierType, map<IdentifierType, double> > > *baseElements,
                                          User* user) {

    // convert the base elements to the corresponding keys and get the masks
    uint32_t length = (uint32_t) baseElements->size();

    // special case of degenerated cube, no dimension at all
    if (length == 0) {
      return;
    }
    
    // special case of degenerated cube, just one dimension
    else if (length == 1) {
      CubePage * page = lookupCubePage(0, 0);

      if (page == 0) {
        return;
      }

      page->sort();

      vector< map<IdentifierType, double>* > mappings(1);

      for (map<IdentifierType, map<IdentifierType, double> >::iterator i = (*baseElements)[0].begin();
           i != (*baseElements)[0].end();
           i++) {
        IdentifierType id = i->first;
        
        CubePage::buffer_t first = page->begin();
        CubePage::buffer_t last  = page->end();

        page->equal_range(&first, &last, id, valueSize);

        if (first != last) {
          mappings[0] = &i->second;

          double value;

          if (isMarked(first)) {
            value = cube->computeRule(first + valueSize, *(double*) first, user);
          }
          else {
            value = * (double*) first;
          }

          setAreaValue(storage, value, mappings);
        }
      }
    }

    // special case of array, just two dimensions
    else if (length == 2) {
      vector< map<IdentifierType, double>* > mappings(2);
      
      for (map<IdentifierType, map<IdentifierType, double> >::iterator i = (*baseElements)[0].begin();
           i != (*baseElements)[0].end();
           i++) {
            
        mappings[0] = &i->second;        
        IdentifierType id1 = i->first;

        CubePage * page = lookupCubePage(id1, 0);

        if (page == 0) {
          continue;
        }

        page->sort();

        for (map<IdentifierType, map<IdentifierType, double> >::iterator j = (*baseElements)[1].begin();
             j != (*baseElements)[1].end();
             j++) {
          mappings[1] = &j->second;        
          IdentifierType id2 = j->first;

          CubePage::buffer_t first = page->begin();
          CubePage::buffer_t last  = page->end();

          page->equal_range(&first, &last, id2, valueSize + 4);

          if (first != last) {
            double value;

            if (isMarked(first)) {
              value = cube->computeRule(first + valueSize, *(double*) first, user);
            }
            else {
              value = * (double*) first;
            }

            setAreaValue(storage, value, mappings);
          }
        }
      }
    }

    // more than two dimensions, use recursion
    else {
      size_t minimalDimension = 0;      

      while (firstDimension == minimalDimension || secondDimension == minimalDimension) {
        minimalDimension++;
      }
      
      vector< map<IdentifierType, double>* > mappings(length);
      
      for (map<IdentifierType, map<IdentifierType, double> >::iterator i = (*baseElements)[firstDimension].begin();
           i != (*baseElements)[firstDimension].end();
           i++) {

        IdentifierType id1 = i->first;

        mappings[0] = &i->second;        

        if (id1 >= endIdentifier1) {
          break;
        }
        
        for (map<IdentifierType, map<IdentifierType, double> >::iterator j = (*baseElements)[secondDimension].begin();
             j != (*baseElements)[secondDimension].end();
             j++) {
          IdentifierType id2 = j->first;

          if (id2 >= endIdentifier2) {
            break;
          }

          CubePage * page = lookupCubePage(id1, id2);

          if (page != 0) {
            page->sort();
            mappings[1] = &j->second;        
            computeAreaCellValue(storage,
                                 page,
                                 baseElements,
                                 minimalDimension,
                                 length - 1,
                                 mappings,
                                 user);
          }
        }
      }
    }
  }


  void CubeStorage::computeAreaCellValue (AreaStorage* storage,
                                          CubePage * page,
                                          vector< map<IdentifierType, map<IdentifierType, double> > >* baseElements,
                                          size_t minimalDimension,
                                          size_t dimension,
                                          vector< map<IdentifierType, double>* >& mappings,
                                          User* user) {
    CubePage::buffer_t start = page->begin();
    CubePage::buffer_t stop = page->end();
    size_t rowSize = page->getRowSize();

    uint32_t lookUp = (uint32_t) mappings.size() - 1;
                                            
    for (CubePage::buffer_t row = start;  row < stop;  row += rowSize) {
      uint32_t changeNum  = (* (uint32_t*) (row + valueSize + keySize)) & 0x3fffffff;
      
      if (changeNum >= lookUp) {
        bool found = true;
        
        IdentifierType* idPtr  = (IdentifierType*) (row + valueSize);

        for (uint32_t i = changeNum; i > 1; i--) {
          lookUp = i;
          map<IdentifierType, map<IdentifierType, double> >& base = (*baseElements)[i];
          map<IdentifierType, map<IdentifierType, double> >::iterator x = base.find(* (idPtr+i) );

          if (x == base.end()) {
            found = false;
            break;
          }

          mappings[i] = &x->second;
        }
        
        if (found) {
          double value;

          if (isMarked(row)) {
            value = cube->computeRule(row + valueSize, *(double*) row, user);
          }
          else {
            value = * (double*) row;
          }

          setAreaValue(storage, value, mappings);
        }
      }
    }    
  }

  

  void CubeStorage::setAreaValue (AreaStorage* storage,
                                  double value,
                                  vector< map<IdentifierType, double>* >& mappings) {
    IdentifiersType path(mappings.size());    
    IdentifierType * buffer = (IdentifierType*) tmpKeyBuffer;
    
    size_t max = mappings.size();
    vector< map<IdentifierType, double>::iterator > iv;

    for (size_t i = 0; i < max; i++) {
      iv.push_back( mappings[i]->begin() );
      * (buffer+i) = iv[i]->first;
    }

    size_t pos = 0;

    do {
      double v = value;

      for (size_t i = 0; i < max; i++) {
        v *= iv[i]->second;
      }

      storage->addDoubleValue(tmpKeyBuffer, v);
      
      bool stop = false;
      pos = 0;      

      while (!stop && pos < max) {
        iv[pos]++;

        if (iv[pos] == mappings[pos]->end()) {
          iv[pos] = mappings[pos]->begin();
          * (buffer+pos) = iv[pos]->first;
          pos++;
        }
        else {
          * (buffer+pos) = iv[pos]->first;
          stop = true;
        }
      }
      
    }
    while (pos < max); 
  }
  


  void CubeStorage::computeHashAreaCellValue (HashAreaStorage* storage,
                                              vector< vector< vector< pair<uint32_t, double> > > >& numericMapping,
                                              User* user) {
    

    // convert the base elements to the corresponding keys and get the masks
    uint32_t length = (uint32_t) numericMapping.size();

    // special case of degenerated cube, no dimension at all
    if (length == 0) {
      return;
    }
    
    // special case of degenerated cube, just one dimension
    else if (length == 1) {
      throw ErrorException(ErrorException::ERROR_INTERNAL, "computeHashAreaCellValue for one dimension called");
    }

    // special case of array, just two dimensions
    else if (length == 2) {
      throw ErrorException(ErrorException::ERROR_INTERNAL, "computeHashAreaCellValue for two dimensions called");
    }

    // more than two dimensions, handle cases page by page
    else {
      size_t minimalDimension = 0;      

      while (firstDimension == minimalDimension || secondDimension == minimalDimension) {
        minimalDimension++;
      }

      vector< vector< pair<uint32_t, double> >* > mappings(numericMapping.size(), 0);
      
      for (size_t i = 0;  i < endIdentifier1 && i < numericMapping[firstDimension].size();  i++) {
        if (numericMapping[firstDimension][i].empty()) {
          continue;
        }

        mappings[firstDimension] = &numericMapping[firstDimension][i];

        for (size_t k = 0;  k < endIdentifier2 && k < numericMapping[secondDimension].size();  k++) {
          if (numericMapping[secondDimension][k].empty()) {
            continue;
          }

          mappings[secondDimension] = &numericMapping[secondDimension][k];

          CubePage * page = lookupCubePage((IdentifierType) i, (IdentifierType) k);

          if (page != 0) {
            page->sort();

            computeHashAreaCellValue(storage->getArea(),
                                     page,
                                     minimalDimension,
                                     length - 1,
                                     numericMapping,
                                     mappings,
                                     user);
          }
        }
      }
    }
  }



  void CubeStorage::computeHashAreaCellValue (double* storage,
                                              CubePage * page,
                                              size_t minimalDimension,
                                              size_t maximalDimension,
                                              vector< vector< vector< pair<uint32_t, double> > > >& numericMapping,
                                              vector< vector< pair<uint32_t, double> >* >& mappings,
                                              User* user) {
    CubePage::buffer_t start = page->begin();
    CubePage::buffer_t stop = page->end();
    size_t rowSize = page->getRowSize();

    uint32_t lookUp = (uint32_t) maximalDimension;

    for (CubePage::buffer_t row = start;  row < stop;  row += rowSize) {
      uint32_t changeNum  = (* (uint32_t*) (row + valueSize + keySize)) & 0x3fffffff;
      
      if (lookUp <= changeNum) {
        bool found = true;
        
        IdentifierType* idPtr  = (IdentifierType*) (row + valueSize);

        for (uint32_t i = changeNum;  minimalDimension <= i;  i--) {
          lookUp = i;

          vector< pair<uint32_t, double> >& m = numericMapping[i][*(idPtr + i)];

          if (m.empty()) {
            found = false;
            break;
          }

          mappings[i] = &m;
        }
        
        if (found) {
          double value;

          if (isMarked(row)) {
            value = cube->computeRule(row + valueSize, * (double*) row, user);
          }
          else {
            value = * (double*) row;
          }

          setHashAreaValue(storage, value, mappings);
        }
      }
    }
  }

  

  void CubeStorage::setHashAreaValue (double* storage,
                                      double value,
                                      vector< vector< pair<uint32_t, double> >* >& mappings) {
    size_t length = mappings.size();

    vector<uint32_t> path(length, 0);
    vector<uint32_t> maxs(length, 0);

    vector< vector< pair<uint32_t, double> >* >::const_iterator mpIter = mappings.begin();
    vector< vector< pair<uint32_t, double> >* >::const_iterator mpEnd = mappings.end();
    vector<uint32_t>::iterator mIter = maxs.begin();

    for (; mpIter != mpEnd;  mpIter++, mIter++) {
      *mIter = (uint32_t) (*mpIter)->size();
    }

    size_t pos = 0;

    do {

      // compute new factor
      double v = value;
      uint32_t hash = 0;
      
      for (size_t i = 0;  i < length;  i++) {
        vector< pair<uint32_t, double> >* m = mappings[i];
        pair<uint32_t, double>& p = (*m)[path[i]];

        v *= p.second;
        hash += p.first;
      }
      
      if (isnan(storage[hash])) {
        storage[hash] = v;
      }
      else {
        storage[hash] += v;
      }

      // increment path
      pos = 0;

      while (pos < length) {
        path[pos]++;

        if (path[pos] == maxs[pos]) {
          path[pos] = 0;
          pos++;
        }
        else {
          break;
        }
      }
    }
    while (pos < length);
  }

  ////////////////////////////////////////////////////////////////////////////////
  // export cell values
  ////////////////////////////////////////////////////////////////////////////////

  void CubeStorage::computeExportCellValue (ExportStorage* storage,
                                            vector< map<IdentifierType, map<IdentifierType, double> > > *baseElements,
                                            bool baseElementsOnly,											
                                            User* user) {

    // convert the base elements to the corresponding keys and get the masks
    uint32_t length = (uint32_t) baseElements->size();

    // special case of degenerated cube, no dimension at all
    if (length == 0) {
      return;
    }
    
    // special case of degenerated cube, just one dimension
    else if (length == 1) {
      CubePage * page = lookupCubePage(0, 0);

      if (page == 0) {
        return;
      }

      page->sort();

      vector< map<IdentifierType, double>* > mappings(1);

      for (map<IdentifierType, map<IdentifierType, double> >::iterator i = (*baseElements)[0].begin();
           i != (*baseElements)[0].end();
           i++) {
        IdentifierType id = i->first;
        
        CubePage::buffer_t first = page->begin();
        CubePage::buffer_t last  = page->end();

        page->equal_range(&first, &last, id, valueSize);

        if (first != last) {
          mappings[0] = &i->second;

          double value;

		  //computeRule can lead to page->sort which conflicts with page->sortExport. Rule values are applied later.
          if (false && isMarked(first)) {
            value = cube->computeRule(first + valueSize, *(double*) first, user);
          }
          else {
            value = * (double*) first;
          }

          setExportValue(storage, value, mappings);
        }
      }
    }

    // special case of array, just two dimensions
    else if (length == 2) {
      
      vector< map<IdentifierType, double>* > mappings(2);
      
      for (map<IdentifierType, map<IdentifierType, double> >::iterator i = (*baseElements)[0].begin();
           i != (*baseElements)[0].end();
           i++) {
            
        mappings[0] = &i->second;        
        IdentifierType id1 = i->first;

        CubePage * page = lookupCubePage(id1, 0);

        if (page == 0) {
          continue;
        }

        page->sort();

        for (map<IdentifierType, map<IdentifierType, double> >::iterator j = (*baseElements)[1].begin();
             j != (*baseElements)[1].end();
             j++) {
          mappings[1] = &j->second;        
          IdentifierType id2 = j->first;

          CubePage::buffer_t first = page->begin();
          CubePage::buffer_t last  = page->end();

          page->equal_range(&first, &last, id2, valueSize + 4);

          if (first != last) {
            double value;

		    //computeRule can lead to page->sort which conflicts with page->sortExport. Rule values are applied later.
            if (false && isMarked(first)) {
              value = cube->computeRule(first + valueSize, *(double*) first, user);
            }
            else {
              value = * (double*) first;
            }

			setExportValue(storage, value, mappings);
          }
        }
      }
    }

    // more than two dimensions
    else {
      size_t minimalDimension = 0;      

      while (firstDimension == minimalDimension || secondDimension == minimalDimension) {
        minimalDimension++;
      }
      
      vector< map<IdentifierType, double>* > mappings(length);
      
      for (map<IdentifierType, map<IdentifierType, double> >::iterator i = (*baseElements)[firstDimension].begin();
           i != (*baseElements)[firstDimension].end();
           i++) {

        IdentifierType id1 = i->first;

        mappings[0] = &i->second;        

        if (id1 >= endIdentifier1) {
          break;
        }
        
        for (map<IdentifierType, map<IdentifierType, double> >::iterator j = (*baseElements)[secondDimension].begin();
             j != (*baseElements)[secondDimension].end();
             j++) {
          IdentifierType id2 = j->first;

          if (id2 >= endIdentifier2) {
            break;
          }

          CubePage * page = lookupCubePage(id1, id2);
          
          if (page != 0) {
            page->sortExport();
            // page->sort();
            mappings[1] = &j->second;        
            computeExportCellValueNotRecursive(storage,
                                               page,
                                               page->begin(),
                                               page->end(),
                                               baseElements,
                                               minimalDimension,
                                               length - 1,
                                               mappings,											   
                                               user);
                                 
            if (baseElementsOnly && storage->hasBlocksizeElements()) {
              // all elements found
              return;
            }
          }
        }
      }
    }
  }


  void CubeStorage::computeExportCellValueNotRecursive (ExportStorage* storage,
                                                        CubePage * page,
                                                        CubePage::buffer_t start,
                                                        CubePage::buffer_t stop,
                                                        vector< map<IdentifierType, map<IdentifierType, double> > >* baseElements,
                                                        size_t minimalDimension,
                                                        size_t dimension,
                                                        vector< map<IdentifierType, double>* >& mappings,														
                                                        User* user) {
    
    uint32_t max = (uint32_t) mappings.size();
    uint32_t lookUp = 2;
                                            
    size_t rowSize = page->getRowSize();
    for (CubePage::buffer_t row = start; row < stop; row += rowSize) {
      uint32_t changeNum  = (* (uint32_t*) (row + valueSize + keySize)) & 0x3fffffff;
      
      if (changeNum <= lookUp) {
        bool found = true;
        
        IdentifierType* idPtr  = (IdentifierType*) (row + valueSize);
        for (uint32_t i = changeNum; i < max; i++) {
          lookUp = i;
          map<IdentifierType, map<IdentifierType, double> >::iterator x = baseElements->at(i).find(* (idPtr+i) );
          if (x == baseElements->at(i).end()) {
            found = false;
            break;
          }
          mappings[i] = &x->second;
        }
        
        if (found) {
          double value;
		  //computeRule can lead to page->sort which conflicts with page->sortExport. Rule values are applied later.
          if (false && isMarked(row)) {
            value = cube->computeRule(row + valueSize, *(double*) row, user);
          }
          else {
            value = * (double*) row;
          }

          setExportValue(storage, value, mappings);
        }
      }
    }
  }



  void CubeStorage::setExportValue (ExportStorage* storage, double value, vector< map<IdentifierType, double>* >& mappings) {
    IdentifiersType path(mappings.size());    
    
    IdentifierType * buffer = (IdentifierType*) tmpKeyBuffer;
    
    size_t max = mappings.size();
    vector< map<IdentifierType, double>::iterator > iv;
    for (size_t i = 0; i < max; i++) {
      iv.push_back( mappings[i]->begin() );
      // path[i] = iv[i]->first;
      * (buffer+i) = iv[i]->first;
    }

    size_t errorPos = max;
    bool inArea = false; // storage->isInStorageArea(&path, errorPos); 

    size_t pos = max-1;
    do {
      if (pos <= errorPos) {
        inArea = storage->isInStorageArea(tmpKeyBuffer, errorPos);
      }
      if (inArea) {
        double v = value;

        for (size_t i = 0; i < max; i++) {
          v *= iv[i]->second;
        }

        if (storage->getNumberElements() == 10000) {
          for (size_t i = 0; i < max; i++) {
            iv.push_back( mappings[i]->begin() );
          }
        }

        // storage->addDoubleValue(&path, v);
        storage->addDoubleValue(tmpKeyBuffer, v);
        errorPos = max;
        pos = max-1;      
      }
      else {
        pos = errorPos;
      }      
      
      bool stop = false;
      while (!stop && pos>= 0 && pos < max) {
        iv[pos]++;
        if (iv[pos] == mappings[pos]->end()) {
          iv[pos] = mappings[pos]->begin();
          // path[pos] = iv[pos]->first;
          * (buffer+pos) = iv[pos]->first;
          pos--;
        }
        else {
          // path[pos] = iv[pos]->first;
          * (buffer+pos) = iv[pos]->first;
          stop = true;
        }
      }
      
    } while (pos>= 0 && pos < max); 
  }
  





  void CubeStorage::computeExportStringCellValue (ExportStorage* storage,
                                                  vector<IdentifiersType>* area) {

    // convert the base elements to the corresponding keys and get the masks
    uint32_t length = (uint32_t) area->size();

    // special case of degenerated cube, no dimension at all
    if (length == 0) {
      return;
    }
    
    // special case of degenerated cube, just one dimension
    else if (length == 1) {
      CubePage * page = lookupCubePage(0, 0);

      if (page == 0) {
        return;
      }

      page->sort();

      IdentifiersType path(1);

      for (IdentifiersType::iterator i = (*area)[0].begin();
           i != (*area)[0].end();
           i++) {
        IdentifierType id = *i;
        
        CubePage::buffer_t first = page->begin();
        CubePage::buffer_t last  = page->end();

        page->equal_range(&first, &last, id, valueSize);

        if (first != last) {
          if (storage->isInStorageArea(id)) {
            path[0] = id;
            CubePage::key_t keyBuffer = first + valueSize;
            char * * c = (char * *) first;
            string s(*c);

			storage->addStringValue(keyBuffer, s); 
          }       
        }
      }
    }

    // special case of array, just two dimensions
    else if (length == 2) {
      
      IdentifiersType path(2);
      
      for (IdentifiersType::iterator i = (*area)[0].begin();
           i != (*area)[0].end();
           i++) {
            
        IdentifierType id1 = *i;
        path[0] = id1;

        CubePage * page = lookupCubePage(id1, 0);

        if (page == 0) {
          continue;
        }

        page->sort();

        for (IdentifiersType::iterator j = (*area)[1].begin();
             j != (*area)[1].end();
             j++) {
        
          IdentifierType id2 = *j;
          path[1] = id2;

          CubePage::buffer_t first = page->begin();
          CubePage::buffer_t last  = page->end();

          page->equal_range(&first, &last, id2, valueSize + 4);

          if (first != last) {
            if (storage->isInStorageArea(id1, id2)) {
              CubePage::key_t keyBuffer = first + valueSize;
              char * * c = (char * *) first;
              string s(*c);
			  storage->addStringValue(keyBuffer, s); 
            }       
          }
        }
      }
    }

    // more than two dimensions, use recursion
    else {
      size_t minimalDimension = 0;      

      while (firstDimension == minimalDimension || secondDimension == minimalDimension) {
        minimalDimension++;
      }
      
      size_t errorPos = 0;
      
      IdentifiersType path(length);
      
      for (IdentifiersType::iterator i = (*area)[firstDimension].begin();
           i != (*area)[firstDimension].end();
           i++) {

        IdentifierType id1 = *i;
        path[0] = id1;

        if (id1 >= endIdentifier1) {
          break;
        }
        
        for (IdentifiersType::iterator j = (*area)[secondDimension].begin();
             j != (*area)[secondDimension].end();
             j++) {
          IdentifierType id2 = *j;

          if (id2 >= endIdentifier2) {
            break;
          }
          
          if (!storage->isInStorageArea(id1, id2)) {
            break;
          }

          CubePage * page = lookupCubePage(id1, id2);

          if (page != 0) {
            if (storage->isInStorageArea(id1, id2)) {
              size_t rowSize = page->getRowSize();
              page->sortExport();
              path[1] = *j;        
              for (CubePage::buffer_t row = page->begin(); row < page->end(); row += rowSize) {
                CubePage::key_t keyBuffer = row + valueSize;
                
                int inArea = storage->posArea(keyBuffer); 
                if (inArea == 0) {
                  if (storage->isInStorageArea(keyBuffer, errorPos)) {
                    char * * c = (char * *) row;
                    string s(*c);
                    storage->addStringValue(keyBuffer, s); 
                  }
                }
                else if (inArea == 1) {
                  return;
                }
              } 
            }
          }
        }
      }
    }
  }

  ////////////////////////////////////////////////////////////////////////////////
  // delete cell values
  ////////////////////////////////////////////////////////////////////////////////

  void CubeStorage::deleteByMask (const IdentifiersType* path, const IdentifiersType* mask) {
    CubePage::key_t keyBuffer  = new uint8_t [keySize];
    CubePage::key_t maskBuffer = new uint8_t [keySize];
    
    IdentifiersType maskPath;
    int pos = 0;

    for (IdentifiersType::const_iterator i = mask->begin(); i != mask->end(); i++, pos++) {
      if (*i == 1) {
        maskPath.push_back(maxima.at(pos));
      }
      else {
        maskPath.push_back(0);
      }
    }
        
    fillKeyBuffer(keyBuffer,  path);
    fillKeyBuffer(maskBuffer, &maskPath);
    
    vector<CubePage::element_t> freeable;
    size_t size; // will be set in getArray

    for (CubePage::element_t const * table = index->getArray(size);  0 < size;  table++) {
      if (*table != 0) {
        bool found = isEqualBuffer((*table) + valueSize, keyBuffer, maskBuffer);
        
        if (found) {
          freeable.push_back(*table);
        }
        
        size--;
      }
    }
    
    for (vector<uint8_t*>::iterator f = freeable.begin();  f != freeable.end();  f++) {
      CubePage::element_t element = *f;
      CubePage::key_t key = element + valueSize;

      deleteCell(key);
    }
    
    delete [] keyBuffer;
    delete [] maskBuffer;
  }



  void CubeStorage::removeCellValue (AreaNode::Area *baseElements, Lock* lock) {

    // convert the base elements to the corresponding keys and get the masks
    uint32_t length = (uint32_t) baseElements->size();

    // special case of degenerated cube, no dimension at all
    if (length == 0) {
      return;
    }
    
    // special case of degenerated cube, just one dimension
    else if (length == 1) {
      CubePage * page = lookupCubePage(0, 0);

      if (page == 0) {
        return;
      }

      page->sort();

      vector<CubePage::buffer_t> elements;

      for (set<IdentifierType>::iterator i = (*baseElements)[0].begin();
           i != (*baseElements)[0].end();
           i++) {
        IdentifierType id = *i;
        
        CubePage::buffer_t first = page->begin();
        CubePage::buffer_t last  = page->end();

        page->equal_range(&first, &last, id, valueSize);

        if (first != last) {
          elements.push_back(first);
        }
      }

      removeCubeCells(page, &elements, lock);
    }

    // special case of array, just two dimensions
    else if (length == 2) {
      
      for (set<IdentifierType>::iterator i = (*baseElements)[0].begin();
           i != (*baseElements)[0].end();
           i++) {
            
        IdentifierType id1 = *i;

        CubePage * page = lookupCubePage(id1, 0);

        if (page == 0) {
          continue;
        }

        page->sort();
        
        vector<CubePage::buffer_t> elements;

        for (set<IdentifierType>::iterator j = (*baseElements)[1].begin();
             j != (*baseElements)[1].end();
             j++) {
          IdentifierType id2 = *j;

          CubePage::buffer_t first = page->begin();
          CubePage::buffer_t last  = page->end();

          page->equal_range(&first, &last, id2, valueSize + 4);

          if (first != last) {
            elements.push_back(first);
          }
        }
        
        removeCubeCells(page, &elements, lock);
      }
    }

    // more than two dimensions, use recursion
    else {
      size_t minimalDimension = 0;      

      while (firstDimension == minimalDimension || secondDimension == minimalDimension) {
        minimalDimension++;
      }
            
      for (set<IdentifierType>::iterator i = (*baseElements)[firstDimension].begin();
           i != (*baseElements)[firstDimension].end();
           i++) {

        IdentifierType id1 = *i;

        if (id1 >= endIdentifier1) {
          break;
        }
        
        for (set<IdentifierType>::iterator j = (*baseElements)[secondDimension].begin();
             j != (*baseElements)[secondDimension].end();
             j++) {
              
          IdentifierType id2 = *j;

          if (id2 >= endIdentifier2) {
            break;
          }

          CubePage * page = lookupCubePage(id1, id2);

          if (page != 0) {
            page->sort();
            
            vector<CubePage::buffer_t> elements;
            
            getCellsNotRecursive(&elements,
                                 page,
                                 page->begin(),
                                 page->end(),
                                 baseElements,
                                 minimalDimension,
                                 length - 1);

            removeCubeCells(page, &elements, lock);
          }
        }
      }
    }
  }



  void CubeStorage::getCellsNotRecursive (vector<CubePage::element_t> * elements,
                                          CubePage * page,
                                          CubePage::buffer_t start,
                                          CubePage::buffer_t stop,
                                          AreaNode::Area* baseElements,
                                          size_t minimalDimension,
                                          size_t dimension) {
    
    uint32_t lookUp = (uint32_t) baseElements->size()-1;
                                            
    size_t rowSize = page->getRowSize();
    for (CubePage::buffer_t row = start; row < stop; row += rowSize) {
      uint32_t changeNum  = (* (uint32_t*) (row + valueSize + keySize)) & 0x3fffffff;
      
      if (changeNum >= lookUp) {
        bool found = true;
        
        IdentifierType* idPtr  = (IdentifierType*) (row + valueSize);

        for (uint32_t i = changeNum; i > 1; i--) {
          lookUp = i;
          set<IdentifierType>::iterator x = baseElements->at(i).find(* (idPtr + i) );

          if (x == baseElements->at(i).end()) {
            found = false;
            break;
          }
        }
        
        if (found) {
          elements->push_back(row);
        }
      }
    }
  }


  
  void CubeStorage::removeCubeCells (CubePage * page, vector<CubePage::buffer_t>  * elements, Lock* lock) {
    if (lock) {
      IdentifiersType path(maxima.size());
      
      for (vector<CubePage::buffer_t>::reverse_iterator i = elements->rbegin(); i != elements->rend(); i++) {
        lock->getStorage()->addCellValue((RollbackPage::buffer_t) *i); 
      }
    }
    
    for (vector<CubePage::buffer_t>::reverse_iterator i = elements->rbegin(); i != elements->rend(); i++) {
      if (isMarked(*i)) {
        * (double*) (*i) = 0;
      }
      else {
        page->removeElement(*i);
        numberElements--;
      }
    }
  }
  
  ////////////////////////////////////////////////////////////////////////////////
  // auxillary functions
  ////////////////////////////////////////////////////////////////////////////////
  
  bool CubeStorage::isEqualBuffer (uint8_t * buffer, uint8_t * keyBuffer, uint8_t * maskBuffer) {
    uint32_t * b = (uint32_t *) buffer;
    uint32_t * k = (uint32_t *) keyBuffer;
    uint32_t * m = (uint32_t *) maskBuffer;
    
    for (size_t i = 0;  i < keySize / 4;  i++, b++, k++, m++) {
      if ( (*b & *m) != *k) {
        return false;
      }
    }

    return true;
  }
  
  
  
  void CubeStorage::sort () {
    CubePage * const * cubePageArray = cubePages.getTable(); 
    size_t size = cubePages.size();

    for (size_t i = 0;  i < size;  cubePageArray++) {
      CubePage * page = *cubePageArray;

      if (page != 0) {
        page->sort();
        i++;
      }
    }
  }
}
