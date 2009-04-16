////////////////////////////////////////////////////////////////////////////////
/// @brief palo cache page
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

#include "Olap/CachePage.h"

#include <iostream>

#include "Exceptions/ErrorException.h"

#include "InputOutput/Logger.h"

#include "Olap/CubeIndex.h"

namespace palo {

  ////////////////////////////////////////////////////////////////////////////////
  // constructors and destructors
  ////////////////////////////////////////////////////////////////////////////////

  CachePage::CachePage (CubeIndex * index,
                        size_t keySize,
                        size_t valueSize,
                        size_t counterSize)
    : index(index),
      sorted(true),
      keySize(keySize), 
      valueSize(valueSize),
      counterSize(counterSize) {

    rowSize = ((valueSize + keySize + counterSize + 3) / 4) * 4; // align row size to 4 byte

    totalSize = PAGE_SIZE;
    numberElements = totalSize / rowSize;
    usedElements = 0;

    buffer = new uint8_t [totalSize];
    memset(buffer, 0, totalSize);

    tmpBuffer = new uint8_t [rowSize];
    memset(tmpBuffer, 0, rowSize);    
  }



  CachePage::~CachePage () {
    delete [] buffer;
    delete [] tmpBuffer;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // add and remove entries
  ////////////////////////////////////////////////////////////////////////////////

  CachePage::element_t CachePage::addElement (element_t row) {
    if (usedElements == numberElements) {
      grow();
    }

    element_t result = buffer + usedElements * rowSize;
    memcpy(result, row, valueSize + keySize + counterSize);

    index->addElement(result);

    usedElements++;
    sorted = false;

    return result;
  }



  CachePage::element_t CachePage::removeElement (element_t row) {
    size_t pos = (row - buffer) / rowSize;

    if (pos >= usedElements) {
      throw ErrorException(ErrorException::ERROR_INTERNAL,
                           "in CachePage::removeElement");
    }

    usedElements--;

    index->removeKey(row + valueSize);

    if (usedElements == 0) {
      return 0;
    }

    if (pos != usedElements) {
      index->removeKey(buffer + usedElements * rowSize + valueSize);
      copyElement(pos, usedElements);
      index->addElement(row);
    }

    sorted = false;


    return row;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // sort and find
  ////////////////////////////////////////////////////////////////////////////////

  void CachePage::sort () {
    if (sorted) {
      return;
    }

    // use a shell sort to sort the values
    size_t N = usedElements;
    size_t H = 1;

    while (9 * H + 4 < N)  { H = 3 * H + 1; };

    /* sort according to the path */
    size_t h = H;

    while (0 < h) {
      for (size_t i = h + 1;  i <= N;  i++) {
        saveElement(i - 1);
        size_t j = i;

        while (h < j && isBiggerElement(j - h - 1)) {
          copyElement(j - 1, j - h - 1);
          j = j - h;
        }

        restoreElement(j - 1);
      }

      h = h / 3;
    }
    
    sorted = true;
  }

  ////////////////////////////////////////////////////////////////////////////////
  // interal methods
  ////////////////////////////////////////////////////////////////////////////////

  void CachePage::grow () {
    // create new buffer
    size_t newTotalSize = ((3 * totalSize / 2 + PAGE_SIZE) / PAGE_SIZE) * PAGE_SIZE;
    buffer_t newBuffer = new uint8_t [newTotalSize];

    memset(newBuffer, 0, newTotalSize);
    memcpy(newBuffer, buffer, totalSize);

    // reindex
    CachePage::buffer_t ptr = newBuffer;
    CachePage::buffer_t end = newBuffer + usedElements * rowSize;

    for (;  ptr < end;  ptr += rowSize) {
      index->addElement((CachePage::element_t) ptr);
    }

    // delete old buffer
    delete[] buffer;

    buffer = newBuffer;
    totalSize = newTotalSize;
    numberElements = newTotalSize / rowSize;
  }


  void CachePage::shrink () {
    if (totalSize <= PAGE_SIZE) {      
      return;
    }

    // clear index
    index->clear();
    
    // sort page
    sort();

    // create new buffer
    size_t newTotalSize = ((totalSize / 2 + PAGE_SIZE) / PAGE_SIZE) * PAGE_SIZE;
    buffer_t newBuffer = new uint8_t [newTotalSize];

    size_t newElements = usedElements / 2;

    memset(newBuffer, 0, newTotalSize);
    memcpy(newBuffer, buffer, newElements * rowSize);

    // reindex
    CachePage::buffer_t ptr = newBuffer;
    CachePage::buffer_t end = newBuffer + newElements * rowSize;

    for (;  ptr < end;  ptr += rowSize) {
      index->addElement((CachePage::element_t) ptr);
    }

    // delete old buffer
    delete[] buffer;

    buffer = newBuffer;
    totalSize = newTotalSize;
    numberElements = newTotalSize / rowSize;
    usedElements = newElements;
  }


  void CachePage::deleteRows(size_t num, set<IdentifierType>* ids) {
    // delete rows by a list of ids of one dimension
    
    size_t pos = num * sizeof(IdentifierType);
    
    element_t ptr = buffer + usedElements * rowSize - rowSize;
    element_t end = buffer;
    for (;  ptr >= end;  ptr -= rowSize) {
      IdentifierType* id = (IdentifierType*) (ptr + valueSize + pos);
      set<IdentifierType>::iterator i = ids->find(*id);
      if (i != ids->end()) {
        removeElement(ptr);
      }
    }
    
  }
    
  void CachePage::saveElement (size_t pos) {
    memcpy(tmpBuffer, buffer + pos * rowSize, rowSize);
  }



  void CachePage::restoreElement (size_t pos) {
    memcpy(buffer + pos * rowSize, tmpBuffer, rowSize);
  }



  void CachePage::copyElement (size_t dst, size_t src) {
    memcpy(buffer + dst * rowSize, buffer + src * rowSize, rowSize);
  }



  bool CachePage::isBiggerElement (size_t right) {
    uint32_t* l = (uint32_t*) (tmpBuffer + valueSize + keySize);
    uint32_t* r = (uint32_t*) (buffer + right * rowSize + valueSize + keySize);
    
    if (*l > *r) {
      return true;
    }
    
    return false; 
  }


  ////////////////////////////////////////////////////////////////////////////////
  // print
  ////////////////////////////////////////////////////////////////////////////////

  void CachePage::print () {
    CachePage::buffer_t ptr = buffer;
    CachePage::buffer_t end = buffer + usedElements * rowSize;

    for (;  ptr < end;  ptr += rowSize) {
      double* d = (double*) ptr;
      uint32_t* counter = (uint32_t*) (ptr + valueSize + keySize);
      Logger::debug << "value= " << *d << " counter= " << *counter << endl;      
    }

  }


}
